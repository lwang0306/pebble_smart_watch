#include <pebble.h>

static Window *window;
static TextLayer *hello_layer;
static char msg[500];
static int mode = 0;
static int number_of_mode = 2;
static int pause = 0;
static int error_count = 0;
static int last_time = 0;


//static const int CELSIUS = 0;
//static const int FAHRENHEIT = 1;
static const int RESUME = 10;
static const int PAUSE = 11;

void resume();
void topause();
void send_req();

void out_sent_handler(DictionaryIterator *sent, void *context) {
   // outgoing message was delivered -- do nothing
}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
   // outgoing message failed
   if (error_count > 10) {
     error_count = 0;
     text_layer_set_text(hello_layer, "Error out!");
   } else {
     error_count++;
     if (last_time == 10)
       resume();
     if (last_time == 11)
       topause();
     if (last_time != 10 && last_time != 11)
       send_req();
   }   
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
     
     text_layer_set_text(hello_layer, msg);
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

//exists the pause mode
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

void up_click_handler(ClickRecognizerRef recognizer, void *context) {
   resume();
}

void down_click_handler(ClickRecognizerRef recognizer, void *context) {
   topause();
}


/* this registers the appropriate function to the appropriate button */
void config_provider(void *context) {
   window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
   window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
   window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
}


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  hello_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 80 } });
  text_layer_set_text(hello_layer, "Welcome! Press the select button to choose mode");
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
