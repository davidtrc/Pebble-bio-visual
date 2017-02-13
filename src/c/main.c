///////////////////////////////////////////////////////////
// TO-DO LIST
// Comprobar siempre que se quiera hablar con el ESP32 que este está conectado
// si no lo está avisar y cambiar la frase de DETECTADo en el main
// Al arrancar la aplicacion intentar conectarse al ESP32
// Añadir persistencia a la aplicación, que cuando se cierre no se pierdan algunas variables o estados
// Mostrar una ventana con la nueva contraseña cada vez que se cambie
//
///////////////////////////////////////////////////////////
#include <pebble.h>
#include "src/c/main.h"
#include "src/c/main_functions.h"
#include "src/c/T3Window.h"

#define DEBUG
#define ACCEL_STEP_MS 50

#define NUM_OPTIONS_SECTIONS 1
#define NUM_OPTIONS_ITEMS 6

#define NUM_WIFIAP_SECTIONS 1
#define NUM_WIFIAP_ITEMS 15
#define MAX_AP_SSID 15

#define NO_OF_FRAMES 2
#define NO_OF_FRAMES_WIFI 3

static Window *s_main_window;
static Layer *main_window_layer;
static TextLayer *s_time_layer;
static TextLayer *s_textvaloriza_layer;
static TextLayer *s_textespmart105_layer;

static Window *s_list_window;
Layer *options_layer;
static SimpleMenuLayer *s_options_layer;
static SimpleMenuSection s_options_sections[NUM_OPTIONS_SECTIONS];
static SimpleMenuItem s_options_items[NUM_OPTIONS_ITEMS];

Window *s_wifilist_window;
Layer *wifilist_layer;
SimpleMenuLayer *s_wifiap_layer;
static TextLayer *s_gettingwifi_layer;
SimpleMenuItem s_wifiap_items[7];
SimpleMenuSection s_wifiap_sections[1];
static GBitmap *wifi_icon;
static BitmapLayer *wifi_icon_layer;
static AppTimer *wifi_timer;
const int wifi_gif[] = {  
  RESOURCE_ID_WIFI1,
  RESOURCE_ID_WIFI2,
  RESOURCE_ID_WIFI3
};
extern int apnumber;
extern char *apssid [MAX_AP_SSID];
extern int apnumber;
extern const char* chosenAP;
static char default_password[33];
static char custom_password[33];

static Window *pass_window;
static Layer *pass_window_layer;
static TextLayer *set_pass_layer;
static TextLayer *use_default_pass_layer;

static Window *confirm_window;
static Layer *confirm_window_layer;
static TextLayer *chosenAP1_layer;
static TextLayer *chosenAP2_layer;
static TextLayer *chosenPass1_layer;
static TextLayer *chosenPass2_layer;
static TextLayer *iscorrect_layer;
static TextLayer *confirm_layer;
static TextLayer *abort_layer;

static Window *connecting_window;
static Layer *connecting_window_layer;
static TextLayer *connecting_layer;
static AppTimer *connecting_timer;

static Window *heartrate_window;
static Layer *heartrate_layer;
static TextLayer *infobpm_layer;
static TextLayer *bpm_layer;
static TextLayer *ppm_layer;
static GBitmap *heart_small;
static BitmapLayer *heart_small_layer;
static GBitmap *heart_big;
static BitmapLayer *heart_big_layer;

static Window *temperature_window;
static Layer *temperature_layer;
static TextLayer *infotemperature_textlayer;
static TextLayer *temp_textlayer;
static TextLayer *degrees_textlayer;
static GBitmap *term_icon;
static BitmapLayer *term_icon_layer;
static AppTimer *term_timer;
const int temp_gif[] = {  
  RESOURCE_ID_TERM_BIG,
  RESOURCE_ID_TERM_SMALL,
};
int frame_no = 0;
bool temperature_received = false;
char last_temperature[5];

static T3Window * myT3Window; //T3 keyboard Window
const char LAYOUT_NUMBERS[] =
    "123\0"  "456\0"  "789\0"
    "0{}\0"  "|~`\0"  "[\\]\0"
    "^_!\0"  "\"#$\0"  "%&'";

const char LAYOUT_NUMBERS2[] =
    "()*\0"  "+,-\0"  "./ \0"
    ":  \0"  ";  \0"  "<  \0"
    "=  \0"  ">  \0"  "?  ";

const char * keyboardSet1[] = {T3_LAYOUT_LOWERCASE, T3_LAYOUT_UPPERCASE};
const char * keyboardSet2[] = {LAYOUT_NUMBERS, LAYOUT_NUMBERS2};
const char * keyboardSet3[] = {T3_LAYOUT_BRACKETS, T3_LAYOUT_PUNC};

static const SmartstrapServiceId raw_service_id = SMARTSTRAP_RAW_DATA_SERVICE_ID;
static const SmartstrapAttributeId raw_attribute_id = SMARTSTRAP_RAW_DATA_ATTRIBUTE_ID;
static const int raw_attribute_buffer_length = 36*16; //32 is the max byte size of an SSID + 4 separating them in the received char* * (number of AP received + security hoverhead)
static uint8_t *raw_buffer;

static SmartstrapAttribute *raw_attribute;
bool apsready = false;
bool wificonnected = false;
bool esp_connected = false;
HealthValue heartRateMeasured;

static time_t hour1; //Used to measure time differences
static time_t hour2;
static int difference; //Used to measure time differences

