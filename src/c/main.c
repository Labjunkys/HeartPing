#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_title_layer;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_heart_layer;
static HealthValue last_value;
static int other_heartrate;
static int timeout;

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  
  // Create temperature Layer
  s_weather_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(130, 134), bounds.size.w, 25));
  
  // Style the text
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorBlack);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_weather_layer, "Loading...");
  
  // Create temperature Layer
  s_heart_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(56, 50), bounds.size.w, 50));
  
  // Style the text
  text_layer_set_background_color(s_heart_layer, GColorClear);
  text_layer_set_text_color(s_heart_layer, GColorBlack);
  text_layer_set_text_alignment(s_heart_layer, GTextAlignmentCenter);
  text_layer_set_font(s_heart_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(s_heart_layer, "Loading...");
  

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(98, 92), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Create the TextLayer with specific bounds
  s_title_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(8, 2), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text_color(s_title_layer, GColorBlack);
  text_layer_set_text(s_title_layer, "Jebediah Industries");
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_title_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_heart_layer));
}


static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_heart_layer);
}


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 5 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }
  
}

static void send_heartrate(HealthValue value) {
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  // Add a key-value pair
  dict_write_uint8(iter, 1, (uint8_t) value);

  // Send the message!
  app_message_outbox_send();
}



static void update_heart() {
  HealthServiceAccessibilityMask hr = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL));
  if (hr & HealthServiceAccessibilityMaskAvailable) {
    HealthValue val = health_service_peek_current_value(HealthMetricHeartRateRawBPM);
    //if(val > 0) {
      // Display HRM value
      static char s_hrm_buffer[8];
      snprintf(s_hrm_buffer, sizeof(s_hrm_buffer), "%lu BPM", (u_long)val);
      text_layer_set_text(s_heart_layer, s_hrm_buffer);
    //}
    if (val != last_value || true) {
      last_value = val;
      send_heartrate(val);
    }
  }
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}


static void tap_handler(AccelAxisType axis, int32_t direction) {
  update_heart();
}


static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  
  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
  
  static char heartrate_buffer[8];
  
  Tuple *heartrate_tuple = dict_find(iterator, MESSAGE_KEY_HEARTRATE);
  
  if(heartrate_tuple) {
    other_heartrate = (int)heartrate_tuple->value->int32;
    timeout = 6000;
    snprintf(heartrate_buffer, sizeof(heartrate_buffer), "%d", (int)heartrate_tuple->value->int32);
    text_layer_set_text(s_title_layer, heartrate_buffer);
  }
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

#define MAX_PWM_DURATION 1000 // Longest duration (in ms). Bigger makes larger array
// Pulse-Width Modulation Vibration
// strength: pwm ranges from 0 - 10, <0 is fully off, >10 is fully on
// duration: duration in milliseconds (must be > 0)
void vibes_pwm(int32_t strength, uint32_t duration) {
  uint32_t pwm_segments[MAX_PWM_DURATION/7], pwm_seglen;
  if(strength < 0) {
    vibes_cancel();
  } else if(strength > 10) {
    pwm_segments[0] = duration;  // on for [duration] (in ms)
    VibePattern pwm_pat = {.durations = pwm_segments, .num_segments = 1};
    vibes_enqueue_custom_pattern(pwm_pat);
  } else {
    if(duration > MAX_PWM_DURATION) duration = MAX_PWM_DURATION;
    duration /= 7;  // Should be 5. Trial&Error found 7
    for(pwm_seglen = 0; pwm_seglen < duration; pwm_seglen++) {
      pwm_segments[pwm_seglen] = strength;      // on duration (in ms)
      pwm_segments[++pwm_seglen] = 10 - strength; // off duration (in ms)
     }
    VibePattern pwm_pat = {.durations = pwm_segments, .num_segments = pwm_seglen};
    vibes_enqueue_custom_pattern(pwm_pat);
  }
}

static void get_next_pulse() {

  int rate;  
  if (other_heartrate == 0 || timeout <= 0) {
    rate = 60;
  } else {
    vibes_pwm(3, 300);
    //vibes_double_pulse();
    rate = other_heartrate;
  }
  uint32_t timeout_ms = (60 * 1000 / rate);
  if (timeout > 0) {
    timeout -= timeout_ms;
  }
  app_timer_register(timeout_ms, get_next_pulse, (void*)0);
}
  
  
static void init() {
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  accel_tap_service_subscribe(tap_handler);
  
  get_next_pulse();
  last_value = 0;
  timeout = 0;
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  // Make sure the time is displayed from the start
  update_time();
  update_heart();
}


static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

