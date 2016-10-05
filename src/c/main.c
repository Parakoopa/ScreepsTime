#include <pebble.h>

/*
 * REDUCING POWER CONSUMPTION:
 *  - Track the number of canvas redraws we're doing.
 * BUGS / PROBLEMS:
 *  - Sometimes 'stalls' and wont request? anymore updates until the watchface is reset. Time still displays correctly so it IS redrawing.
 * TODO List / Features:
 *  - Option to vibrate on the hour?
 */

static void rebind_tick_handler();

uint32_t PKEY_RECEIVED = 0;           // Have we recieved our first configuration burst?
uint32_t PKEY_BT_DISCONNECT = 1; 
uint32_t PKEY_BT_RECONNECT = 2; 
uint32_t PKEY_WAKE_ON_CONNECT = 3;
uint32_t PKEY_WAKE_ON_DISCONNECT = 4;
uint32_t PKEY_USE_WEATHER = 5;
uint32_t PKEY_MAIL_SHOW = 6;
uint32_t PKEY_MAIL_RAIL = 7;
uint32_t PKEY_BATTERY_RAIL = 8;
uint32_t PKEY_BATTERY_THRESHOLD = 9;
uint32_t PKEY_BATTERY_MAIN = 10;
uint32_t PKEY_BLUETOOTH_MAIN = 11;
uint32_t PKEY_POLL_SCREEPS = 12;
uint32_t PKEY_POLL_WEATHER = 13;

// All of these have a length of 4, make sure you compensate.
uint32_t PKEY_LAST_WEATHER = 49;
uint32_t PKEY_SCREEPS_TEXT = 50;
uint32_t PKEY_SCREEPS_TEXT_COLOR = 54;
uint32_t PKEY_SCREEPS_TEXT2_COLOR = 58;
uint32_t PKEY_SCREEPS_OVER_COLOR = 62;
uint32_t PKEY_SCREEPS_UNDER_COLOR = 66;
uint32_t PKEY_SCREEPS_PROGRESS = 70;
uint32_t PKEY_SCREEPS_BLINK = 74;
uint32_t PKEY_SCREEPS_BOLD = 78;

static Window *s_window;              // Main watchface window, rendering goes here.
static Layer *s_parts;                // Layer containing the overlay fields.
static Layer *s_canvas;               // Canvas layer for drawing primitives.

static TextLayer *s_time_layer;       // TextLayer containing the current time. (IE: 10:28)
static TextLayer *s_weather_layer;    // TextLayer containing the current weather. (IE: Wilsonville - 28*)
static TextLayer *s_date_layer;       // TextLayer containing the current date. (IE: Mon 26 Sep, 2016)

static TextLayer *s_dynamic_layer[4];

static bool s_bluetooth_connected;
static bool s_battery_charging, s_battery_plugged;
static int s_battery_level;

static GRect bounds;

static int mailCount;
static char mailBuf[8];  
static char dynamicBuf[4][64];
static char weatherBuf[64];
static GColor textColorBuf[4];
static GColor textSecondColorBuf[4];
static GColor bgColorUnder[4];
static GColor bgColorOver[4];
static int progressBuf[4];
static bool blink[4];
static bool bold[4];
static int last_vibrate = 0;
  
static bool requesting_screeps_data = true;
static bool appReady = false;
static bool firstData = false;
static bool firstBTCheck = false;

static int poll_screeps = 1;
static int poll_weather = 30;

// This automatically toggles every second, so we can redraw for blinking.
static bool blinking;

// Save our display data to persistant memory, to make watchface redraw faster when loading. (We'll still go grab up-to-date data)
static void persist_display() {
  static int i;
  APP_LOG(APP_LOG_LEVEL_INFO, "Saving display data...");
  for ( i = 0; i < 4; i++ ) {
    persist_write_string(PKEY_SCREEPS_TEXT + i, dynamicBuf[i]);
    persist_write_int(PKEY_SCREEPS_TEXT_COLOR + i, textColorBuf[i].argb);
    persist_write_int(PKEY_SCREEPS_TEXT2_COLOR + i, textSecondColorBuf[i].argb);
    persist_write_int(PKEY_SCREEPS_OVER_COLOR + i, bgColorOver[i].argb);
    persist_write_int(PKEY_SCREEPS_UNDER_COLOR + i, bgColorUnder[i].argb);
    persist_write_int(PKEY_SCREEPS_PROGRESS + i, progressBuf[i]);
    persist_write_bool(PKEY_SCREEPS_BLINK + i, blink[i]);
    persist_write_bool(PKEY_SCREEPS_BOLD + i, bold[i]);
  }
  persist_write_string(PKEY_LAST_WEATHER, weatherBuf);
}

