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
#define WINDOW_HEIGHT 168
#define WINDOW_WIDTH 144

#define STATUS_BAR_WIDTH 5

#define POINTS_COUNT_DEFAULT 0
#define POINTS_COUNT_GOAL 100

#define STREAK_DEFAULT 0

#define MAX_POINTS_DIGITS 4
#define MAX_DATE_CHAR 20
#define MAX_TIME_CHAR 10

#define POINTS_COUNT_KEY 1
#define STREAK_KEY 2
#define GOAL_REACHED_KEY 3
#define DATE_KEY 4

// ---------------- Macro definitions

// ---------------- Structures/Types

// ---------------- Private variables
static Window *window;

/* persist variables */
static int points_count;
static int streak;
static int goal_reached_today;
static char *date_string;

/* used for graphics */
static char *points_string;
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

static void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});

	window_set_background_color(window, GColorBlack);

	// initialize strings
	date_string = calloc(MAX_DATE_CHAR, sizeof(char));
	char *previous_date = calloc(MAX_DATE_CHAR, sizeof(char));
	points_string = malloc(sizeof(char) * MAX_POINTS_DIGITS);
	time_string = calloc(MAX_TIME_CHAR, sizeof(char));

	// get persistent data
	points_count = persist_exists(POINTS_COUNT_KEY) ? persist_read_int(POINTS_COUNT_KEY) : POINTS_COUNT_DEFAULT;
	streak = persist_exists(STREAK_KEY) ? persist_read_int(STREAK_KEY) : STREAK_DEFAULT;
	goal_reached_today = persist_exists(GOAL_REACHED_KEY) ? persist_read_int(GOAL_REACHED_KEY) : 0;

	// check for date change
	refresh_day(); // get current date, store in date_string
	if (persist_exists(DATE_KEY)) { // if there exists previous data

		// get the data, store in previous_date
		persist_read_string(DATE_KEY, previous_date, MAX_DATE_CHAR);

		// if the date has changed, reset the date
		if ( strncmp(previous_date, date_string, strlen(previous_date)) ) {
			reset_day();
		} 
	} 

	free(previous_date);

	// begin creating layers
	// get bounds for use in creating layers
	Layer *window_layer = window_get_root_layer(window);
	bounds = layer_get_bounds(window_layer);

	// create the text layer that displays the points text
	time_text = text_layer_create((GRect) { .origin = { STATUS_BAR_WIDTH, 0 }, .size = { bounds.size.w - STATUS_BAR_WIDTH, 50 } });
	text_layer_set_font(time_text, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	text_layer_set_background_color(time_text, GColorBlack);
	text_layer_set_text_color(time_text, GColorWhite);
	text_layer_set_text_alignment(time_text, GTextAlignmentRight);

	// create text layer
	date_text = text_layer_create((GRect) { .origin = { STATUS_BAR_WIDTH, 50 }, .size = { bounds.size.w - STATUS_BAR_WIDTH, 50 } });
	text_layer_set_background_color(date_text, GColorBlack);
	text_layer_set_text_color(date_text, GColorWhite);
	text_layer_set_text_alignment(date_text, GTextAlignmentRight);

	// initialize status helper bar (background)
	status_helper_bar = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { STATUS_BAR_WIDTH, WINDOW_HEIGHT} });
	text_layer_set_background_color(status_helper_bar, GColorWhite);

	// initialize status bar
	status_bar = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { STATUS_BAR_WIDTH, WINDOW_HEIGHT - (WINDOW_HEIGHT * points_count / POINTS_COUNT_GOAL) } });
	text_layer_set_background_color(status_bar, GColorBlack);

	// initialize points 
	points_text = text_layer_create((GRect) { .origin = { STATUS_BAR_WIDTH, WINDOW_HEIGHT - 20 }, .size = { bounds.size.w - STATUS_BAR_WIDTH, 50 } });
	text_layer_set_text_color(points_text, GColorWhite);
	text_layer_set_background_color(points_text, GColorBlack);

	// add layers to window layer
	layer_add_child(window_layer, text_layer_get_layer(status_helper_bar));
	layer_add_child(window_layer, text_layer_get_layer(status_bar));
	layer_add_child(window_layer, text_layer_get_layer(time_text));
	layer_add_child(window_layer, text_layer_get_layer(date_text));
	layer_add_child(window_layer, text_layer_get_layer(points_text));

	window_stack_push(window, true);
}

static void window_load(Window *window) {
	update_points_display();
	text_layer_set_text(date_text, date_string);

	tick_timer_service_subscribe(SECOND_UNIT, minute_tick_handler);	
	accel_tap_service_subscribe(tap_handler);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
	points_count++;
	update_points_display();
}

// called when the user shakes his/her pebble
static void update_points_display() {
	snprintf(points_string, MAX_POINTS_DIGITS, "%d", points_count);
	text_layer_set_text(points_text, points_string);

	// used for bar
	text_layer_set_size(status_bar, (GSize) { .w = STATUS_BAR_WIDTH, .h = WINDOW_HEIGHT - (WINDOW_HEIGHT * points_count / POINTS_COUNT_GOAL) });	

	// check for goal condition
	if (points_count >= POINTS_COUNT_GOAL) {

		if ( !goal_reached_today ) {

			// TODO create custom vibration
			vibes_double_pulse();
			streak++;
			goal_reached_today = 1;
		}
	}
}

static void window_unload(Window *window) {

}

static void minute_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	time_t currentTime = time(NULL);
	struct tm* tm = localtime(&currentTime);
	strftime(time_string, MAX_DATE_CHAR, "%I:%M", tm);

	text_layer_set_text(time_text, time_string);	
}


static void refresh_day() {

	// store date in date_string
	time_t currentTime = time(NULL);
	struct tm* tm = localtime(&currentTime);
	strftime(date_string, MAX_DATE_CHAR, "%b %d, %Y", tm);

}

static void reset_day() {
	points_count = 0;
	goal_reached_today = 0;
}

static void deinit(void) {
	
	// write persist variables
	persist_write_int(POINTS_COUNT_KEY, points_count);
	persist_write_int(STREAK_KEY, streak);
	persist_write_int(GOAL_REACHED_KEY, goal_reached_today);
	persist_write_string(DATE_KEY, date_string);


	// destroy components
	window_destroy(window);
	free(points_string);
	free(date_string);

	text_layer_destroy(date_text);
	text_layer_destroy(time_text);
}
