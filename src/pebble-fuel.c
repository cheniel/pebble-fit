/* ========================================================================== */
/* File: pebble-points.c
 *
 * Author: Daniel Chen (github: cheniel)
 * Date: 6/15/14
 */
/* ========================================================================== */
// ---------------- Open Issues

// ---------------- System includes e.g., <stdio.h> 
#include <pebble.h>

// ---------------- Local includes	e.g., "file.h"

// ---------------- Constant definitions

// graphics definitions
#define WINDOW_HEIGHT 168
#define WINDOW_WIDTH 144
#define STATUS_BAR_WIDTH 5
#define BUFFER 3

// Max definitions
#define MAX_INFO_LENGTH 100
#define MAX_DATE_CHAR 30
#define MAX_TIME_CHAR 10

// keys for persistant storage
#define POINTS_COUNT_KEY 1
#define STREAK_KEY 2
#define GOAL_REACHED_KEY 3
#define DATE_KEY 4
#define RECORD_KEY 5

// ---------------- Macro definitions

// ---------------- Structures/Types

// ---------------- Private variables
static Window *window;

/* persist variables */
static int points_count;
static int streak;
static int goal_reached_today;
static int record;
static int goal = 100;
static char *date_string;

/* used for graphics */
static char *info_string;
static char *time_string;
static TextLayer *time_text;
static TextLayer *date_text;
static TextLayer *points_text;
static TextLayer *status_bar;
static TextLayer *status_helper_bar;
static GRect bounds;

// ---------------- Private prototypes
static void tap_handler(AccelAxisType axis, int32_t direction);
static void window_load(Window *window);
static void update_points_display();
static void window_unload(Window *window);
static void init(void);
static void deinit(void);
static void reset_day();
static void refresh_day();
static void minute_tick_handler(struct tm *tick_time, TimeUnits units_changed);
int main(void);

/* ========================================================================== */

int main(void) {
	init();
	app_event_loop();
	deinit();
}

/*
 * initializes windows, strings, layers, and populates window with layers
 */