// Reload our persisted display data.
static void recover_display() {
  static int i;
  APP_LOG(APP_LOG_LEVEL_INFO, "Recovering display data...");
  for ( i = 0; i < 4; i++ ) {
    persist_read_string(PKEY_SCREEPS_TEXT + i, dynamicBuf[i], sizeof(dynamicBuf[i]));
    textColorBuf[i] = (GColor){.argb = ((uint8_t)(persist_read_int(PKEY_SCREEPS_TEXT_COLOR + i)))};
    textSecondColorBuf[i] = (GColor){.argb = ((uint8_t)(persist_read_int(PKEY_SCREEPS_TEXT2_COLOR + i)))};
    bgColorOver[i] = (GColor){.argb = ((uint8_t)(persist_read_int(PKEY_SCREEPS_OVER_COLOR + i)))};
    bgColorUnder[i] = (GColor){.argb = ((uint8_t)(persist_read_int(PKEY_SCREEPS_UNDER_COLOR + i)))};
    progressBuf[i] = persist_read_int(PKEY_SCREEPS_PROGRESS + i);
    blink[i] = persist_read_bool(PKEY_SCREEPS_BLINK + i);
    bold[i] = persist_read_bool(PKEY_SCREEPS_BOLD + i);
    
    text_layer_set_text_color(s_dynamic_layer[i], textColorBuf[i]);    
    if ( bold[i] ) text_layer_set_font(s_dynamic_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    if ( !bold[i] ) text_layer_set_font(s_dynamic_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_14));
  }
  persist_read_string(PKEY_LAST_WEATHER, weatherBuf, sizeof(weatherBuf));
  
  layer_mark_dirty(window_get_root_layer(s_window));
}


// Wake up the screen, but only if we enabled light in a specific config setting.
static void configurable_wake(uint32_t config_key) {
  static bool mode;
  mode = persist_read_bool(config_key);
  if ( mode ) light_enable_interaction();
}

// Run a vibration based off a configuration key.
static void configurable_vibrate(uint32_t config_key) {
  static int32_t mode;
  mode = persist_read_int(config_key);
  
  // Short, Long, Double
  if ( mode == 1 ) vibes_short_pulse();
  if ( mode == 2 ) vibes_long_pulse();
  if ( mode == 3 ) vibes_double_pulse();  
}

static void request_weather() {
  if ( !appReady || !firstData ) return;
  if ( persist_read_bool(PKEY_USE_WEATHER) ) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Requesting weather...");

    DictionaryIterator *out;
    app_message_outbox_begin(&out);
    dict_write_uint8(out, MESSAGE_KEY_REQUEST_WEATHER, 1);
    app_message_outbox_send();    
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Weather disabled, skipping request.");
  }
}

static void apply_special_rails() {
  static int rail, bThreshold;
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "Applying special rails.. %d %d", persist_read_bool(PKEY_MAIL_SHOW), bThreshold);
  
  bThreshold = persist_read_int(PKEY_BATTERY_THRESHOLD);     
  if ( (bThreshold > 0) && (s_battery_level <= bThreshold) ) {
    rail = persist_read_int(PKEY_BATTERY_RAIL);
    if ( rail < 0 ) rail = 0;
    if ( rail > 3 ) rail = 3;
    
    // APP_LOG(APP_LOG_LEVEL_ERROR, "Applying battery rail on %d (%ld)", rail, persist_read_int(PKEY_BATTERY_RAIL));
    snprintf(dynamicBuf[rail], sizeof(dynamicBuf[rail]), "%d%% Battery", s_battery_level);
    text_layer_set_text_color(s_dynamic_layer[rail], GColorBlack);
    bgColorUnder[rail] = GColorDarkGreen; // GColorCobaltBlue;
    bgColorOver[rail] = GColorKellyGreen; // GColorVividCerulean;
    progressBuf[rail] = s_battery_level;
  }
  if ( persist_read_bool(PKEY_MAIL_SHOW) && mailCount > 0 ) {
    rail = persist_read_int(PKEY_MAIL_RAIL);
    if ( rail < 0 ) rail = 0;
    if ( rail > 3 ) rail = 3;
    
    // APP_LOG(APP_LOG_LEVEL_ERROR, "Applying mail notification rail on %d", rail);
    snprintf(dynamicBuf[rail], sizeof(dynamicBuf[rail]), "%d New Message%s", mailCount, (mailCount == 1 ? "" : "s"));
    text_layer_set_text_color(s_dynamic_layer[rail], GColorBlack);
    bgColorUnder[rail] = GColorGreen; // GColorDarkGreen; // GColorCobaltBlue;
    bgColorOver[rail] = GColorGreen; // GColorMayGreen; // GColorVividCerulean;
    progressBuf[rail] = 0;
    
  }
}

