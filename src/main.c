#include <pebble.h>

struct Game {
  unsigned int state;
  unsigned int play_time;
  int time_to_go;
  unsigned int added_time;
  unsigned int break_time;
  unsigned int period;
  unsigned int pause_reminder;
} game = {0, 0, 2700, 0, 900, 1, 0};

#define GAME_UNSTARTED 0
#define GAME_STARTED 1
#define GAME_PAUSED 2
#define GAME_HALFTIME 4
#define GAME_ENDED 8

  
Window *my_window;
TextLayer *text_state_layer;
TextLayer *text_game_time_layer;
TextLayer *text_period_layer;
TextLayer *text_togo_layer;
TextLayer *text_added_time_layer;
TextLayer *text_break_time_layer;
static char play_time_buffer[20];
static char period_buffer[10];
static char time_to_go_buffer[20];
static char added_time_buffer[20];
static char break_time_buffer[20];
bool game_started = false;
bool game_paused = false;
bool game_ended = false;
bool game_halftime = false;


static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!game_paused && !game_ended && game_started){
    text_layer_set_text(text_state_layer, "Game Paused");
    game_paused = true;
    vibes_short_pulse();
  } else if (game_paused && !game_ended && game_started){
    text_layer_set_text(text_state_layer, "Game Running");
    game.pause_reminder = 0;
    game_paused = false;
    vibes_short_pulse();
  }
}

static void up_long_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (!game_started)
  {
    text_layer_set_text(text_state_layer, "Game Running");
    vibes_long_pulse();
    game_started = true;
  }
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (!game_started)
  {
    text_layer_set_text(text_state_layer, "Setup");
  } else if (game_paused){
    text_layer_set_text(text_state_layer, "Player Change +30s");
    game.added_time = game.added_time + 30;
    game.time_to_go = game.time_to_go + 30;
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  /* Game Logic */
  if (game_started && !game_halftime){game.play_time++;}
  if (game_started && !game_paused){
    if (game.time_to_go > 0){
      game.time_to_go--;
    } else if (!game_halftime){
      game_halftime = true;
      static const uint32_t const segments[] = { 500, 250, 500 };
      VibePattern pat = {
        .durations = segments,
        .num_segments = ARRAY_LENGTH(segments),
      };
      vibes_enqueue_custom_pattern(pat);
      text_layer_set_text(text_state_layer, "Halftime");
    }
  }
  /* Halftime */
  if (game_halftime){
    if (game.break_time > 0){
      game.break_time--;
    } else if (game.period < 2){
      game.period++;
      vibes_long_pulse();
      text_layer_set_text(text_state_layer, "Start Game");
      game_halftime = false;
      game_started = false;
      game.play_time = 2700;
      game.time_to_go = 2700;
      game.added_time = 0;
    }
  }
  
  if (game_paused){
    game.added_time++;
    game.pause_reminder++;
  }
  if (game.pause_reminder >= 10){
    game.pause_reminder = 0;
    vibes_short_pulse();
  }
  /* Display */
  snprintf(play_time_buffer, sizeof(play_time_buffer), "%.2d:%.2d",  game.play_time / 60, game.play_time % 60);
  text_layer_set_text(text_game_time_layer, play_time_buffer);
  
  snprintf(period_buffer, sizeof(period_buffer), "%d",  game.period);
  text_layer_set_text(text_period_layer, period_buffer);
  
  snprintf(time_to_go_buffer, sizeof(time_to_go_buffer), "%.2d:%.2d", game.time_to_go / 60, game.time_to_go % 60);
  text_layer_set_text(text_togo_layer, time_to_go_buffer);
  
  snprintf(added_time_buffer, sizeof(added_time_buffer), "+ %.2d:%.2d", game.added_time / 60, game.added_time % 60);
  text_layer_set_text(text_added_time_layer, added_time_buffer);
  
  snprintf(break_time_buffer, sizeof(break_time_buffer), "P %.2d:%.2d", game.break_time / 60, game.break_time % 60 );
  text_layer_set_text(text_break_time_layer, break_time_buffer);
}

void config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, select_long_click_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_down_click_handler, NULL);
}

void handle_init(void) {
	/* GUI Init */
  my_window = window_create();
  window_stack_push(my_window, true /* Animated */);
  Layer *window_layer = window_get_root_layer(my_window);
  /* Text Layer*/
	text_state_layer = text_layer_create(GRect(0, 0, 144, 20));
  text_game_time_layer = text_layer_create(GRect(20, 20, 144, 70));
  text_period_layer = text_layer_create(GRect(0, 30, 20, 70));
  text_togo_layer = text_layer_create(GRect(0, 75, 72, 105));
  text_break_time_layer = text_layer_create(GRect(72, 70, 144, 90));
  text_added_time_layer = text_layer_create(GRect(72, 90, 144, 110));
  
  text_layer_set_text_alignment(text_game_time_layer, GTextAlignmentCenter);
  /*text_layer_set_text_alignment(text_break_time_layer, GTextAlignmentRight);
  text_layer_set_text_alignment(text_added_time_layer, GTextAlignmentRight);*/
  
  /* Font Setup */
  text_layer_set_font(text_game_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_40)));
  //text_layer_set_font(text_game_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_font(text_period_layer,fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_24)));
  text_layer_set_font(text_togo_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_24)));
  text_layer_set_font(text_added_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_18)));
  text_layer_set_font(text_break_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_18)));
  
  layer_add_child(window_layer, text_layer_get_layer(text_state_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_game_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_period_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_togo_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_break_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_added_time_layer));
  
  /* Event Init */
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  window_set_click_config_provider(my_window, config_provider);
}

void handle_deinit(void) {
	text_layer_destroy(text_state_layer);
  text_layer_destroy(text_game_time_layer);
	window_destroy(my_window);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}
