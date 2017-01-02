#ifndef CONFIG_HPP
#define CONFIG_HPP

#define DEFAULT_SSID "esp-rfid"

// should be firmwar_1.bin on the server
#define TFTP_IMAGE_FILENAME "firmware.bin"

#define GREEN_LED_PIN 04
#define RED_LED_PIN 05
#define NEXT_PUSH_PIN 15
#define PLAYPAUSE_PUSH_PIN 2
#define PROGRAM_PUSH_PIN 0

#define ROTARY_A 14
#define ROTARY_B 12
#define ROTARY_C 13

#define BUTTON_PRESSED_DELAY 100

#define UP_TAG "AF00AF00AC"
#define DOWN_TAG "AF00AF00AD"
#define NEXT_TAG "AF00AF00AE"
#define PAUSE_TAG "AF00AF00AF"

#define RAW_TCP
#endif