static void request_screeps_data() {
  if ( !appReady || !firstData ) return;
  if ( requesting_screeps_data ) return;
  
  // requesting_screeps_data = true;
  
  APP_LOG(APP_LOG_LEVEL_ERROR, "Requesting Screeps Data...");

  DictionaryIterator *out;
  app_message_outbox_begin(&out);
  dict_write_uint8(out, MESSAGE_KEY_REQUEST_SCREEPS_API, 1);
  app_message_outbox_send();    
}

static void bluetooth_callback(bool connected) {
  // State changed, mark the window for redraw.
  if ( s_bluetooth_connected != connected ) layer_mark_dirty(window_get_root_layer(s_window));
  
  s_bluetooth_connected = connected;

  // Prevent vibrate on watchface first-run
  if ( !firstBTCheck ) { firstBTCheck = true; return; }  
  if ( connected ) {
    configurable_wake(PKEY_WAKE_ON_CONNECT);
    configurable_vibrate(PKEY_BT_RECONNECT);
  } else {
    configurable_wake(PKEY_WAKE_ON_DISCONNECT);
    configurable_vibrate(PKEY_BT_DISCONNECT);
  }
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;  // Record the new battery level  
  s_battery_charging = state.is_charging;
  s_battery_plugged = state.is_plugged;
  
  // State changed, mark the window for redraw.
  // if ( s_battery_level != state.charge_percent ) layer_mark_dirty(window_get_root_layer(s_window));
  layer_mark_dirty(window_get_root_layer(s_window));  
}

static void vibrate_on_demand(int mode) {
  if ( mode <= 0 ) return;
  if ( mode > 6 ) return;
  
  // 1-3 will only vibrate once, and never again until vibrate is reset back to 0.
  if ( mode == 1 && last_vibrate == 0 ) vibes_short_pulse();
  if ( mode == 2 && last_vibrate == 0 ) vibes_long_pulse();
  if ( mode == 3 && last_vibrate == 0 ) vibes_double_pulse();    
  // 4-5 will vibrate EVERY TIME the screeps payload arrives.
  if ( mode == 4 ) vibes_short_pulse();
  if ( mode == 5 ) vibes_long_pulse();
  if ( mode == 6 ) vibes_double_pulse();      
  
  // Drive backlight in sync.
  if ( mode <= 3 && last_vibrate == 0 ) light_enable_interaction();
  if ( mode > 3 ) light_enable_interaction();  
}

static bool is_blinking() {
  static int i = 0;
  for ( i = 0; i < 4; i++ ) {
    if ( blink[i] ) return true;
  }
  return false;
}

static int myAtoi(char *str) {
  static int i = 0;
  static char buf[64];
  snprintf(buf, sizeof(buf), "%s", str);
  return atoi(buf);
}

/*
 * We need to handle three major types of messages in here:
 *  Configuration, Weather, ScreepsData
 */
