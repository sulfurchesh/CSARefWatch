#include <pebble.h>

struct Game {
  unsigned int state;
  unsigned int play_time;
  int time_to_go;
  unsigned int added_time;
  unsigned int break_time;
  unsigned int period;
  unsigned int nr_periods;
  unsigned int pause_reminder;
  unsigned int penalty_time;
  unsigned int change_time;
  unsigned int template;
  unsigned int penalty_times[6];
}
game = {0, 0, 2700, 0, 600, 1, 2, 0, 0, 0, 0, {0,0,0,0,0,0}};

typedef struct {
  char name[15];
  unsigned int nr_periods;
  unsigned int play_time[10];
  unsigned int break_time[10];
  unsigned int penalty_time;
  unsigned int change_time;
  unsigned int period;
} GameTemplate;

const GameTemplate game_templates[] = {
  {"Test!!!   ",2,{30,30},{45,0},0,10,0},                    /* 11: TEST. 2x.5, HT: .75m */
  {"2x45/5    ",2,{2700,2700},{300,0},0,0,0},                /*  0: 2x45, HT: 5m */
  {"L1O 45/15 ",2,{2700,2700},{900,0},0,30,0},               /*  1: L1O - 2x45, HT: 15m */
  {"2x45+10/5 ",4,{2700,2700,600,600},{300,300,0,0},0,0,0},  /*  2: 2x45, 2x10 ET, HT: 5m */
  {"OPDL 13/14",3,{1500,1500,1500},{480,480,0},0,30,0},      /*  3: OPDL U13/14 - 3x25, HT: 8m */
  {"OPDL 15+  ",2,{2400,2400},{600,0},0,30,0},               /*  4: OPDL U15/16 - 2x40, HT: 10m */
  {"2x25/5    ",2,{1500,1500},{300,0},0,0,0},                /*  5: 2x25, HT: 5m */
  {"2x30/5    ",2,{1800,1800},{300,0},0,0,0},                /*  6: 2x30, HT: 5m */
  {"2x35/5    ",2,{2100,2100},{300,0},0,0,0},                /*  7: 2x35, HT: 5m */
  {"2x40/5    ",2,{2400,2400},{300,0},0,0,0},                /*  8: 2x40, HT: 5m */
  {"4x12/252  ",4,{720,720,720,720},{120,300,120,0},0,0,0},  /*  9: 4x12, HT: 2m, 5m, 2m */
  {"4x15/252  ",4,{900,900,900,900},{120,300,120,0},0,0,0},  /* 10: 4x15, HT: 2m, 5m, 2m */
  {"Test 2/1 o",2,{120,120},{30,0},0,10,0},                  /* 11: TEST. 2x4, HT: 1m */
  {"Test 4/1 p",2,{240,240},{60,0},20,10,0}                  /* 12: TEST. 2x4, HT: 1m, penalties HIDDEN */
};
/* Define the number of templates above, not including the Test template */
#define NrTemplates 2

#define TRUE     1
#define FALSE    0

#define GAME_UNSTARTED 0
#define GAME_STARTED  (1 << 0) // Game was started or restarted
#define GAME_PAUSED   (1 << 1) // Game is paused
#define GAME_HALFTIME (1 << 2) // Halftime, or any other "break"
#define GAME_READY    (1 << 3) // Ready to restart from a "break"
#define GAME_ENDED    (1 << 4) // End of the game

#define SET_LENGTH     1
#define PERIODS        2
#define HT_LENGTH      3
#define ET_LENGTH      4

#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))

/* Global Vars*/  
Window *my_window;
TextLayer *text_state_layer;
TextLayer *text_game_time_layer;
TextLayer *text_period_layer;
TextLayer *text_togo_layer;
TextLayer *text_added_time_layer;
TextLayer *text_break_time_layer;
TextLayer *text_penalty_time_layer;
static char play_time_buffer[20];
static char period_buffer[10];
static char time_to_go_buffer[20];
static char added_time_buffer[20];
static char break_time_buffer[20];
static char penalty_buffer[80];

// We have kickoff! (the start of any period of play)
void startGame(void) {
  SET_BIT(game.state, GAME_STARTED);
  REMOVE_BIT(game.state, GAME_PAUSED);
  REMOVE_BIT(game.state, GAME_HALFTIME);
  REMOVE_BIT(game.state, GAME_READY);
}

// Mark our game as complete/over/done-like-dinner
void endGame(void) {
  REMOVE_BIT(game.state, GAME_HALFTIME);
  SET_BIT(game.state, GAME_ENDED);
  text_layer_set_text(text_state_layer, "Game complete");  
}

