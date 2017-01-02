#ifndef USER_MAIN_H
#define USER_MAIN_H
#include "espressif/esp_common.h"


bool get_button_pressed();
bool is_connected();
bool load_server(char * _server, int *_port);
bool load_network(struct sdk_station_config* _config);
bool save_network(char * _essid, char *_password);
bool save_server(char * _server, int _port);

bool get_serial(uint8_t *_c);

void set_green_led(bool _status);
void set_green_led_blink(bool _status, unsigned int _frequency);
void set_red_led(bool _status);
void set_red_led_blink(bool _status, unsigned int _frequency);
#endif