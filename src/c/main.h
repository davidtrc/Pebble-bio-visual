#include <pebble.h>
/*
static char* smartstrap_result_to_string(SmartstrapResult result);

static void strap_notify_handler(SmartstrapAttribute *attribute);

static void strap_read_handler(SmartstrapAttribute *attribute, SmartstrapResult result, const uint8_t *data, size_t length);

static void strap_write_handler(SmartstrapAttribute *attribute, SmartstrapResult result);

static void strap_availability_handler(SmartstrapServiceId service_id, bool is_available); 

static void getwifiaps(void);

static void wifilist_window_load(Window *window); 
  
static void wifilist_up_single_click(ClickRecognizerRef recognizer, void *context);

static void wifilist_window_unload(Window *window); 

static void wifilist_click_config_provider(void *context); 

static void connect_valoriza(int index, void *ctx); 

static void menu_select_callback(int index, void *ctx);
*/

void choose_ap(int index, void *ctx); 

/*

static void list_window_load(Window *window); 

static void list_window_unload(Window *window);
  
static void main_up_single_click(ClickRecognizerRef recognizer, void *context);

static void main_click_config_provider(void *context); 

static void update_time();
  
static void tick_handler(struct tm *tick_time, TimeUnits units_changed); 

static void main_window_load(Window *window);

static void main_window_unload(Window *window);

static void init(); 

static void deinit();
*/

int main(void); 

void choose_ap(int index, void *ctx);