static void inbox_received_handler(DictionaryIterator *iter, void *context) {  
  static int i;
  static char buf[64];
  
  static bool was_blinking;
  was_blinking = is_blinking();
  
  firstData = true;
  
  // persist_write_bool(PKEY_RECEIVED, true);  
  APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_received_handler starting");
          
  Tuple *tupper = dict_read_first(iter);
  while (tupper) {    
    APP_LOG(APP_LOG_LEVEL_INFO, "tupper key: %lu = %s (%ld)", (unsigned long)tupper->key, tupper->value->cstring, tupper->value->int32);  
    
    if ( tupper->key == MESSAGE_KEY_ALERT_MISSING_CONFIG ) {
      snprintf(dynamicBuf[0], sizeof(dynamicBuf[0]), "%s", "Please Check Config");
      snprintf(dynamicBuf[1], sizeof(dynamicBuf[1]), "%s", "in App Settings");
      snprintf(dynamicBuf[2], sizeof(dynamicBuf[2]), "%s", "");
      snprintf(dynamicBuf[3], sizeof(dynamicBuf[3]), "%s", "Not Configured");     
      for ( i = 0; i < 4; i++ ) {
        bgColorOver[i] = GColorBlack;
        bgColorUnder[i] = GColorBlack;
        text_layer_set_text_color(s_dynamic_layer[i], GColorYellow);
        text_layer_set_font(s_dynamic_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
      }
      text_layer_set_font(s_dynamic_layer[3], fonts_get_system_font(FONT_KEY_GOTHIC_14));
      bgColorOver[3] = GColorBulgarianRose;
    }
    if ( tupper->key == MESSAGE_KEY_CONFIG_MAIL_SHOW ) {
      light_enable_interaction(); // Hack on new config push to light the screen.      
      persist_write_bool(PKEY_MAIL_SHOW, tupper->value->uint8);
    }
    if ( tupper->key == MESSAGE_KEY_CONFIG_POLL_SCREEPS ) {
      persist_write_int(PKEY_POLL_SCREEPS, myAtoi(tupper->value->cstring));
      poll_screeps = myAtoi(tupper->value->cstring);
    } 
    if ( tupper->key == MESSAGE_KEY_CONFIG_POLL_WEATHER ) {
      persist_write_int(PKEY_POLL_WEATHER, myAtoi(tupper->value->cstring));
      poll_weather = myAtoi(tupper->value->cstring);
    } 
    if ( tupper->key == MESSAGE_KEY_CONFIG_MAIL_RAIL ) persist_write_int(PKEY_MAIL_RAIL, myAtoi(tupper->value->cstring));
    if ( tupper->key == MESSAGE_KEY_CONFIG_BATTERY_RAIL ) persist_write_int(PKEY_BATTERY_RAIL, myAtoi(tupper->value->cstring));
    if ( tupper->key == MESSAGE_KEY_CONFIG_BATTERY_THRESHOLD ) persist_write_int(PKEY_BATTERY_THRESHOLD, myAtoi(tupper->value->cstring));
    if ( tupper->key == MESSAGE_KEY_CONFIG_BATTERY_MAIN ) persist_write_bool(PKEY_BATTERY_MAIN, tupper->value->uint8);
    if ( tupper->key == MESSAGE_KEY_CONFIG_BLUETOOTH_MAIN ) persist_write_bool(PKEY_BLUETOOTH_MAIN, tupper->value->uint8);

    if ( tupper->key == MESSAGE_KEY_CONFIG_BLUETOOTH_RECONNECT ) persist_write_int(PKEY_BT_RECONNECT, myAtoi(tupper->value->cstring));
    if ( tupper->key == MESSAGE_KEY_CONFIG_BLUETOOTH_DISCONNECT ) persist_write_int(PKEY_BT_DISCONNECT, myAtoi(tupper->value->cstring));
    if ( tupper->key == MESSAGE_KEY_CONFIG_WAKE_ON_CONNECT ) persist_write_bool(PKEY_WAKE_ON_CONNECT, tupper->value->uint8);
    if ( tupper->key == MESSAGE_KEY_CONFIG_WAKE_ON_DISCONNECT ) persist_write_bool(PKEY_WAKE_ON_DISCONNECT, tupper->value->uint8);
    if ( tupper->key == MESSAGE_KEY_CONFIG_USE_WEATHER ) persist_write_bool(PKEY_USE_WEATHER, tupper->value->uint8);
    if ( tupper->key == MESSAGE_KEY_WEATHER ) snprintf(weatherBuf, sizeof(weatherBuf), "%s", tupper->value->cstring);
    if ( tupper->key == MESSAGE_KEY_SCREEPS_MAIL ) mailCount = tupper->value->int32;    
    if ( tupper->key == MESSAGE_KEY_SCREEPS_VIBRATE ) {
      vibrate_on_demand(tupper->value->int32);
      last_vibrate = tupper->value->int32; 
    } 
    
    // Row data containing screeps data.
    for ( i = 0; i < 4; i++ ) {
      if ( tupper->key == (MESSAGE_KEY_SCREEPS_TEXT + i) ) {
        snprintf(dynamicBuf[i], sizeof(dynamicBuf[i]), "%s", tupper->value->cstring);
        requesting_screeps_data = false;
      }
      if ( tupper->key == (MESSAGE_KEY_SCREEPS_TEXT_COLOR + i) ) {
        textColorBuf[i] = GColorFromHEX(tupper->value->int32);        
        text_layer_set_text_color(s_dynamic_layer[i], textColorBuf[i]);
      }
      if ( tupper->key == (MESSAGE_KEY_SCREEPS_TEXT2_COLOR + i) ) textSecondColorBuf[i] = GColorFromHEX(tupper->value->int32);        
      if ( tupper->key == (MESSAGE_KEY_SCREEPS_OVER_COLOR + i) ) bgColorOver[i] = GColorFromHEX(tupper->value->int32);
      if ( tupper->key == (MESSAGE_KEY_SCREEPS_UNDER_COLOR + i) ) bgColorUnder[i] = GColorFromHEX(tupper->value->int32);
      if ( tupper->key == (MESSAGE_KEY_SCREEPS_PROGRESS + i) ) progressBuf[i] = tupper->value->int32;      
      if ( tupper->key == (MESSAGE_KEY_SCREEPS_BLINK + i) ) blink[i] = tupper->value->uint8;
      if ( tupper->key == (MESSAGE_KEY_SCREEPS_BOLD + i) ) {
        if ( tupper->value->uint8 ) {
          bold[i] = true;
          text_layer_set_font(s_dynamic_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
        } else {
          bold[i] = false;
          text_layer_set_font(s_dynamic_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_14));
        }
      }
      // requesting_screeps_data = false;
    }
    
    tupper = dict_read_next(iter);
  }
  
  if ( is_blinking() != was_blinking ) rebind_tick_handler();
  
  persist_display();
  apply_special_rails();  
  layer_mark_dirty(window_get_root_layer(s_window));
  
  APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_received_handler finished");
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void update_time() {
  APP_LOG(APP_LOG_LEVEL_INFO, "update_time()");
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  static char s_date_buffer[24];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %d %b, %Y", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_time_buffer);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void update_blink(struct tm *tick_time) {    
  blinking = ( tick_time->tm_sec % 2 == 0 ? true : false );
  
  static bool dirty = false;
  static int i = 0;
  for ( i = 0; i < 4; i++ ) {
    if ( !blink[i] ) continue;
    text_layer_set_text_color(s_dynamic_layer[i], (blinking ? textSecondColorBuf[i] : textColorBuf[i]));
    dirty = true;
  }
  
  // if ( s_battery_level <= 20 ) dirty = true;
  
  if ( dirty ) layer_mark_dirty(window_get_root_layer(s_window));
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "********** Running Tick Handler **********");
  
  if ( is_blinking() ) update_blink(tick_time);
  
  if ( units_changed & MINUTE_UNIT ) {
    update_time();
    
    // Automatic updates.
    if ( (poll_weather < 60) && (tick_time->tm_min % poll_weather) == 0 ) request_weather();
    if ( (poll_screeps < 60) && (tick_time->tm_min % poll_screeps) == 0 ) request_screeps_data();  
  }
  if ( units_changed & HOUR_UNIT ) {
    if ( poll_weather == 60 ) request_weather();
    if ( poll_screeps == 60 ) request_screeps_data();
  }
}

static void setup_text(TextLayer *layer, GFont font, GColor bg_color, GColor txt_color, GTextAlignment alignment, char *text) {
  text_layer_set_background_color(layer, bg_color);
  text_layer_set_text_color(layer, txt_color);
  text_layer_set_text(layer, text);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, alignment);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  const int yOff[4] = {0, 21, 125, 146};
  static int i;
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Redrawing canvas...");
  
  apply_special_rails();
  
  for ( i = 0; i < 4; i++ ) {
    // Set our line color and fill color.
    graphics_context_set_stroke_color(ctx, (blink[i] && blinking) ? bgColorOver[i] : bgColorUnder[i]);
    graphics_context_set_fill_color(ctx, (blink[i] && blinking) ? bgColorOver[i] : bgColorUnder[i]);
  
    // Draw our bottom layer.
    graphics_fill_rect(ctx, GRect(0, yOff[i], bounds.size.w, 20), 0, GCornersAll);

    // Determine the width of our top layer based on progress.
    int barWidth = bounds.size.w;
    if ( progressBuf[i] > 0 && progressBuf[i] < 99 ) 
      barWidth = (int)(((float)(bounds.size.w) / (float)(100)) * (float)(progressBuf[i]));
   
    // APP_LOG(APP_LOG_LEVEL_INFO, "width of %d is %d", i, barWidth);  
    
    // Set our line color and fill color.
    graphics_context_set_stroke_color(ctx, (blink[i] && blinking) ? bgColorUnder[i] : bgColorOver[i]);
    graphics_context_set_fill_color(ctx, (blink[i] && blinking) ? bgColorUnder[i] : bgColorOver[i]);
  
    // Draw our top layer.
    graphics_fill_rect(ctx, GRect(0, yOff[i], barWidth, 20), 0, GCornersAll);
  }  
  
  // Draw right-side battery meter
  if ( persist_read_bool(PKEY_BATTERY_MAIN) ) {
    static GColor bg;
    static int bH;
    if ( s_battery_charging || s_battery_plugged ) {
      bH = 74;
      bg = GColorCyan;
    } else {
      if ( s_battery_level <= 10 ) s_battery_level = 10;
      bH = (int)(((float)(74) / (float)(100)) * (float)(s_battery_level));
      bg = GColorGreen;
      if ( s_battery_level <= 40 ) bg = GColorChromeYellow;
      if ( s_battery_level <= 20 ) bg = GColorRed; // (blinking ? GColorRed : GColorWhite);  
    }
  
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(bounds.size.w - 8, 45, 6, 76), 0, GCornersAll);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(bounds.size.w - 7, 46, 4, 74), 0, GCornersAll);  
    graphics_context_set_stroke_color(ctx, bg);
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, GRect(bounds.size.w - 7, 46+(74-bH), 4, bH), 0, GCornersAll);
  }
  
  // Draw left-side bluetooth indicator.
  if ( persist_read_bool(PKEY_BLUETOOTH_MAIN) ) {
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(2, 45, 6, 76), 0, GCornersAll);
    graphics_context_set_stroke_color(ctx, s_bluetooth_connected ? GColorBlue : GColorBlack);
    graphics_context_set_fill_color(ctx, s_bluetooth_connected ? GColorBlue : GColorBlack);
    graphics_fill_rect(ctx, GRect(3, 46, 4, 74), 0, GCornersAll);
  }
  
  // layer_mark_dirty(window_get_root_layer(s_window));
}

