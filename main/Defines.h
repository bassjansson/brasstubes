#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <stdlib.h>

const char *WIFI_SSID = "BrassTubesAP";
const char *WIFI_PASS = "KoenDeGrootBassJansson";

const char *MIDI_FILE_NAME = "MIDITEST.MID";

#define CMD_NOTE_ON 0x90

#define DEVICE_ITERATE_DELAY 200   // ms
#define START_PLAYBACK_DELAY 10000 // ms

#define NUMBER_OF_DEVICES 6 // 1 master + 5 slaves
#define NUMBER_OF_SLAVES_IN_USE 5

// MAC addresses
const uint8_t DEVICE_MAC_ADDRESSES[NUMBER_OF_DEVICES][6] = {
    {0x7C, 0xDF, 0xA1, 0x00, 0xD3, 0x08}, // Master  - 7C:DF:A1:00:D3:08
    {0x84, 0xCC, 0xA8, 0x61, 0x20, 0x01}, // Slave 1 - 84:CC:A8:61:20:01
    {0x84, 0xCC, 0xA8, 0x61, 0x20, 0x02}, // Slave 2 - 84:CC:A8:61:20:02
    {0x84, 0xCC, 0xA8, 0x61, 0x20, 0x03}, // Slave 3 - 84:CC:A8:61:20:03
    {0x84, 0xCC, 0xA8, 0x61, 0x20, 0x04}, // Slave 4 - 84:CC:A8:61:20:04
    {0x84, 0xCC, 0xA8, 0x61, 0x20, 0x05}, // Slave 5 - 84:CC:A8:61:20:05
};

// Notes
const uint8_t DEVICE_NOTES[NUMBER_OF_DEVICES][2] = {
    {0, 0},   // Master
    {48, 50}, // Slave 1 (c, d)
    {52, 53}, // Slave 2 (e, f)
    {55, 57}, // Slave 3 (g, a)
    {59, 60}, // Slave 4 (b, c)
    {62, 64}, // Slave 5 (d, e)
};

// Data structs
enum EspNowEventCmd {
    ESP_NOW_EVENT_NONE = 0,

    ESP_NOW_EVENT_NO_MIDI_DATA,

    ESP_NOW_EVENT_CHECK_DATA,
    ESP_NOW_EVENT_CHECK_CONFIRM,

    ESP_NOW_EVENT_START_SYNC,
    ESP_NOW_EVENT_START_CONFIRM,

    ESP_NOW_EVENT_RESET,
    ESP_NOW_EVENT_RESET_CONFIRM,
};

struct EspNowEvent {
    EspNowEventCmd cmd;
    unsigned long value;
};

struct MotorEvent {
    uint32_t time;
    uint8_t motor;
};

#endif /* __DEFINES_H__ */
