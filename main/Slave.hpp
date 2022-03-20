#pragma once

#include <WiFi.h>
#include <esp_wifi.h>

#include "Defines.h"

void setup() {
    // Init Serial
    Serial.begin(115200);
    Serial.println();
    Serial.print("This is a SLAVE device with number: ");
    Serial.println(DEVICE_NUMBER);
    Serial.println();

    // Init WiFi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Set MAC address
    Serial.print(" Default MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.print("Changing MAC Address:  ");
    Serial.println(esp_err_to_name(esp_wifi_set_mac(WIFI_IF_STA, &DEVICE_MAC_ADDRESSES[DEVICE_NUMBER][0])));
    Serial.print("  Forced MAC Address:  ");
    Serial.println(WiFi.macAddress());
}

void loop() {}
