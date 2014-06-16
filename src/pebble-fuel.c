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

// ---------------- Local includes  e.g., "file.h"

// ---------------- Constant definitions
#define FUEL_COUNT_KEY 1
#define FUEL_COUNT_DEFAULT 0
#define FUEL_COUNT_GOAL 2000

#define STREAK_KEY 2
#define STREAK_DEFAULT 0

#define MAX_FUEL_DIGITS 10

// ---------------- Macro definitions

// ---------------- Structures/Types

// ---------------- Private variables
static Window *window;
static int fuel_count = FUEL_COUNT_DEFAULT;
static int streak = STREAK_DEFAULT;
static char *fuel_string;
static TextLayer *fuel_text;

// ---------------- Private prototypes
static void tap_handler(AccelAxisType axis, int32_t direction);
static void window_load(Window *window);
static void update_fuel_display();
static void window_unload(Window *window);
static void init(void);
static void deinit(void);
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

  update_fuel_display();

  layer_add_child(window_layer, text_layer_get_layer(fuel_text));



  accel_tap_service_subscribe(tap_handler);
}

static void update_fuel_display() {
  snprintf(fuel_string, sizeof(fuel_string), "%d", fuel_count);
  text_layer_set_text(fuel_text, fuel_string);
}

static void window_unload(Window *window) {

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

  // initialize fuel string
  fuel_string = malloc(sizeof(char) * MAX_FUEL_DIGITS);

  window_stack_push(window, true);
}

static void deinit(void) {
  persist_write_int(FUEL_COUNT_KEY, fuel_count);
  persist_write_int(STREAK_KEY, streak);
  window_destroy(window);
  free(fuel_string);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
