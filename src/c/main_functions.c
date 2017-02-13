#include <pebble.h>
#include "main_functions.h"
#include "src/c/main.h"

#define DEBUG
#define MAX_AP_SSID 15

char *apssid [MAX_AP_SSID];
extern bool apsready;
extern bool esp_connected;

extern Window *s_wifilist_window;
extern Layer *wifilist_layer;
extern SimpleMenuLayer *s_wifiap_layer;
extern SimpleMenuSection s_wifiap_sections[1];
extern SimpleMenuItem s_wifiap_items[7];
extern bool wificonnected;
extern char last_temperature [5];
extern bool temperature_received;

const char* chosenAP;
uint8_t* password;

int apnumber = 0;

int decodedata( uint8_t *data){
  uint8_t operation = *data++;
  char numberofaps[2];
  apnumber = 0;
  int i = 0;
  int j = 0;
  //char temp_received[5];
  
  char ssid[32];
  char* pointer_error = "Error allocating pointer";
  
  switch(operation){
    case 1:
      break;
    case 2:
      break;
    case 48: //Temperature stuff
      last_temperature[0] = *data++;
      last_temperature[1] = *data++;
      last_temperature[2] = *data++;
      last_temperature[3] = *data;
      last_temperature[4] = '\0';
      //last_temperature = strtol(temp_received, NULL, 10);
      //sscanf(temp_received, "%lu", &temp_temp);
      temperature_received = true;
      break;
    case 50:
      numberofaps[0] = *data++;
      numberofaps[1] = *data++;
      apnumber = strtol(numberofaps, NULL, 10);
      for(i = 0; i<MAX_AP_SSID; i++){
        apssid [i] = NULL;
      }
      for(i = 0; i<apnumber; i++){
        apssid [i] = malloc(32*sizeof(uint8_t));
        if(apssid[i] == NULL){APP_LOG(APP_LOG_LEVEL_ERROR, pointer_error); return 0;}
        for(j = 0; j<32; j++){
          ssid[j] = *data++;
        }
        strncpy(apssid[i], ssid,32);
      }
      apsready = true;
      break;
    case 55:
      if( strncmp((char*) data, "1", 1) == 0){
        wificonnected = true;
      }
      break;
    case 60:
      if( strncmp((char*) data, "1", 1) == 0){
       esp_connected = true;
      }
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "UNRECOGNIZED OPERATION");
      break;
  }
  
  return operation;
}

void printAPS(int apnumber2){
  
  GRect bounds = layer_get_bounds(wifilist_layer);
  //layer_mark_dirty(wifilist_layer);
  int num_a_items = 0;
  
  #ifndef DEBUG
  int i = 0;
  for(i=0; i<apnumber2; i++){
     s_wifiap_items[num_a_items++] = (SimpleMenuItem) {
    .title = apssid [i],
    //.subtitle = "This has an icon",
    .callback = choose_ap,
    //.icon = s_menu_icon_image,
    };
  }
  
  s_wifiap_sections[0] = (SimpleMenuSection) {
    .num_items = num_a_items,
    .items = s_wifiap_items,
  };
  #endif
  
  #ifdef DEBUG
  s_wifiap_items[num_a_items++] = (SimpleMenuItem) {
    .title = "AndroidAPAndroidAPAndroidAP",
    .callback = choose_ap,
    //.subtitle = "This has an icon",
    
    //.icon = s_menu_icon_image,
  };
  s_wifiap_items[num_a_items++] = (SimpleMenuItem) {
    .title = "B105_net",
    .callback = choose_ap,
  };
  s_wifiap_items[num_a_items++] = (SimpleMenuItem) {
    .title = "B105_net_5Ghz",
    .callback = choose_ap,
  };
  s_wifiap_items[num_a_items++] = (SimpleMenuItem) {
    .title = "B105_net_plus",
    .callback = choose_ap,
  };
  s_wifiap_items[num_a_items++] = (SimpleMenuItem) {
    .title = "eduroam",
    .callback = choose_ap,
  };
  s_wifiap_sections[0] = (SimpleMenuSection) {
    .num_items = num_a_items,
    .items = s_wifiap_items,
  };
  #endif
  
  s_wifiap_layer = simple_menu_layer_create(bounds, s_wifilist_window, s_wifiap_sections, 1, NULL);
  layer_add_child(wifilist_layer, simple_menu_layer_get_layer(s_wifiap_layer));
}
