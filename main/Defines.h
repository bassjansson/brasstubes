#ifndef __DEFINES_H__
#define __DEFINES_H__

#define NUMBER_OF_DEVICES 2

// MAC addresses
const uint8_t DEVICE_MAC_ADDRESSES[NUMBER_OF_DEVICES][6] = {
    {0x7C, 0xDF, 0xA1, 0x00, 0xD3, 0x08}, // Master  - 7C:DF:A1:00:D3:08
    {0x84, 0xCC, 0xA8, 0x61, 0x26, 0xEC}, // Slave 1 - 84:CC:A8:61:26:EC
};

// Data structs
struct test_struct {
    int x;
    int y;
};

#endif /* __DEFINES_H__ */
