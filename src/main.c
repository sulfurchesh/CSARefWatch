#include <pebble.h>

Window *my_window;
TextLayer *text_state_layer;
TextLayer *text_game_time_layer;
bool game_started;
bool game_paused;
bool game_ended;

void handle_init(void) {
	my_window = window_create();
  window_stack_push(my_window, true /* Animated */);
	text_state_layer = text_layer_create(GRect(0, 0, 144, 20));
  text_game_time_layer = text_layer_create(GRect(20, 0, 144, 40));
  Layer *window_layer = window_get_root_layer(my_window);
  layer_add_child(window_layer, text_layer_get_layer(text_state_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_game_time_layer));
}

static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!game_paused && !game_ended){
    text_layer_set_text(text_state_layer, "Paused Game");
    game_paused = true;
    vibes_short_pulse();
  } else if (game_paused && !game_ended){
    text_layer_set_text(text_state_layer, "Restarted Game");
    game_paused = false;
    vibes_short_pulse();
  }
}

static void up_long_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (!game_started)
  {
    text_layer_set_text(text_state_layer, "Started Game");
    vibes_short_pulse();
    game_started = true;
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  text_layer_set_text(text_game_time_layer, "tick");
}

void config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_down_click_handler, NULL);
}

static void init(void){
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  window_set_click_config_provider(my_window, config_provider);
  game_started = false;
  game_paused = false;
  game_ended = false;
}
void handle_deinit(void) {
	text_layer_destroy(text_state_layer);
  text_layer_destroy(text_game_time_layer);
	window_destroy(my_window);
}

int main(void) {
	handle_init();
  init();
	app_event_loop();
	handle_deinit();
}
