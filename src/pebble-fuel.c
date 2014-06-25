/* ========================================================================== */
/* File: pebble-fuel.c
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

#define FUEL_COUNT_DEFAULT 0
#define FUEL_COUNT_GOAL 100

#define STREAK_DEFAULT 0

#define MAX_FUEL_DIGITS 10
#define MAX_DATE_CHAR 20

#define FUEL_COUNT_KEY 1
#define STREAK_KEY 2
#define GOAL_REACHED_KEY 3
#define DATE_KEY 4

// ---------------- Macro definitions

// ---------------- Structures/Types

// ---------------- Private variables
static Window *window;

/* persist variables */
static int fuel_count;
static int streak;
static int goal_reached_today;
static char *date_string;

/* used for graphics */
static char *fuel_string;
static TextLayer *fuel_text;
static TextLayer *date_text;
static TextLayer *status_bar;
static GRect bounds;

// ---------------- Private prototypes
static void tap_handler(AccelAxisType axis, int32_t direction);
static void window_load(Window *window);
static void update_fuel_display();
static void window_unload(Window *window);
static void init(void);
static void deinit(void);
static void reset_day();
static void refresh_day();
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
	fuel_string = malloc(sizeof(char) * MAX_FUEL_DIGITS);

	// get persistent data
	fuel_count = persist_exists(FUEL_COUNT_KEY) ? persist_read_int(FUEL_COUNT_KEY) : FUEL_COUNT_DEFAULT;
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

	// create the text layer that displays the fuel text
	fuel_text = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 50 } });
	text_layer_set_font(fuel_text, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	text_layer_set_background_color(fuel_text, GColorBlack);
	text_layer_set_text_color(fuel_text, GColorWhite);
	text_layer_set_text_alignment(fuel_text, GTextAlignmentRight);

	// create text layer
	date_text = text_layer_create((GRect) { .origin = { 0, 50 }, .size = { bounds.size.w, 50 } });
	text_layer_set_background_color(date_text, GColorBlack);
	text_layer_set_text_color(date_text, GColorWhite);
	text_layer_set_text_alignment(date_text, GTextAlignmentRight);

	// initialize status bar
	status_bar = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { 20, 168 * fuel_count / FUEL_COUNT_GOAL } });
	text_layer_set_background_color(status_bar, GColorWhite);

	fuel_count = 0;

	// add layers to window layer
	layer_add_child(window_layer, text_layer_get_layer(fuel_text));
	layer_add_child(window_layer, text_layer_get_layer(date_text));
	layer_add_child(window_layer, text_layer_get_layer(status_bar));

	window_stack_push(window, true);
}

static void window_load(Window *window) {
	update_fuel_display();
	text_layer_set_text(date_text, date_string);

	accel_tap_service_subscribe(tap_handler);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
	fuel_count++;
	update_fuel_display();
}

// called when the user shakes his/her pebble
static void update_fuel_display() {
	snprintf(fuel_string, MAX_FUEL_DIGITS, "%d", fuel_count);
	text_layer_set_text(fuel_text, fuel_string);

	// used for bar
	text_layer_set_size(status_bar, (GSize) { .w = 20, .h = 168 * fuel_count / FUEL_COUNT_GOAL });	

	// check for goal condition
	if (fuel_count >= FUEL_COUNT_GOAL) {
		date_string = "GOAL";
		text_layer_set_text(date_text, date_string);

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

static void refresh_day() {

	// store date in date_string
	time_t currentTime = time(NULL);
	struct tm* tm = localtime(&currentTime);
	strftime(date_string, MAX_DATE_CHAR, "%b %d, %Y", tm);

}

static void reset_day() {
	fuel_count = 0;
	goal_reached_today = 0;
}

static void deinit(void) {
	
	// write persist variables
	persist_write_int(FUEL_COUNT_KEY, fuel_count);
	persist_write_int(STREAK_KEY, streak);
	persist_write_int(GOAL_REACHED_KEY, goal_reached_today);

	// write date string if it has not been modified to become a non-date.
	if ( !goal_reached_today ) {
		persist_write_string(DATE_KEY, date_string);
		free(date_string);
	}

	// destroy components
	window_destroy(window);
	free(fuel_string);
	text_layer_destroy(date_text);
	text_layer_destroy(fuel_text);
}