// We have halftime (or any break from play)
void startBreak(void) {
  SET_BIT(game.state, GAME_HALFTIME);
  REMOVE_BIT(game.state, GAME_STARTED);
  REMOVE_BIT(game.state, GAME_PAUSED);
  REMOVE_BIT(game.state, GAME_READY);
  static const uint32_t const segments[] = { 500, 250, 500, 250, 500 };
  VibePattern pat = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pat);
  // Have we passed/met the number of periods in the game? If so... we're done
  if (game.period >= game.nr_periods) {
    endGame();
  } else {
    text_layer_set_text(text_state_layer, "Halftime");
  }
}

// Halftime (or our break) has ended... ready to kickoff!
// We start by incrementing the game.period by one and resetting the times...
void endBreak(void) {
  game.period++;
  game.play_time = game_templates[game.template].play_time[game.period - 1];
  game.time_to_go = game_templates[game.template].play_time[game.period - 1];
  game.break_time = game_templates[game.template].break_time[game.period - 1];
  game.added_time = 0;
  text_layer_set_text(text_state_layer, "Ready to Play");
  REMOVE_BIT(game.state, GAME_HALFTIME);
  REMOVE_BIT(game.state, GAME_STARTED);
  REMOVE_BIT(game.state, GAME_PAUSED);
  SET_BIT(game.state, GAME_READY);
}

// PAUSE the game
void pauseGame(void) {
  SET_BIT(game.state, GAME_PAUSED);
}

// UnPAUSE the game
void continueGame(void) {
  REMOVE_BIT(game.state, GAME_PAUSED);
  game.pause_reminder = 0;
}

// Have we kicked off?
int isGameKickedOff(void) {
  if (game.state == GAME_UNSTARTED) {
    return FALSE;
  }
  return TRUE;
}

// Has the game ended?
int isGameEnded(void) {
  if (IS_SET(game.state, GAME_ENDED)) {
    return TRUE;
  }
  return FALSE;
}

// Are we in a break or ready for kickoff?
int isGameBreak(void) {
  if (IS_SET(game.state, GAME_HALFTIME) || IS_SET(game.state, GAME_READY)) {
    return TRUE;
  }
  return FALSE;
}

// Are we ready for a kickoff?
int isGameReady(void) {
  if (!isGameKickedOff() || IS_SET(game.state, GAME_READY)) {
    return TRUE;
  }
  return FALSE;  
}

// Have we paused the game?
int isGamePaused(void) {
  if (IS_SET(game.state, GAME_PAUSED)) {
    return TRUE;
  }
  return FALSE;
}

// Is the game clock actively running?
int isGameRunning(void) {
  if (!isGameKickedOff() || isGameEnded() || isGameBreak() || isGamePaused()) {
    return FALSE;
  }
  return TRUE;
}

// Literally something that just... returns.
void doNothing(void) {
  return;
}

void setGameTemplate(int template_no) {
  if (!isGameKickedOff()) {
    text_layer_set_text(text_state_layer, game_templates[template_no].name);
    game.time_to_go = game_templates[template_no].play_time[game.period - 1];
    game.break_time = game_templates[template_no].break_time[game.period - 1];
    game.penalty_time = game_templates[template_no].penalty_time;
    if (game.penalty_time == 0) {
      text_layer_set_text(text_penalty_time_layer, "");
    }
    game.change_time = game_templates[template_no].change_time;
    game.nr_periods = game_templates[template_no].nr_periods;
  }
}

void gameSetMode(void) {
  // TODO: Let's change this around entirely and put us into "GAME_SET" mode now.
  //       If in GAME_SET mode, let's just exit
  game.template++;
  // TODO: if (game.template >= (sizeof(game_templates) / sizeof(GameTemplate))){
  if (game.template > NrTemplates) {
    game.template = 0;
  }
  setGameTemplate(game.template);
}

static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!isGameKickedOff()) {
    gameSetMode();
  } else {
    text_layer_set_text(text_state_layer, "Game...");
  }
}

static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (isGameEnded()) {
    doNothing();
  } else if (isGameRunning()) {
    text_layer_set_text(text_state_layer, "Game Paused");
    vibes_short_pulse();
    pauseGame();
  } else if (isGamePaused()) {
    text_layer_set_text(text_state_layer, "Game Running");
    vibes_short_pulse();
    continueGame();
  }
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (isGameEnded()) {
    doNothing();
  } else if (!isGameKickedOff() || isGameReady()) {
    text_layer_set_text(text_state_layer, "Game Running");
    vibes_long_pulse();
    startGame();
  } else if (game.penalty_time != 0 && isGameRunning()) {
    text_layer_set_text(text_state_layer, "Time Penalty");
    int i;
    for(i = 0; i < 6; i++){
      if (game.penalty_times[i] == 0){
        game.penalty_times[i] = game.penalty_time;
        vibes_short_pulse();
        break;
      }
    }
  } else if (IS_SET(game.state, GAME_HALFTIME) && (game.break_time > 5)) {
      // Needed cause player can shorten the break
      game.break_time = 5;
  }
}