static void window_load(Window *window) {  
  static int i;  
  
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  bounds = layer_get_bounds(window_layer);
  
  // bounds.origin.y += PBL_IF_RECT_ELSE(0, 50);
  
  // Prepare our buffers
  snprintf(mailBuf, sizeof(mailBuf), "%s", "0");
  snprintf(dynamicBuf[0], sizeof(dynamicBuf[0]), "%s", "");
  snprintf(dynamicBuf[1], sizeof(dynamicBuf[1]), "%s", "");
  snprintf(dynamicBuf[2], sizeof(dynamicBuf[2]), "%s", "");
  snprintf(dynamicBuf[3], sizeof(dynamicBuf[3]), "%s", "Connecting Phone");
  snprintf(weatherBuf, sizeof(weatherBuf), "%s", "Screeps.com");
  
  bgColorOver[0] = GColorBlack;
  bgColorOver[1] = GColorBlack;
  bgColorOver[2] = GColorBlack;
  bgColorOver[3] = GColorCobaltBlue;
  bgColorUnder[0] = GColorBlack;
  bgColorUnder[1] = GColorBlack;
  bgColorUnder[2] = GColorBlack;
  bgColorUnder[3] = GColorBlack;
  
  // Create our drawing canvas
  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, canvas_update_proc);
  layer_add_child(window_get_root_layer(window), s_canvas);
  layer_mark_dirty(s_canvas);  // Redraw us as soon as possible
  
  // Create our generic layer
  s_parts = layer_create(bounds);
  layer_add_child(window_get_root_layer(window), s_parts);
  layer_mark_dirty(s_parts);  // Redraw us as soon as possible
  
  // Create our bounded TextLayers, configure them and add them to the layer.
  s_time_layer = text_layer_create(GRect(0, 56, bounds.size.w, 50));
  setup_text(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS), GColorClear, GColorWhite, GTextAlignmentCenter, "--:--");
  layer_add_child(s_parts, text_layer_get_layer(s_time_layer));

  s_weather_layer = text_layer_create(GRect(0, 46, bounds.size.w, 50));
  setup_text(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorClear, GColorWhite, GTextAlignmentCenter, weatherBuf);
  layer_add_child(s_parts, text_layer_get_layer(s_weather_layer));

  s_date_layer = text_layer_create(GRect(0, 100, bounds.size.w, 50));
  setup_text(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorClear, GColorWhite, GTextAlignmentCenter, " ");
  layer_add_child(s_parts, text_layer_get_layer(s_date_layer));
  
  s_dynamic_layer[0] = text_layer_create(GRect(0, 0, bounds.size.w, 50));
  s_dynamic_layer[1] = text_layer_create(GRect(0, 21, bounds.size.w, 50));
  s_dynamic_layer[2] = text_layer_create(GRect(0, 125, bounds.size.w, 50));
  s_dynamic_layer[3] = text_layer_create(GRect(0, 146, bounds.size.w, 50));

  setup_text(s_dynamic_layer[0], fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorClear, GColorWhite, GTextAlignmentCenter, dynamicBuf[0]);
  setup_text(s_dynamic_layer[1], fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorClear, GColorWhite, GTextAlignmentCenter, dynamicBuf[1]);
  setup_text(s_dynamic_layer[2], fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorClear, GColorWhite, GTextAlignmentCenter, dynamicBuf[2]);
  setup_text(s_dynamic_layer[3], fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorClear, GColorWhite, GTextAlignmentCenter, dynamicBuf[3]);
  
  layer_add_child(s_parts, text_layer_get_layer(s_dynamic_layer[0]));  
  layer_add_child(s_parts, text_layer_get_layer(s_dynamic_layer[1]));  
  layer_add_child(s_parts, text_layer_get_layer(s_dynamic_layer[2]));  
  layer_add_child(s_parts, text_layer_get_layer(s_dynamic_layer[3]));  
  
  update_time();
  
  recover_display();
}