static char* smartstrap_result_to_string(SmartstrapResult result) {
  switch(result) {
    case SmartstrapResultOk:
      return "SmartstrapResultOk";
    case SmartstrapResultInvalidArgs:
      return "SmartstrapResultInvalidArgs";
    case SmartstrapResultNotPresent:
      return "SmartstrapResultNotPresent";
    case SmartstrapResultBusy:
      return "SmartstrapResultBusy";
    case SmartstrapResultServiceUnavailable:
      return "SmartstrapResultServiceUnavailable";
    case SmartstrapResultAttributeUnsupported:
      return "SmartstrapResultAttributeUnsupported";
    case SmartstrapResultTimeOut:
      return "SmartstrapResultTimeOut";
    default:
      return "Not a SmartstrapResult value!";
  }
}

static void strap_notify_handler(SmartstrapAttribute *attribute) {
  //se está haciendo todo el procesado del paquete recibido en la ISR. Sería más óptimo crear una variable bool que avise de que ha llegado algo por parte del ESP32
  // y ya procesar el paquete en la parte que proceda del código.
  int operation;
  // The smartstrap has emitted a notification for this attribute
  APP_LOG(APP_LOG_LEVEL_INFO, "Attribute with ID %d sent notification", (int)smartstrap_attribute_get_attribute_id(attribute));

  // Some data is ready, let's read it
  smartstrap_attribute_read(attribute);
  operation = decodedata(raw_buffer);
  if(operation == 0){ APP_LOG(APP_LOG_LEVEL_INFO, "OP. received = 0");}
  if( operation == 50 && apsready == true){
    printAPS(apnumber);
  }
}

static void strap_read_handler(SmartstrapAttribute *attribute, SmartstrapResult result, const uint8_t *data, size_t length) {
  if(result == SmartstrapResultOk) {
    // Data has been read into the data buffer provided
    APP_LOG(APP_LOG_LEVEL_INFO, "Smartstrap sent: %s", (char*)data);
    decodedata(raw_buffer);
  } else {
    // Some error has occured
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error in read handler: %d", (int)result);
  }
}

/*
static void read_attribute() {
  SmartstrapResult result = smartstrap_attribute_read(raw_attribute);
  if(result != SmartstrapResultOk) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error reading attribute: %s", smartstrap_result_to_string(result));
  }
}
*/

static void strap_write_handler(SmartstrapAttribute *attribute, SmartstrapResult result) { //Called when Pebble ends the data sending
  // A write operation has been attempted
  if(result != SmartstrapResultOk) {
    // Handle the failure
    APP_LOG(APP_LOG_LEVEL_ERROR, "Smartstrap error occured: %s", smartstrap_result_to_string(result));
  }
}

static void strap_availability_handler(SmartstrapServiceId service_id, bool is_available) {
  // A service's availability has changed
  APP_LOG(APP_LOG_LEVEL_INFO, "Service %d is %s available", (int)service_id, is_available ? "now" : "NOT");
}

static void uart_init(void){
 // Create the attribute, and allocate a buffer for its data
  raw_attribute = smartstrap_attribute_create(raw_service_id, raw_attribute_id, raw_attribute_buffer_length);
  
  // Subscribe to the smartstrap events
  smartstrap_subscribe((SmartstrapHandlers) {
    .availability_did_change = strap_availability_handler,
    .did_read = strap_read_handler,
    .did_write = strap_write_handler, //Call when the write ends 
    .notified = strap_notify_handler
  });
  smartstrap_set_timeout(1000);
  
}

static void wifilist_window_load(Window *window) {
  
  wifilist_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(wifilist_layer);
  
  #ifdef DEBUG
    esp_connected = true;
  #endif

  s_gettingwifi_layer = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  
  text_layer_set_background_color(s_gettingwifi_layer, GColorClear);
  text_layer_set_text_color(s_gettingwifi_layer, GColorBlack);
  if(esp_connected){
    text_layer_set_text(s_gettingwifi_layer, "Pulse el botón 'arriba' para obtener los puntos de acceso. Puede tardar hasta 15 segundos.");
    
    wifi_icon_layer = bitmap_layer_create(GRect(0, 70, bounds.size.w, 70));
    bitmap_layer_set_compositing_mode(wifi_icon_layer, GCompOpSet);
  
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(wifi_icon_layer));
  }
  else{
    text_layer_set_text(s_gettingwifi_layer, "eSpMART105 no detectado. Conecte primero eSpMART105.");
  }

  text_layer_set_font(s_gettingwifi_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_gettingwifi_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_gettingwifi_layer, GTextOverflowModeWordWrap);
  
  
  layer_add_child(wifilist_layer, text_layer_get_layer(s_gettingwifi_layer));
}

static void silly_function(void *context) {}

static void wifi_handler(void *context) {
  
  if(frame_no == NO_OF_FRAMES_WIFI){
    frame_no = 0;
  }
  
  if(frame_no < NO_OF_FRAMES_WIFI) {
    
    if (wifi_icon != NULL) {
      gbitmap_destroy(wifi_icon);
      wifi_icon = NULL;
    }
    
    wifi_icon = gbitmap_create_with_resource(wifi_gif[frame_no]);
    
    bitmap_layer_set_bitmap(wifi_icon_layer, wifi_icon);
    layer_mark_dirty(bitmap_layer_get_layer(wifi_icon_layer));

    frame_no++;
    wifi_timer = app_timer_register(750, wifi_handler, NULL);
  }
  //Start counting time (5 s)
  hour2 = time(NULL);
  
  difference = hour2-hour1;
  #ifdef DEBUG
    if(difference == 3){
      //gbitmap_destroy(wifi_icon);
      app_timer_cancel(wifi_timer);
      printAPS(5); //printAPS(apnumber);
    }
  #endif
  
  //Start counting time (14 s)
  if(difference >=5){  //davidtrc ESTO ES 14 EN LA VIDA REAL
    text_layer_set_text(s_gettingwifi_layer, "No ha sido posible obtener los puntos de acceso");
    app_timer_cancel(wifi_timer);
    wifi_icon = gbitmap_create_with_resource(RESOURCE_ID_WIFI_COMPLETE);
    bitmap_layer_set_bitmap(wifi_icon_layer, wifi_icon);
    layer_mark_dirty(bitmap_layer_get_layer(wifi_icon_layer));
  } else {
    if(apsready){
      printAPS(apnumber);
      app_timer_cancel(wifi_timer);
      difference = 11;
    }
  }
  
}

