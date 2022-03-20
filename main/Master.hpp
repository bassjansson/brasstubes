#pragma once

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "Defines.h"
#include "Utils.h"

unsigned long sendTimes[NUMBER_OF_DEVICES];

void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Serial.print("Send to ");
    // Serial.print(macAddressToReadableString(mac_addr));
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? " succeeded." : " failed.");
}

void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len) {
    unsigned long recvTime = micros();

    int i = getDeviceNumberFromMacAddress(mac);
    if (i < 0)
        return;

    double timeOfFlight = (recvTime - sendTimes[i]) / 1000.0 - TOF_DELAY;

    Serial.print("Slave ");
    Serial.print(getDeviceNumberFromMacAddress(mac));
    Serial.print(" TOF:  ");
    Serial.print(timeOfFlight);
    Serial.println(" ms");
}

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
    Serial.println(esp_err_to_name(esp_wifi_set_mac(WIFI_IF_STA, &DEVICE_MAC_ADDRESSES[DEVICE_NUMBER][0])));
    Serial.print("  Forced MAC Address:  ");
    Serial.println(WiFi.macAddress());

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW.");
        return;
    }

    // Register callbacks
    esp_now_register_send_cb(onDataSend);
    esp_now_register_recv_cb(onDataReceived);

    // Add all slaves as peers
    esp_now_peer_info_t peerInfo = {};
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        memcpy(peerInfo.peer_addr, DEVICE_MAC_ADDRESSES[i], 6);

        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.print("Failed to add slave as peer with number: ");
            Serial.println(i);
            return;
        }
    }
}

void loop() {
    Serial.println();

    test_struct test;
    test.x = random(0, 20);
    test.y = random(0, 20);

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        sendTimes[i] = micros();
        esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&test, sizeof(test_struct));
        delay(15);
    }

    delay(1000);
}
