#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdint>
#include <stdio.h>

char macStr[18];
char *macAddressToReadableString(const uint8_t *macAddr) {
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    return macStr;
}

#endif /* __UTILS_H__ */
