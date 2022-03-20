#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdint>
#include <stdio.h>

#include "Defines.h"

char macStr[18];
char *macAddressToReadableString(const uint8_t *macAddr) {
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    return macStr;
}

int getDeviceNumberFromMacAddress(const uint8_t *macAddr) {
    for (int i = 0; i < NUMBER_OF_DEVICES; ++i) {
        const uint8_t *mac = DEVICE_MAC_ADDRESSES[i];
        bool match = true;
        for (int j = 0; j < 6; ++j)
            match = match && mac[j] == macAddr[j];
        if (match)
            return i;
    }

    return -1;
}

#endif /* __UTILS_H__ */