static void wifilist_up_single_click(ClickRecognizerRef recognizer, void *context) {
  if(esp_connected){
    window_set_click_config_provider(s_wifilist_window, silly_function);
    layer_mark_dirty(wifilist_layer);
    text_layer_set_text(s_gettingwifi_layer, "Buscando... Espere");
    uint32_t first_delay_ms = 1;
    frame_no = 0;
    wifi_timer = app_timer_register(first_delay_ms, wifi_handler, NULL);
    hour1 = time(NULL);
    difference = 0;
    apsready = false;
    //davidtrc resetear los wifis almacenados... ¿desconectar del wifi al que esté conectado?
    size_t buff_size = 0;
    //Request the AP list (petition 50 to ESP32)
    smartstrap_attribute_begin_write(raw_attribute, &raw_buffer, &buff_size);
    snprintf((char*)raw_buffer, sizeof(char), "2"); //2 in ASCII is 50 in natural numbers
    smartstrap_attribute_end_write(raw_attribute, buff_size, false);
  }
}

static void wifilist_window_unload(Window *window) {
  //simple_menu_layer_destroy(s_wifiap_layer);
  //text_layer_destroy(s_gettingwifi_layer);
  app_timer_cancel(wifi_timer);
  //gbitmap_destroy(wifi_icon);
  //wifi_icon = NULL;
}

static void wifilist_click_config_provider(void *context) {
  ButtonId idup = BUTTON_ID_UP;  // The Select button
  //ButtonId idback = BUTTON_ID_BACK;  // The Select button
  window_single_click_subscribe(idup, wifilist_up_single_click);

}

static void connect_valoriza(int index, void *ctx) {
  
  s_wifilist_window = window_create();
  window_set_click_config_provider(s_wifilist_window, wifilist_click_config_provider); //Alow the use of buttons
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_wifilist_window, (WindowHandlers) {
    .load = wifilist_window_load,
    .unload = wifilist_window_unload
  });
  
  window_stack_push(s_wifilist_window, true);

}

static void connecting_handler(void *context) {
  
  if(frame_no == NO_OF_FRAMES_WIFI){
    frame_no = 0;
  }
  
  if(frame_no < NO_OF_FRAMES_WIFI) {
    
    if (wifi_icon != NULL) {
      gbitmap_destroy(wifi_icon);
      wifi_icon = NULL;
    }
    
    wifi_icon = gbitmap_create_with_resource(wifi_gif[frame_no]);
    
    bitmap_layer_set_bitmap(wifi_icon_layer, wifi_icon);
    layer_mark_dirty(bitmap_layer_get_layer(wifi_icon_layer));

    frame_no++;
    connecting_timer = app_timer_register(750, connecting_handler, NULL);
  }
  
  //Start counting time (10 s)
  hour2 = time(NULL);
  difference = hour2-hour1;
  
  #ifdef DEBUG
    if(difference == 2){
      wificonnected = true;
      app_timer_cancel(wifi_timer);
    }
  #endif
  
  if (difference >3 && !wificonnected){  //davidtrc ESTO ES 10 EN LA VIDA REAL
    text_layer_set_text(s_textvaloriza_layer, "No conectado a la red de Valoriza");
    text_layer_set_text(connecting_layer, "No se ha realizado la conexión");
    app_timer_cancel(connecting_timer);
    wifi_icon = gbitmap_create_with_resource(RESOURCE_ID_WIFI_COMPLETE);
    bitmap_layer_set_bitmap(wifi_icon_layer, wifi_icon);
    layer_mark_dirty(bitmap_layer_get_layer(wifi_icon_layer));
    layer_mark_dirty(connecting_window_layer);
  }
  
  if(wificonnected){
    text_layer_set_text(s_textvaloriza_layer, "CONECTADO a la red de Valoriza");
    text_layer_set_text(connecting_layer, "Se ha realizado la conexión");
    app_timer_cancel(connecting_timer);
    wifi_icon = gbitmap_create_with_resource(RESOURCE_ID_WIFI_NOT_SAD);
    bitmap_layer_set_bitmap(wifi_icon_layer, wifi_icon);
    layer_mark_dirty(bitmap_layer_get_layer(wifi_icon_layer));
    layer_mark_dirty(connecting_window_layer);
    difference = 11;
  }
  
}

