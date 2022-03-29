#ifndef __DEFINES_H__
#define __DEFINES_H__

#define TOF_DELAY 100 // ms

#define NUMBER_OF_DEVICES 4

// MAC addresses
const uint8_t DEVICE_MAC_ADDRESSES[NUMBER_OF_DEVICES][6] = {
    {0x7C, 0xDF, 0xA1, 0x00, 0xD3, 0x08}, // Master  - 7C:DF:A1:00:D3:08
    {0x84, 0xCC, 0xA8, 0x61, 0x26, 0xEC}, // Slave 1 - 84:CC:A8:61:26:EC
    {0x84, 0xCC, 0xA8, 0x61, 0x2D, 0xC4}, // Slave 2 - 84:CC:A8:61:2D:C4
    {0xC4, 0x4F, 0x33, 0x56, 0x56, 0x5D}, // Slave 3 - C4:4F:33:56:56:5D
};

// Notes
const uint8_t DEVICE_NOTES[NUMBER_OF_DEVICES][2] = {
    {0, 0},   // Master
    {48, 50}, // Slave 1
    {52, 53}, // Slave 2
    {55, 57}, // Slave 3
};

// Synchronisation times (ms)
const unsigned long DEVICE_SYNC_TIMES[NUMBER_OF_DEVICES] = {
    0,   // Master
    200, // Slave 1
    400, // Slave 2
    600, // Slave 3
};

// Data structs
struct test_struct {
    int x;
    unsigned long y;
};

#endif /* __DEFINES_H__ */
