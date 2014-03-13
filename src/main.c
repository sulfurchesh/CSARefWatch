#include <pebble.h>

Window *my_window;
TextLayer *text_state_layer;
TextLayer *text_game_time_layer;
TextLayer *text_togo_layer;
TextLayer *text_added_time_layer;
static char play_time_buffer[256];
static char time_to_go_buffer[256];
static char added_time_buffer[256];
bool game_started = false;
bool game_paused = false;
bool game_ended = false;
int play_time = 0;
int time_to_go = 2700;
int added_time = 0;
int pause_reminder = 0;


void handle_init(void) {
	my_window = window_create();
  window_stack_push(my_window, true /* Animated */);
	text_state_layer = text_layer_create(GRect(0, 0, 144, 20));
  text_game_time_layer = text_layer_create(GRect(0, 20, 144, 40));
  text_togo_layer = text_layer_create(GRect(0, 40, 144, 60));
  text_added_time_layer = text_layer_create(GRect(0, 60, 144, 80));
  Layer *window_layer = window_get_root_layer(my_window);
  layer_add_child(window_layer, text_layer_get_layer(text_state_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_game_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_togo_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_added_time_layer));
}

static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!game_paused && !game_ended && game_started){
    text_layer_set_text(text_state_layer, "Paused Game");
    game_paused = true;
    vibes_short_pulse();
  } else if (game_paused && !game_ended && game_started){
    text_layer_set_text(text_state_layer, "Restarted Game");
    pause_reminder = 0;
    game_paused = false;
    vibes_short_pulse();
  }
}

static void up_long_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (!game_started)
  {
    text_layer_set_text(text_state_layer, "Started Game");
    vibes_long_pulse();
    game_started = true;
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (game_started){play_time++;}
  if (game_started && !game_paused){time_to_go--;}
  if (game_paused){
    added_time++;
    pause_reminder++;
  }
  if (pause_reminder >= 10){
    pause_reminder = 0;
    vibes_short_pulse();
  }
  snprintf(play_time_buffer, sizeof(play_time_buffer), "Play Time: %.2d:%.2d", play_time / 60, play_time % 60 );
  text_layer_set_text(text_game_time_layer, play_time_buffer);
  snprintf(time_to_go_buffer, sizeof(time_to_go_buffer), "To Go: %.2d:%.2d", time_to_go / 60, time_to_go % 60 );
  text_layer_set_text(text_togo_layer, time_to_go_buffer);
  snprintf(added_time_buffer, sizeof(added_time_buffer), "Added Time: %.2d:%.2d", added_time / 60, added_time % 60 );
  text_layer_set_text(text_added_time_layer, added_time_buffer);
}

void config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_down_click_handler, NULL);
}

static void init(void){
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  window_set_click_config_provider(my_window, config_provider);
  /*game_started = false;
  game_paused = false;
  game_ended = false;
  play_time = 0;
  time_to_go = 2700;
  added_time = 0;*/
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