static void init(void) {

	// create window
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	window_set_background_color(window, GColorBlack);

	// initialize strings
	date_string = calloc(MAX_DATE_CHAR, sizeof(char));
	info_string = malloc(sizeof(char) * MAX_INFO_LENGTH);
	time_string = calloc(MAX_TIME_CHAR, sizeof(char));

	// get persistent data
	points_count = persist_exists(POINTS_COUNT_KEY) ? 
		persist_read_int(POINTS_COUNT_KEY) : 0;
	streak = persist_exists(STREAK_KEY) ? 
		persist_read_int(STREAK_KEY) : 0;
	goal_reached_today = persist_exists(GOAL_REACHED_KEY) ? 
		persist_read_int(GOAL_REACHED_KEY) : 0;
	record = persist_exists(RECORD_KEY) ? 
		persist_read_int(RECORD_KEY) : 0;

	// check for date change
	refresh_day(); // get current date, store in date_string

	// begin creating layers
	// get bounds for use in creating layers
	Layer *window_layer = window_get_root_layer(window);
	bounds = layer_get_bounds(window_layer);

	// create the text layer that displays the points text
	time_text = text_layer_create((GRect) { 
		.origin = { STATUS_BAR_WIDTH, bounds.size.h / 2 - 25 }, 
		.size = { bounds.size.w - STATUS_BAR_WIDTH, 50 } 
	});
	text_layer_set_font(time_text, 
		fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
	text_layer_set_background_color(time_text, GColorBlack);
	text_layer_set_text_color(time_text, GColorWhite);
	text_layer_set_text_alignment(time_text, GTextAlignmentCenter);

	// create text layer
	date_text = text_layer_create((GRect) { 
		.origin = { STATUS_BAR_WIDTH, 0 }, 
		.size = { bounds.size.w - STATUS_BAR_WIDTH - BUFFER, 40 } 
	});
	text_layer_set_background_color(date_text, GColorBlack);
	text_layer_set_text_color(date_text, GColorWhite);
	text_layer_set_text_alignment(date_text, GTextAlignmentCenter);
	text_layer_set_font(date_text, 
		fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

	// initialize status helper bar (background)
	status_helper_bar = text_layer_create((GRect) { 
		.origin = { 0, 0 }, 
		.size = { STATUS_BAR_WIDTH, WINDOW_HEIGHT} 
	});
	text_layer_set_background_color(status_helper_bar, GColorWhite);

	// initialize status bar
	status_bar = text_layer_create((GRect) { 
		.origin = { 0, 0 }, 
		.size = { STATUS_BAR_WIDTH, WINDOW_HEIGHT - (WINDOW_HEIGHT 
			* points_count / goal) } 
	});
	text_layer_set_background_color(status_bar, GColorBlack);

	// initialize points 
	points_text = text_layer_create((GRect) { 
		.origin = { STATUS_BAR_WIDTH + BUFFER, WINDOW_HEIGHT - 60 }, 
		.size = { bounds.size.w - STATUS_BAR_WIDTH - BUFFER, 60 } 
	});
	text_layer_set_text_color(points_text, GColorWhite);
	text_layer_set_background_color(points_text, GColorBlack);
	text_layer_set_font(points_text, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(points_text, GTextAlignmentCenter);

	// add layers to window layer (order matters)
	layer_add_child(window_layer, text_layer_get_layer(status_helper_bar));
	layer_add_child(window_layer, text_layer_get_layer(status_bar));
	layer_add_child(window_layer, text_layer_get_layer(time_text));
	layer_add_child(window_layer, text_layer_get_layer(date_text));
	layer_add_child(window_layer, text_layer_get_layer(points_text));

	window_stack_push(window, true);
}

static void deinit(void) {
	
	// write persist variables
	persist_write_int(POINTS_COUNT_KEY, points_count);
	persist_write_int(STREAK_KEY, streak);
	persist_write_int(GOAL_REACHED_KEY, goal_reached_today);
	persist_write_string(DATE_KEY, date_string);
	persist_write_int(RECORD_KEY, record);

	// destroy components
	window_destroy(window);
	free(info_string);
	free(date_string);
	free(time_string);

	text_layer_destroy(date_text);
	text_layer_destroy(time_text);
	text_layer_destroy(status_helper_bar);
	text_layer_destroy(status_bar);
	text_layer_destroy(points_text);
}

// when app window is opened
static void window_load(Window *window) {
	update_points_display();

	// push the date
	text_layer_set_text(date_text, date_string);

	// subscribes to services
	tick_timer_service_subscribe(MINUTE_UNIT, minute_tick_handler);	
	accel_tap_service_subscribe(tap_handler);
}

static void window_unload(Window *window) {
	tick_timer_service_unsubscribe();
	accel_tap_service_unsubscribe();
}

// called when the user shakes the pebble
static void tap_handler(AccelAxisType axis, int32_t direction) {
	points_count++;
	update_points_display();
}

// called when the user shakes his/her pebble
static void update_points_display() {

	// check if current point count is a record
	if (points_count >= record ) {
		record = points_count;
	}

	// get info string to print
	snprintf(info_string, MAX_INFO_LENGTH, 
		"points: %d/%d\nstreak: %d\nrecord: %d\nbattery: %d", 
		points_count, goal, streak, record, 
		battery_state_service_peek().charge_percent);

	// push string to text layer
	text_layer_set_text(points_text, info_string);

	// adjust size of status bar based on new points
	text_layer_set_size(status_bar, (GSize) { 
		.w = STATUS_BAR_WIDTH, 
		.h = WINDOW_HEIGHT - (WINDOW_HEIGHT * points_count / goal) 
	});	

	// check for goal condition
	if (points_count >= goal && !goal_reached_today) {
		vibes_double_pulse();
		streak++;
		goal_reached_today = 1;
	}
}

// called every minute
static void minute_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	
	// update the time string
	time_t currentTime = time(NULL);
	struct tm* tm = localtime(&currentTime);
	strftime(time_string, MAX_DATE_CHAR, "%I:%M", tm);

	// remove preceding 0 if there is one
	if ( !strncmp(time_string, "0", 1) ) {
		// skip the first character and change the time
		time_string++;
		text_layer_set_text(time_text, time_string);	
		time_string--;	
	} else {
		// push time string changes to display
		text_layer_set_text(time_text, time_string);	
	}

	refresh_day();
}

// refreshes the day string
static void refresh_day() {

	// store date in date_string
	time_t currentTime = time(NULL);
	struct tm* tm = localtime(&currentTime);
	strftime(date_string, MAX_DATE_CHAR, "%B %d, %Y\n%A", tm);

	// check for a change in the date
	char *previous_date = calloc(MAX_DATE_CHAR, sizeof(char));
	if (persist_exists(DATE_KEY)) { // if there exists previous data

		// get the date that existed last time this app was open, 
		// store in previous_date
		persist_read_string(DATE_KEY, previous_date, MAX_DATE_CHAR);

		// if the stored date is not the same as the current date,
		// reset counters
		if ( strncmp(previous_date, date_string, strlen(previous_date)) ) {
			reset_day();
		} 
	} 
	free(previous_date);

}

// called when there is a new day
static void reset_day() {
	points_count = 0;
	goal_reached_today = 0;
}
