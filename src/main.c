#include <pebble.h>

struct Game {
  int state;
  int play_time;
  int time_to_go;
  int added_time;
  int break_time;
  int period;
} game, templates[10];

#define GAME_STATE_UNSTARTED 0
#define GAME_STATE_STARTED 1
#define GAME_STATE_PAUSED 2
#define GAME_STATE_HALFTIME 5
#define GAME_STATE_ENDED 9

  
Window *my_window;
TextLayer *text_state_layer;
TextLayer *text_game_time_layer;
TextLayer *text_togo_layer;
TextLayer *text_added_time_layer;
TextLayer *text_break_time_layer;
static char play_time_buffer[50];
static char time_to_go_buffer[50];
static char added_time_buffer[50];
static char break_time_buffer[50];
bool game_started = false;
bool game_paused = false;
bool game_ended = false;
bool game_halftime = false;
int play_time = 0;
int time_to_go = 2700;
int added_time = 0;
int break_time = 900;
int period = 1;
int pause_reminder = 0;


static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!game_paused && !game_ended && game_started){
    text_layer_set_text(text_state_layer, "Game Paused");
    game_paused = true;
    vibes_short_pulse();
  } else if (game_paused && !game_ended && game_started){
    text_layer_set_text(text_state_layer, "Game Running");
    pause_reminder = 0;
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
    text_layer_set_text(text_state_layer, "Change +30");
    added_time = added_time + 30;
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  /* Game Logic */
  if (game_started && !game_halftime){play_time++;}
  if (game_started && !game_paused){
    if (time_to_go > 0){
      time_to_go--;
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
    if (break_time > 0){
      break_time--;
    } else if (period < 2){
      period++;
      vibes_long_pulse();
      text_layer_set_text(text_state_layer, "Start Game");
      game_halftime = false;
      game_started = false;
      play_time = 2700;
      time_to_go = 2700;
      added_time = 0;
    }
  }
  
  if (game_paused){
    added_time++;
    pause_reminder++;
  }
  if (pause_reminder >= 10){
    pause_reminder = 0;
    vibes_short_pulse();
  }
  /* Display */
  snprintf(play_time_buffer, sizeof(play_time_buffer), "%d            %.2d:%.2d", period,  play_time / 60, play_time % 60 );
  text_layer_set_text(text_game_time_layer, play_time_buffer);
  
  snprintf(time_to_go_buffer, sizeof(time_to_go_buffer), "%.2d:%.2d", time_to_go / 60, time_to_go % 60 );
  text_layer_set_text(text_togo_layer, time_to_go_buffer);
  
  snprintf(added_time_buffer, sizeof(added_time_buffer), "+ %.2d:%.2d", added_time / 60, added_time % 60 );
  text_layer_set_text(text_added_time_layer, added_time_buffer);
  
  snprintf(break_time_buffer, sizeof(break_time_buffer), "P %.2d:%.2d", break_time / 60, break_time % 60 );
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
  text_game_time_layer = text_layer_create(GRect(0, 20, 144, 50));
  text_togo_layer = text_layer_create(GRect(0, 65, 72, 90));
  text_break_time_layer = text_layer_create(GRect(72, 55, 144, 70));
  text_added_time_layer = text_layer_create(GRect(72, 70, 144, 90));
  
  text_layer_set_text_alignment(text_game_time_layer, GTextAlignmentRight);
  /*text_layer_set_text_alignment(text_break_time_layer, GTextAlignmentRight);
  text_layer_set_text_alignment(text_added_time_layer, GTextAlignmentRight);*/
  
  layer_add_child(window_layer, text_layer_get_layer(text_state_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_game_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_togo_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_break_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_added_time_layer));
  /* Font Setup */
  text_layer_set_font(text_game_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_font(text_togo_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_font(text_added_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_font(text_break_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
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
