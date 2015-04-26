#include <pebble.h>

static Window *window;
static TextLayer *hello_layer;
static char msg[500];
static int mode = 0;
static int mode2 = 0;
static int mode3 = 0;
static int mode4 = 0;
static int number_of_mode = 2;
static int number_of_mode2 = 2;
static int number_of_mode3 = 2;
static int number_of_mode4 = 2;
static int pause = 0;
static int error_count = 0;
static int last_time = 0;
static int last_time2 = 0;
static int sleeptime = 10000; // number of milliseconds to sleep
static int ispolling = 0;



//static const int CELSIUS = 0;
//static const int FAHRENHEIT = 1;
static const int POLL  = 12;
static const int AVGC = 2;
static const int AVGF = 3; 
static const int TIMER = 6;
static const int RGB = 5;
static const int RESUME = 10;
static const int PAUSE = 11;

void resume();
void topause();
void send_req();
void rgb();
void polling();




static void timer_callback(void *data) { 
    text_layer_set_text_alignment(hello_layer, GTextAlignmentCenter);
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (ispolling) {
      polling();
      app_timer_register(sleeptime, timer_callback, NULL);
    } else {
      resume();
    }
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
   // outgoing message was delivered -- do nothing
   
}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  
   text_layer_set_text(hello_layer, "Error out!");

}

// Handles all messages received from server
void in_received_handler(DictionaryIterator *received, void *context) {
   // incoming message received 
  
   // looks for key #0 in the incoming message
   int key = 0;
   Tuple *text_tuple = dict_find(received, key);
   if (text_tuple) {
     if (text_tuple->value) {
       // put it in this global variable
       strcpy(msg, text_tuple->value->cstring);
     }
     else strcpy(msg, "no value!");
     if (msg[0] == 'A' || msg[0] == 'P') {
       int i = 0;
       while (msg[i] != '\0') {
         if (msg[i] == '#') {
           msg[i] = '\n';
         }
         i++;
       }
     }
     text_layer_set_text(hello_layer, msg);
     if (msg[0] == 'A') {
       text_layer_set_font(hello_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
     } else {
       text_layer_set_font(hello_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
     }
   }
   else {
     text_layer_set_text(hello_layer, "no message!");
   }
 }


void in_dropped_handler(AppMessageResult reason, void *context) {
   // incoming message dropped
   text_layer_set_text(hello_layer, "Error in!");
}

// send request to the server 
void send_req() {
  if (pause) return;
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  int key;
  key = mode;
  last_time = key;
  last_time2 = key;
  // send the message "hello?" to the phone, using key #0
  Tuplet value = TupletCString(key, "msg");
  dict_write_tuplet(iter, &value);
  app_message_outbox_send();
}

/* This is called when the select button is clicked */
void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // text_layer_set_text(hello_layer, "Selected");
  if (pause) return;
  text_layer_set_text_alignment(hello_layer, GTextAlignmentCenter);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  mode++;
  mode = mode % number_of_mode;
  //last_time = 0;
  send_req();
}

/* This called when select is long pressed. */
void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    int key = 0;
    if (last_time2 == 0) {
      key = AVGC;
    } else {
      key = AVGF;
    }
    last_time = key;
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    Tuplet value = TupletCString(key, "msg");
    dict_write_tuplet(iter, &value);
    app_message_outbox_send();
}


//switch to the resume mode
void resume() {
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = RESUME;
   pause = 0;
   last_time = key;
   Tuplet value = TupletCString(key, "msg");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}

// switch to the pause mode
void topause() {
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = PAUSE;
   pause = 1;
   last_time = key;
   Tuplet value = TupletCString(key, "msg");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}

/* This is called when the up button is clicked */
void up_click_handler(ClickRecognizerRef recognizer, void *context) {
//    resume();
//   if (pause) return;
  text_layer_set_text_alignment(hello_layer, GTextAlignmentCenter);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  mode2++;
  mode2 = mode2 % number_of_mode2;
  if (mode2 == 1) {
    topause();
  } 
  else {
    resume();
  }
}

// switch to the polling mode
void polling() {
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = POLL;
   pause = 0;
   last_time = key;
   Tuplet value = TupletCString(key, "msg");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}

/* This called when the up button is long pressed. */
void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text_alignment(hello_layer, GTextAlignmentCenter);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  mode3++;
  mode3 = mode3 % number_of_mode3;
  if (mode3 == 1) {
    ispolling = 1;
    polling();
    app_timer_register(sleeptime, timer_callback, NULL);
  } 
  else {
    ispolling = 0;
  }
}

// /* This called when the up button is double clicked. */
// void up_double_click_handler(ClickRecognizerRef recognizer, void *context) {
//     int key = last_time;
//     DictionaryIterator *iter;
//     app_message_outbox_begin(&iter);
//     Tuplet value = TupletCString(key, "msg");
//     dict_write_tuplet(iter, &value);
//     app_message_outbox_send();
// }


//switch to the RGB light mode
void rgb() {
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = RGB;
   pause = 0;
   last_time = key;
   Tuplet value = TupletCString(key, "msg");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}

/* This is called when the down button is clicked */
void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (pause) return;
    text_layer_set_text_alignment(hello_layer, GTextAlignmentCenter);
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    rgb();
}

/* This called when the down button is long pressed. */
void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    mode4++;
    mode4 = mode4 % number_of_mode4;
    int key = TIMER;
    if (mode4 == 1) key = TIMER;
    else key = RESUME;
    last_time = key;
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    Tuplet value = TupletCString(key, "msg");
    dict_write_tuplet(iter, &value);
    app_message_outbox_send();
}

// /* This called when the down button is double clicked. */
// void down_double_click_handler(ClickRecognizerRef recognizer, void *context) {
//     int key = last_time;
//     DictionaryIterator *iter;
//     app_message_outbox_begin(&iter);
//     Tuplet value = TupletCString(key, "msg");
//     dict_write_tuplet(iter, &value);
//     app_message_outbox_send();
// }

/* this registers the appropriate function to the appropriate button */
void config_provider(void *context) {
   window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
   window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
   window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
   window_long_click_subscribe(BUTTON_ID_SELECT, 700, NULL, select_long_click_handler);
   window_long_click_subscribe(BUTTON_ID_UP, 700, NULL, up_long_click_handler);
   window_long_click_subscribe(BUTTON_ID_DOWN, 700, NULL, down_long_click_handler);
}


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  hello_layer = text_layer_create((GRect) { .origin = { 0, 40 }, .size = { bounds.size.w, 80 } });
  text_layer_set_text(hello_layer, "Welcome! Press the button and have fun!");
  text_layer_set_font(hello_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(hello_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(hello_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(hello_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(hello_layer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  
  // need this for adding the listener
  window_set_click_config_provider(window, config_provider);
  
  
  // for registering AppMessage handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
  
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

