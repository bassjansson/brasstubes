#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <stdlib.h>

const char *WIFI_SSID = "BrassTubesAP";
const char *WIFI_PASS = "KoenDeGrootBassJansson";

const char *MIDI_FILE_NAME = "MIDITEST.MID";

#define CMD_NOTE_ON 0x90

#define START_PLAYBACK_DELAY 5000 // ms

#define NUMBER_OF_DEVICES 6 // 1 master + 5 slaves

// MAC addresses
const uint8_t DEVICE_MAC_ADDRESSES[NUMBER_OF_DEVICES][6] = {
    {0x7C, 0xDF, 0xA1, 0x00, 0xD3, 0x08}, // Master  - 7C:DF:A1:00:D3:08
    {0x84, 0xCC, 0xA8, 0x61, 0x26, 0xEC}, // Slave 1 - 84:CC:A8:61:26:EC
    {0x84, 0xCC, 0xA8, 0x61, 0x2D, 0xC4}, // Slave 2 - 84:CC:A8:61:2D:C4
    {0xC4, 0x4F, 0x33, 0x56, 0x56, 0x5D}, // Slave 3 - C4:4F:33:56:56:5D
    {0xC4, 0x4F, 0x33, 0x56, 0x56, 0x5E}, // Slave 4 - C4:4F:33:56:56:5E (TODO)
    {0xC4, 0x4F, 0x33, 0x56, 0x56, 0x5F}, // Slave 5 - C4:4F:33:56:56:5F (TODO)
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

// Synchronisation times (ms)
const unsigned long DEVICE_SYNC_TIMES[NUMBER_OF_DEVICES] = {
    0,    // Master
    200,  // Slave 1
    400,  // Slave 2
    600,  // Slave 3
    800,  // Slave 4
    1000, // Slave 5
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
