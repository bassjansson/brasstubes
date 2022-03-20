#pragma once

#include <WiFi.h>
#include <esp_wifi.h>

#include "Defines.h"

void setup() {
    // Init Serial
    Serial.begin(115200);
    Serial.println();
    Serial.println("This is the MASTER device.");
    Serial.println();

    // Init WiFi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Set MAC address
    Serial.print(" Default MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.print("Changing MAC Address:  ");
    Serial.println(esp_err_to_name(esp_wifi_set_mac(WIFI_IF_STA, &MASTER_MAC_ADDRESS[0])));
    Serial.print("  Forced MAC Address:  ");
    Serial.println(WiFi.macAddress());
}

void loop() {}