static void connecting_window_load(Window *window){
  
  connecting_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(connecting_window_layer);
  
  connecting_layer = text_layer_create(GRect(0, 5, bounds.size.w, 40));
  
  text_layer_set_background_color(connecting_layer, GColorClear);
  text_layer_set_text_color(connecting_layer, GColorBlack);
  text_layer_set_text(connecting_layer, "Conectando... Espere");
  text_layer_set_font(connecting_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(connecting_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(connecting_layer, GTextOverflowModeWordWrap);
  
  wifi_icon_layer = bitmap_layer_create(GRect(0, 70, bounds.size.w, 70));
  bitmap_layer_set_compositing_mode(wifi_icon_layer, GCompOpSet);
  
  layer_add_child(connecting_window_layer, text_layer_get_layer(connecting_layer));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(wifi_icon_layer));
  
  size_t buff_size;
  uint8_t *temp_buffer = malloc(102*sizeof(uint8_t));
  uint8_t *ap_lg = malloc(3*sizeof(uint8_t));
  uint8_t *pass_lg = malloc(3*sizeof(uint8_t));
  if(temp_buffer == NULL || ap_lg == NULL || pass_lg == NULL){
     APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", temp_buffer);
  }
  snprintf( (char*)ap_lg, 4, "%.2d", (int) strlen(chosenAP));
  snprintf( (char*)pass_lg, 4, "%.2d", (int) strlen(custom_password));
  
  smartstrap_attribute_begin_write(raw_attribute, &temp_buffer, &buff_size);
  strcpy((char*)temp_buffer, "7"); //7 in ASCII is 55 in natural numbers
  strcat((char*)temp_buffer, (char*) ap_lg);
  strcat((char*)temp_buffer, (char*) pass_lg);
  strcat((char*)temp_buffer, (char*) chosenAP);
  strcat((char*)temp_buffer, (char*) custom_password);
  smartstrap_attribute_end_write(raw_attribute, buff_size, false);
  free(temp_buffer);
  free(ap_lg);
  free(pass_lg);
  
  uint32_t first_delay_ms = 1;
  frame_no = 0;
  connecting_timer = app_timer_register(first_delay_ms, connecting_handler, NULL);
  hour1 = time(NULL);
  difference = 0;
  
}

static void connecting_window_unload(Window *window){
  text_layer_destroy(connecting_layer);
  window_stack_remove(connecting_window, true);
  window_stack_remove(confirm_window, true);
  window_stack_remove(pass_window, true);
  window_stack_remove(s_wifilist_window, true);
  window_stack_remove(s_list_window, true);
  window_stack_push(s_main_window, true);
  
}
  
static void confirm_window_load(Window *window){
  
  // Get information about the Window
  confirm_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(confirm_window_layer);
  
  // Create the TextLayer with specific bounds
  chosenAP1_layer = text_layer_create(GRect(0, 5, bounds.size.w-30, 20));
  chosenAP2_layer = text_layer_create(GRect(0, 25, bounds.size.w-30, 40));
  chosenPass1_layer = text_layer_create(GRect(0, 70, bounds.size.w-30, 20));
  chosenPass2_layer = text_layer_create(GRect(0, 90, bounds.size.w-30, 40));
  iscorrect_layer = text_layer_create(GRect(0, 140, bounds.size.w-30, 20));
  confirm_layer = text_layer_create(GRect(bounds.size.w-30, 10, 40, 50));
  abort_layer = text_layer_create(GRect(bounds.size.w-30, bounds.size.h-40, 40, 50));

  text_layer_set_background_color(chosenAP1_layer, GColorClear);
  text_layer_set_text_color(chosenAP1_layer, GColorBlack);
  text_layer_set_text(chosenAP1_layer, "Punto de acceso:");
  text_layer_set_font(chosenAP1_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(chosenAP1_layer, GTextAlignmentCenter);
  
  text_layer_set_background_color(chosenAP2_layer, GColorClear);
  text_layer_set_text_color(chosenAP2_layer, GColorBlack);
  text_layer_set_text(chosenAP2_layer, chosenAP);
  text_layer_set_font(chosenAP2_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(chosenAP2_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(chosenAP2_layer, GTextOverflowModeWordWrap);
  
  text_layer_set_background_color(chosenPass1_layer, GColorClear);
  text_layer_set_text_color(chosenPass1_layer, GColorBlack);
  text_layer_set_text(chosenPass1_layer, "Contraseña:");
  text_layer_set_font(chosenPass1_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(chosenPass1_layer, GTextAlignmentCenter);
  
  text_layer_set_background_color(chosenPass2_layer, GColorClear);
  text_layer_set_text_color(chosenPass2_layer, GColorBlack);
  text_layer_set_text(chosenPass2_layer, custom_password);
  text_layer_set_font(chosenPass2_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(chosenPass2_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(chosenPass2_layer, GTextOverflowModeWordWrap);
  
  text_layer_set_background_color(iscorrect_layer, GColorClear);
  text_layer_set_text_color(iscorrect_layer, GColorBlack);
  text_layer_set_text(iscorrect_layer, "¿Es correcto?");
  text_layer_set_font(iscorrect_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(iscorrect_layer, GTextAlignmentCenter);
  
  text_layer_set_background_color(confirm_layer, GColorClear);
  text_layer_set_text_color(confirm_layer, GColorBlack);
  text_layer_set_text(confirm_layer, "Si");
  text_layer_set_font(confirm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(confirm_layer, GTextAlignmentCenter);
  
  text_layer_set_background_color(abort_layer, GColorClear);
  text_layer_set_text_color(abort_layer, GColorBlack);
  text_layer_set_text(abort_layer, "No");
  text_layer_set_font(abort_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(abort_layer, GTextAlignmentCenter);
  
  layer_add_child(confirm_window_layer, text_layer_get_layer(chosenAP1_layer));
  layer_add_child(confirm_window_layer, text_layer_get_layer(chosenAP2_layer));
  layer_add_child(confirm_window_layer, text_layer_get_layer(chosenPass1_layer));
  layer_add_child(confirm_window_layer, text_layer_get_layer(chosenPass2_layer));
  layer_add_child(confirm_window_layer, text_layer_get_layer(iscorrect_layer));
  layer_add_child(confirm_window_layer, text_layer_get_layer(confirm_layer));
  layer_add_child(confirm_window_layer, text_layer_get_layer(abort_layer));
}

static void confirm_window_unload(Window *window){
  
  text_layer_destroy(chosenAP1_layer);
  text_layer_destroy(chosenAP2_layer);
  text_layer_destroy(chosenPass1_layer);
  text_layer_destroy(chosenPass2_layer);
  text_layer_destroy(iscorrect_layer);
  text_layer_destroy(confirm_layer);
  text_layer_destroy(abort_layer);
}

static void confirm_window_up_single_click(ClickRecognizerRef recognizer, void *context) {
  
  connecting_window = window_create();
  
  window_set_window_handlers(connecting_window, (WindowHandlers) {
    .load = connecting_window_load,
    .unload = connecting_window_unload
  });
  
  window_stack_push(connecting_window, true);

}

static void confirm_window_down_single_click(ClickRecognizerRef recognizer, void *context) {
  
  window_stack_remove(confirm_window, true);
  window_stack_push(pass_window, true);
  //window_stack_remove(s_wifilist_window, true);
  //window_stack_remove(s_list_window, true);
  //window_stack_push(s_main_window, true);
}

static void confirm_window_click_config_provider(void *context) {
  
  ButtonId idup = BUTTON_ID_UP;  // The Select button
  ButtonId iddown = BUTTON_ID_DOWN;  // The Select button

  window_single_click_subscribe(idup, confirm_window_up_single_click);
  window_single_click_subscribe(iddown, confirm_window_down_single_click);
  
}

static void connect(const char * text){
  strcpy(custom_password, text);
  confirm_window = window_create();
  window_set_click_config_provider(confirm_window, confirm_window_click_config_provider); //Allow the special use of buttons
  window_set_window_handlers(confirm_window, (WindowHandlers) {
    .load = confirm_window_load,
    .unload = confirm_window_unload
  });
  window_stack_push(confirm_window, true);
}

static void menu_select_callback(int index, void *ctx) {
  s_options_items[index].subtitle = "You've hit select here!";
  layer_mark_dirty(simple_menu_layer_get_layer(s_options_layer)); //Forces the redraw of the layer
}

static void pass_window_load(Window *window) {
  
  pass_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(pass_window_layer);
  
  set_pass_layer = text_layer_create(GRect(0, 0, bounds.size.w, 80));
  use_default_pass_layer = text_layer_create(GRect(0, bounds.size.h-75, bounds.size.w, 80));
  
  text_layer_set_background_color(set_pass_layer, GColorClear);
  text_layer_set_text_color(set_pass_layer, GColorBlack);
  text_layer_set_text(set_pass_layer, "Pulse 'arriba' para introducir la contraseña");
  text_layer_set_font(set_pass_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(set_pass_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(set_pass_layer, GTextOverflowModeWordWrap);
  
  text_layer_set_background_color(use_default_pass_layer, GColorClear);
  text_layer_set_text_color(use_default_pass_layer, GColorBlack);
  text_layer_set_text(use_default_pass_layer, "Pulse 'abajo' para introducir la contraseña por defecto");
  text_layer_set_font(use_default_pass_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(use_default_pass_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(use_default_pass_layer, GTextOverflowModeWordWrap);

  // Add it as a child layer to the Window's root layer
  layer_add_child(pass_window_layer, text_layer_get_layer(set_pass_layer));
  layer_add_child(pass_window_layer, text_layer_get_layer(use_default_pass_layer));
}

static void pass_window_unload(Window *window) {
  
}

void T3Handler(const char * text){
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Introduced text is: %s", text);
  t3window_destroy(myT3Window);
  connect(text);
}

static void pass_window_up_single_click(ClickRecognizerRef recognizer, void *context) {
  myT3Window = t3window_create(
    keyboardSet1, 2,
    keyboardSet2, 2,
    keyboardSet3, 2,
    (T3CloseHandler)T3Handler);
  t3window_show(myT3Window, true);
}

static void pass_window_down_single_click(ClickRecognizerRef recognizer, void *context) {
  connect(default_password);
}

static void pass_window_click_config_provider(void *context) {
  
  ButtonId idup = BUTTON_ID_UP;  // The Select button
  ButtonId iddown = BUTTON_ID_DOWN;  // The Select button

  window_single_click_subscribe(idup, pass_window_up_single_click);
  window_single_click_subscribe(iddown, pass_window_down_single_click);
  
}

void choose_ap(int index, void *ctx) {
  chosenAP = malloc(32*(sizeof(char)));
  chosenAP = s_wifiap_items[index].title;
  
  pass_window = window_create();
  window_set_click_config_provider(pass_window, pass_window_click_config_provider); //Allow the special use of buttons
  window_set_window_handlers(pass_window, (WindowHandlers) {
    .load = pass_window_load,
    .unload = pass_window_unload
  });
  window_stack_push(pass_window, true);
}

static void connect_esp32(int index, void *ctx) {
  size_t buff_size;
  uint8_t *temp_buffer = malloc(3*sizeof(uint8_t));

  if(temp_buffer == NULL){
     APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", temp_buffer);
  }
  
  time_t hour1;
  time_t hour2;
  int difference;
  
  smartstrap_attribute_begin_write(raw_attribute, &temp_buffer, &buff_size);
  strcpy((char*)temp_buffer, "60");
  smartstrap_attribute_end_write(raw_attribute, buff_size, false);
  free(temp_buffer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", temp_buffer);
  
  //Start counting time (10 s)
  hour1 = time(NULL);
  hour2 = time(NULL);
  difference = 0;
  
  while (difference <2){ 
    text_layer_set_text(s_textespmart105_layer, "eSpMART105 no detectado");
    hour2 = time(NULL);
    difference = hour2-hour1;
    if(esp_connected){
      text_layer_set_text(s_textespmart105_layer, "eSpMART105 DETECTADO");
      difference = 11;
    }
  }
  
  window_stack_remove(s_list_window, true);
  window_stack_push(s_main_window, true);
  
}

static void heartrate_window_load(Window *window) {

  heartrate_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(heartrate_layer);
  
  infobpm_layer = text_layer_create(GRect(0, 0, bounds.size.w, 70));
  bpm_layer = text_layer_create(GRect(5, 70, bounds.size.w/2, 50));
  ppm_layer = text_layer_create(GRect(bounds.size.w/2, 82, bounds.size.w/2, 50));

  text_layer_set_background_color(infobpm_layer, GColorClear);
  text_layer_set_text_color(infobpm_layer, GColorBlack);
  text_layer_set_text(infobpm_layer, "El pulso medido o la lectura más reciente de él es:");
  text_layer_set_font(infobpm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(infobpm_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(infobpm_layer, GTextOverflowModeWordWrap);
    
  HealthServiceAccessibilityMask hr = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL));
  if (hr & HealthServiceAccessibilityMaskAvailable) {
    heartRateMeasured = health_service_peek_current_value(HealthMetricHeartRateBPM);
    if(heartRateMeasured > 0) {
      static char s_hrm_buffer[12];
      text_layer_set_background_color(bpm_layer, GColorClear);
      text_layer_set_text_color(bpm_layer, GColorBlack);
      snprintf(s_hrm_buffer, sizeof(s_hrm_buffer), "%lu", (uint32_t)heartRateMeasured);
      text_layer_set_text(bpm_layer, s_hrm_buffer);
      text_layer_set_font(bpm_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
      //text_layer_set_text_alignment(bpm_layer, GTextAlignmentCenter);
      text_layer_set_overflow_mode(bpm_layer, GTextOverflowModeWordWrap);
      layer_add_child(heartrate_layer, text_layer_get_layer(bpm_layer));
      
      
      text_layer_set_background_color(ppm_layer, GColorClear);
      text_layer_set_text_color(ppm_layer, GColorBlack);
      text_layer_set_text(ppm_layer, "PPM");
      text_layer_set_font(ppm_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
      //text_layer_set_text_alignment(bpm_layer, GTextAlignmentCenter);
      text_layer_set_overflow_mode(ppm_layer, GTextOverflowModeWordWrap);
      layer_add_child(heartrate_layer, text_layer_get_layer(ppm_layer));
     
      
      //Send the measure to the ESP32
      size_t buff_size;
      uint8_t *temp_buffer = malloc(102*sizeof(uint8_t));
      uint8_t *code = malloc(3*sizeof(uint8_t));
      if(temp_buffer == NULL || code == NULL){
         APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", temp_buffer);
      }
      snprintf( (char*)code, 2, "F1"); //F is the code to HR operations (70 in decimal). 1 is store new value (49 in decimal)
      smartstrap_attribute_begin_write(raw_attribute, &temp_buffer, &buff_size);
      strcat((char*)temp_buffer, (char*) code);
      strcat((char*)temp_buffer, s_hrm_buffer);
      smartstrap_attribute_end_write(raw_attribute, buff_size, false);
      free(temp_buffer);
      free(code);
      
    } else{
       text_layer_set_text(infobpm_layer, "No ha sido posible obtener una medida");
        heart_small = gbitmap_create_with_resource(RESOURCE_ID_HEART_SMALL);
        heart_small_layer = bitmap_layer_create(GRect(5, 5, 48, 48));
        bitmap_layer_set_compositing_mode(heart_small_layer, GCompOpSet);
        bitmap_layer_set_bitmap(heart_small_layer, heart_small);
        
        layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(heart_small_layer));
      }
  }
 
  layer_add_child(heartrate_layer, text_layer_get_layer(infobpm_layer));
  
}

static void heartrate_window_unload(Window *window) {
  text_layer_destroy(infobpm_layer);
  text_layer_destroy(bpm_layer);
  gbitmap_destroy(heart_small);
  bitmap_layer_destroy(heart_small_layer);
}

static void measure_hr(int index, void *ctx) {
  
  heartrate_window = window_create();
  //window_set_click_config_provider(s_main_window, main_click_config_provider); //Allow the special use of buttons
  
  window_set_window_handlers(heartrate_window, (WindowHandlers) {
    .load = heartrate_window_load,
    .unload = heartrate_window_unload
  });

  window_stack_push(heartrate_window, true);

}

static void timer_handler(void *context) {
  static char temp_received_buffer[9];
  
  if(frame_no == NO_OF_FRAMES){
    frame_no = 0;
  }
  
  if(frame_no < NO_OF_FRAMES) {
    
    if (term_icon != NULL) {
      gbitmap_destroy(term_icon);
      term_icon = NULL;
    }
    
    term_icon = gbitmap_create_with_resource(temp_gif[frame_no]);
    
    bitmap_layer_set_bitmap(term_icon_layer, term_icon);
    layer_mark_dirty(bitmap_layer_get_layer(term_icon_layer));

    frame_no++;
    term_timer = app_timer_register(750, timer_handler, NULL);
  }
  //Start counting time (5 s)
  hour2 = time(NULL);
  
  difference = hour2-hour1;
  #ifdef DEBUG
    if(difference >=3){
      temperature_received = true;
      last_temperature[0] = '3';
      last_temperature[1] = '6';
      last_temperature[2] = '.';
      last_temperature[3] = '7';
      last_temperature[4] = '\0';
    }
  #endif
  if(difference < 5){
    if(temperature_received){
      layer_mark_dirty(temperature_layer); //Forces the redraw of the layer
      text_layer_set_text(infotemperature_textlayer, "La temperatura actual es de:");
      text_layer_set_background_color(temp_textlayer, GColorClear);
      text_layer_set_text_color(temp_textlayer, GColorBlack);
      snprintf(temp_received_buffer, sizeof(temp_received_buffer), "%s", last_temperature); 
      //davidtrc el ESP32 debe hacer una conversion CON TABLA de la salida del sensor de temp, mandando directamente la secuencia que tiene que salir por pantalla, por ejemplo "36.7",
      //de forma que last_temperature es un array de cuatro caracteres
      text_layer_set_text(temp_textlayer, temp_received_buffer);
      text_layer_set_font(temp_textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
      text_layer_set_text_alignment(temp_textlayer, GTextAlignmentCenter);
      text_layer_set_overflow_mode(temp_textlayer, GTextOverflowModeWordWrap);
      layer_add_child(temperature_layer, text_layer_get_layer(temp_textlayer));
      
      
      text_layer_set_text(degrees_textlayer, "°C");
      text_layer_set_font(degrees_textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
      text_layer_set_text_alignment(degrees_textlayer, GTextAlignmentRight);
      text_layer_set_overflow_mode(degrees_textlayer, GTextOverflowModeWordWrap);
      layer_add_child(temperature_layer, text_layer_get_layer(degrees_textlayer));
      
      term_icon = gbitmap_create_with_resource(temp_gif[0]);
      bitmap_layer_set_bitmap(term_icon_layer, term_icon);
      layer_mark_dirty(bitmap_layer_get_layer(term_icon_layer));
      app_timer_cancel(term_timer);
      difference = 11;
    }
  } else {
    if(!temperature_received){
      layer_mark_dirty(temperature_layer); //Forces the redraw of the layer
      text_layer_set_text(infotemperature_textlayer, "No ha sido posible obtener la temperatura");
      term_icon = gbitmap_create_with_resource(temp_gif[0]);
      bitmap_layer_set_bitmap(term_icon_layer, term_icon);
      layer_mark_dirty(bitmap_layer_get_layer(term_icon_layer));
      app_timer_cancel(term_timer);
    }
  }
  
}

static void temp_window_load(Window *window) {
  temperature_received = false;
  temperature_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(temperature_layer);
  
  infotemperature_textlayer = text_layer_create(GRect(0, 0, bounds.size.w, 70));
  temp_textlayer = text_layer_create(GRect(20, 40, bounds.size.w, 50));
  degrees_textlayer = text_layer_create(GRect(60, 90, bounds.size.w/2, 50));

  text_layer_set_background_color(infotemperature_textlayer, GColorClear);
  text_layer_set_text_color(infotemperature_textlayer, GColorBlack);
  text_layer_set_text(infotemperature_textlayer, "Obteniendo temperatura...Espere");
  text_layer_set_font(infotemperature_textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(infotemperature_textlayer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(infotemperature_textlayer, GTextOverflowModeWordWrap);
  term_icon_layer = bitmap_layer_create(GRect(-10, 70, 70, 70));
  bitmap_layer_set_compositing_mode(term_icon_layer, GCompOpSet);
  uint32_t first_delay_ms = 1;
  frame_no = 0;
  app_timer_register(first_delay_ms, timer_handler, NULL);

  size_t buff_size;
  uint8_t *temp_buffer= malloc(3*sizeof(uint8_t));
  if(temp_buffer == NULL){
     APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", temp_buffer);
  }
  smartstrap_attribute_begin_write(raw_attribute, &temp_buffer, &buff_size);
  snprintf( (char*)temp_buffer, 2, "02"); //0 is the code of temperature operations (48 in decimal). 2 is request the value (50 in decimal)
  smartstrap_attribute_end_write(raw_attribute, buff_size, false);
  free(temp_buffer);
 
  layer_add_child(temperature_layer, text_layer_get_layer(infotemperature_textlayer));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(term_icon_layer));
}

static void temp_window_unload(Window *window) {
  text_layer_destroy(temp_textlayer);
  text_layer_destroy(infotemperature_textlayer);
  app_timer_cancel(term_timer);
  //gbitmap_destroy(term_icon);
}

static void temperature_refresh_layer(struct tm *tick_time, TimeUnits units_changed){
  
  static char temp_received_buffer[8];
  //Start counting time (10 s)
  hour2 = time(NULL);
  
  difference = hour2-hour1;
  if(difference < 5){
    if(temperature_received){
      layer_mark_dirty(temperature_layer); //Forces the redraw of the layer
      text_layer_set_text(infotemperature_textlayer, "La temperatura actual es de:");
      text_layer_set_background_color(temp_textlayer, GColorClear);
      text_layer_set_text_color(temp_textlayer, GColorBlack);
      snprintf(temp_received_buffer, sizeof(temp_received_buffer), "%lu PPM", (uint32_t)temperature_received);
      text_layer_set_text(temp_textlayer, temp_received_buffer);
      text_layer_set_font(temp_textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
      text_layer_set_text_alignment(temp_textlayer, GTextAlignmentCenter);
      text_layer_set_overflow_mode(temp_textlayer, GTextOverflowModeWordWrap);
      layer_add_child(temperature_layer, text_layer_get_layer(temp_textlayer));
      difference = 11;
    }
  } else {
    layer_mark_dirty(temperature_layer); //Forces the redraw of the layer
    text_layer_set_text(infotemperature_textlayer, "No ha sido posible obtener la temperatura");
    app_timer_cancel(term_timer);
    tick_timer_service_unsubscribe();
  }
  
}

static void measure_temp(int index, void *ctx) {
  temperature_window = window_create();
  //window_set_click_config_provider(s_main_window, main_click_config_provider); //Allow the special use of buttons
  
  window_set_window_handlers(temperature_window, (WindowHandlers) {
    .load = temp_window_load,
    .unload = temp_window_unload
  });
  window_stack_push(temperature_window, true);
  //tick_timer_service_subscribe(SECOND_UNIT, temperature_refresh_layer);
  hour1 = time(NULL);
  difference = 0;
}

void T3Handler_pass(const char * text){
  
  int pass_length = strlen(text);
  if(pass_length >0 && pass_length <33){
    strcpy(default_password, text);
  }
  t3window_destroy(myT3Window);
}

static void set_default_pass(int index, void *ctx){
    myT3Window = t3window_create(
    keyboardSet1, 2,
    keyboardSet2, 2,
    keyboardSet3, 2,
    (T3CloseHandler)T3Handler_pass);
  t3window_show(myT3Window, true);
}

static void list_window_load(Window *window) {
  // Get information about the Window
  options_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(options_layer);
  //GRect bounds = layer_get_frame(window_layer);

  // Although we already defined NUM_FIRST_MENU_ITEMS, you can define
  // an int as such to easily change the order of menu items later
  int num_a_items = 0;

  s_options_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Con. a Valoriza",
    .callback = connect_valoriza,
  };
  s_options_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Con. eSpMART105",
    .callback = connect_esp32,
  };
  s_options_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Medir ritmo",
    .callback = measure_hr,
  };
  s_options_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Medir temperatura",
    .callback = measure_temp,
  };
  s_options_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Enviar datos",
    .callback = menu_select_callback,
  };
  s_options_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Conf. contraseña",
    .callback = set_default_pass,
    .subtitle = "(por defecto)",
  };
  s_options_sections[0] = (SimpleMenuSection) {
    .num_items = NUM_OPTIONS_ITEMS,
    .items = s_options_items,
  };
  s_options_layer = simple_menu_layer_create(bounds, s_list_window, s_options_sections, NUM_OPTIONS_SECTIONS, NULL);
  layer_add_child(options_layer, simple_menu_layer_get_layer(s_options_layer));
}

static void list_window_unload(Window *window) {
  simple_menu_layer_destroy(s_options_layer);
}

static void main_up_single_click(ClickRecognizerRef recognizer, void *context) {

  s_list_window = window_create(); //List with all possible options
  //window_set_click_config_provider(s_list_window, list_click_config_provider); //Alow the use of buttons
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_list_window, (WindowHandlers) {
    .load = list_window_load,
    .unload = list_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_list_window, true);
}

static void main_click_config_provider(void *context) {
  ButtonId idup = BUTTON_ID_UP;  // The Select button
  //ButtonId idback = BUTTON_ID_BACK;  // The Select button

  window_single_click_subscribe(idup, main_up_single_click);
  //window_single_click_subscribe(idback, main_back_single_click);

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
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // Get information about the Window
  main_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(main_window_layer);
  
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(GRect(0, 0, bounds.size.w, 50));
  s_textvaloriza_layer = text_layer_create(GRect(0, 70, bounds.size.w, 50));
  s_textespmart105_layer = text_layer_create(GRect(0, bounds.size.h-50, bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  text_layer_set_background_color(s_textvaloriza_layer, GColorClear);
  text_layer_set_text_color(s_textvaloriza_layer, GColorBlack);
  text_layer_set_text(s_textvaloriza_layer, "No conectado a la red de Valoriza");
  text_layer_set_font(s_textvaloriza_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_textvaloriza_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_textvaloriza_layer, GTextOverflowModeWordWrap);
  
  text_layer_set_background_color(s_textespmart105_layer, GColorClear);
  text_layer_set_text_color(s_textespmart105_layer, GColorBlack);
  text_layer_set_text(s_textespmart105_layer, "eSpMART105 no detectado");
  text_layer_set_font(s_textespmart105_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_textespmart105_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_textespmart105_layer, GTextOverflowModeWordWrap);

  // Add it as a child layer to the Window's root layer
  layer_add_child(main_window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(main_window_layer, text_layer_get_layer(s_textvaloriza_layer));
  layer_add_child(main_window_layer, text_layer_get_layer(s_textespmart105_layer));
  
  uart_init();
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_textvaloriza_layer);
  text_layer_destroy(s_textespmart105_layer);
}

static void prv_on_health_data(HealthEventType type, void *context) {
  // If the update was from the Heart Rate Monitor, query it
  if (type == HealthEventHeartRateUpdate) {
   heartRateMeasured = health_service_peek_current_value(HealthMetricHeartRateBPM);
    // Display the heart rate
  }
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, main_click_config_provider); //Allow the special use of buttons
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();
  //Make the HRS (heart rate sensor) accesible
  health_service_events_subscribe(prv_on_health_data, NULL);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  strncpy(default_password, "illaillaillajuanitomaravilla", 28);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
  smartstrap_attribute_destroy(raw_attribute);
  smartstrap_unsubscribe(); // Stop getting callbacks
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}