static void up_double_click(ClickRecognizerRef recognizer, void *context) {
  if (IS_SET(game.state, GAME_HALFTIME) && !IS_SET(game.state, GAME_READY)) {
    endBreak();
  }
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (isGameEnded()) {
    doNothing();
  } else if (!isGameKickedOff()) {
    gameSetMode();
  } else if (!isGameBreak() && !isGamePaused()) {
      text_layer_set_text(text_state_layer, "Player Change");
      // TODO: Reset away from this text...
      game.added_time = game.added_time + game.change_time;
      // We add the time to the "to_go", because the countdown ISN'T stopped...
      game.time_to_go = game.time_to_go + game.change_time;
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  /* Game Logic */
  if (isGameEnded()) {
    // If we're done the game, let's just completely skip/void this entire section
    return;
  }
  if (isGameRunning()) {
    if (game.time_to_go > 0) {
      game.time_to_go--;
    } else if (!IS_SET(game.state, GAME_HALFTIME)) {
      // This will end the game if that's where we happen to be too...
      startBreak();
    }
  }
  /* Halftime... */
  if (IS_SET(game.state, GAME_HALFTIME) && !IS_SET(game.state, GAME_READY)) {
    if (game.break_time > 0) {
      game.break_time--;
    } else {
      vibes_long_pulse();
      endBreak();
    }
  }
  
  // Let's deal with a paused game and do some notifying every 10s
  if (isGamePaused()) {
    game.added_time++;
    game.pause_reminder++;
  }
  if (game.pause_reminder >= 10) {
    game.pause_reminder = 0;
    vibes_short_pulse();
  }
  
  // Now deal with penalty timers
  if (isGameRunning()) {
    game.play_time++;
    if (game.penalty_time > 0) {
      int i;
      for(i = 0; i < 6; i++){
        if (game.penalty_times[i] > 0) {
          if (game.penalty_times[i] == 1) {
            vibes_long_pulse();
          }
          game.penalty_times[i]--;
        }
      }
    }
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
  
  // Update penalty text buffers if enabled
  if (game.penalty_time > 0) {
    typedef char string[10];
    static string temp_penalty_buffer[6];
    int i;
    for(i = 0; i < 6; i++) {
      snprintf((temp_penalty_buffer[i]), sizeof(string), "%d: %.2d:%.2d", i + 1, game.penalty_times[i] / 60, game.penalty_times[i] % 60);
    }
    snprintf(penalty_buffer, sizeof(penalty_buffer), " %s   %s\n %s   %s\n %s   %s", temp_penalty_buffer[0], temp_penalty_buffer[1],
             temp_penalty_buffer[2], temp_penalty_buffer[3], temp_penalty_buffer[4], temp_penalty_buffer[5]);
    text_layer_set_text(text_penalty_time_layer, penalty_buffer);
  }
}

void config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, select_long_click_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_click_handler, NULL);
  window_multi_click_subscribe(BUTTON_ID_UP, 2, 0, 500, true, up_double_click);
}

void handle_init(void) {
  /* GUI Init */
  my_window = window_create();
  window_stack_push(my_window, true /* Animated */);
  Layer *window_layer = window_get_root_layer(my_window);
  /* Text Layer*/
  text_state_layer = text_layer_create(GRect(0, 0, 144, 15));
  text_game_time_layer = text_layer_create(GRect(20, 15, 144, 65));
  text_period_layer = text_layer_create(GRect(0, 25, 20, 65));
  text_togo_layer = text_layer_create(GRect(0, 75, 72, 105));
  text_break_time_layer = text_layer_create(GRect(72, 65, 144, 85));
  text_added_time_layer = text_layer_create(GRect(72, 85, 144, 105));
  text_penalty_time_layer = text_layer_create(GRect(0, 110, 144, 168));
  
  text_layer_set_text_alignment(text_game_time_layer, GTextAlignmentCenter);
  /*text_layer_set_text_alignment(text_break_time_layer, GTextAlignmentRight);
  text_layer_set_text_alignment(text_added_time_layer, GTextAlignmentRight);*/
  
  /* Font Setup */
  text_layer_set_font(text_game_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_40)));
  text_layer_set_font(text_period_layer,fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_24)));
  text_layer_set_font(text_togo_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_24)));
  text_layer_set_font(text_added_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_18)));
  text_layer_set_font(text_break_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_18)));
  text_layer_set_font(text_penalty_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNIVERSELSE_14)));
  
  layer_add_child(window_layer, text_layer_get_layer(text_state_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_game_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_period_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_togo_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_break_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_added_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(text_penalty_time_layer));
  
  /* Event Init */
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  window_set_click_config_provider(my_window, config_provider);
  
  setGameTemplate(0);
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
