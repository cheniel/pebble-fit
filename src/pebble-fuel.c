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
#define FUEL_COUNT_DEFAULT 0
#define FUEL_COUNT_GOAL 500

#define STREAK_DEFAULT 0

#define MAX_FUEL_DIGITS 10
#define MAX_DATE_CHAR 20

#define FUEL_COUNT_KEY 1
#define STREAK_KEY 2
#define DATE_KEY 3


// ---------------- Macro definitions

// ---------------- Structures/Types

// ---------------- Private variables
static Window *window;
static int fuel_count = FUEL_COUNT_DEFAULT;
static int streak = STREAK_DEFAULT;
static char *fuel_string;
static char *date_string = "\0";
static TextLayer *fuel_text;
static TextLayer *date_text;

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

static void tap_handler(AccelAxisType axis, int32_t direction) {
	fuel_count++;
	update_fuel_display();
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	// create the text layer that displays the fuel text
	fuel_text = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 50 } });
	text_layer_set_font(fuel_text, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	text_layer_set_background_color(fuel_text, GColorBlack);
	text_layer_set_text_color(fuel_text, GColorWhite);
	text_layer_set_text_alignment(fuel_text, GTextAlignmentRight);

	update_fuel_display();

	layer_add_child(window_layer, text_layer_get_layer(fuel_text));

	// create text layer
	date_text = text_layer_create((GRect) { .origin = { 0, 50 }, .size = { bounds.size.w, 50 } });
	text_layer_set_background_color(date_text, GColorBlack);
	text_layer_set_text_color(date_text, GColorWhite);
	text_layer_set_text_alignment(date_text, GTextAlignmentRight);

	// create the date layer
	text_layer_set_text(date_text, date_string);
	layer_add_child(window_layer, text_layer_get_layer(date_text));

	accel_tap_service_subscribe(tap_handler);
}

static void update_fuel_display() {
	snprintf(fuel_string, MAX_FUEL_DIGITS, "%d", fuel_count);
	text_layer_set_text(fuel_text, fuel_string);

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
}

static void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});

	window_set_background_color(window, GColorBlack);

	// get persistent data
	fuel_count = persist_exists(FUEL_COUNT_KEY) ? persist_read_int(FUEL_COUNT_KEY) : FUEL_COUNT_DEFAULT;
	streak = persist_exists(STREAK_KEY) ? persist_read_int(STREAK_KEY) : STREAK_DEFAULT;
	
	date_string = calloc(MAX_DATE_CHAR, sizeof(char));
	refresh_day();

	// allocate previous_date
	char *previous_date = calloc(MAX_DATE_CHAR, sizeof(char));

	// if there exists previous data
	if (persist_exists(DATE_KEY)) {

		// get the data
		persist_read_string(DATE_KEY, previous_date, MAX_DATE_CHAR);

		// if the date has changed, reset the date
		if ( strncmp(previous_date, date_string, strlen(previous_date)) ) {
			reset_day();
		} 
	} 

	free(previous_date);

	// initialize fuel string
	fuel_string = malloc(sizeof(char) * MAX_FUEL_DIGITS);

	window_stack_push(window, true);
}

static void deinit(void) {
	
	// write persist variables
	persist_write_int(FUEL_COUNT_KEY, fuel_count);
	persist_write_int(STREAK_KEY, streak);
	persist_write_string(DATE_KEY, date_string);

	// destroy components
	window_destroy(window);
	free(fuel_string);
	free(date_string);
	text_layer_destroy(date_text);
	text_layer_destroy(fuel_text);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