static void window_unload(Window *window) {  
  // Destroy our TextLayers.
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_date_layer);
  
  text_layer_destroy(s_dynamic_layer[0]);
  text_layer_destroy(s_dynamic_layer[1]);
  text_layer_destroy(s_dynamic_layer[2]);  
  text_layer_destroy(s_dynamic_layer[3]);  
  
  // Destroy our Parts and Canvas layers?
  layer_destroy(s_canvas);
  layer_destroy(s_parts);
}

static void rebind_tick_handler() {
  tick_timer_service_unsubscribe();
  if ( is_blinking() ) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Binding tick_timer_service under SECOND_UNIT");
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Binding tick_timer_service under MINUTE_UNIT");
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  }
}

static void init() {
  s_window = window_create();  
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});
	window_stack_push(s_window, true);

  poll_screeps = persist_read_int(PKEY_POLL_SCREEPS);
  poll_weather = persist_read_int(PKEY_POLL_WEATHER);
  if ( poll_screeps <= 0 ) poll_screeps = 1;
  if ( poll_weather <= 0 ) poll_screeps = 30;
  
  /*
  UnobstructedAreaHandlers handlers = {
		.change = unobstructed_change,
	};
	unobstructed_area_service_subscribe(handlers, NULL);
  */

  connection_service_subscribe((ConnectionHandlers) {
  		.pebble_app_connection_handler = bluetooth_callback
	});
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);  
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
	battery_state_service_subscribe(battery_callback);
	battery_callback(battery_state_service_peek());
  
  rebind_tick_handler();

  appReady = true;
}

static void deinit() {
  window_destroy(s_window);
}

int main(void) {
  mailCount = 0;
  appReady = false;
  firstData = false;
  
  init();
  app_event_loop();
  deinit();  
}