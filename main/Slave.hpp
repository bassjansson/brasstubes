#pragma once

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "Defines.h"
#include "Utils.h"

void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Send to ");
    Serial.print(macAddressToReadableString(mac_addr));
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? " succeeded." : " failed.");
}

void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len) {
    static test_struct test;

    delay(TOF_DELAY);
    esp_now_send(DEVICE_MAC_ADDRESSES[0], (uint8_t *)&test, sizeof(test_struct));

    /*
    test_struct myData;
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("x: ");
    Serial.println(myData.x);
    Serial.print("y: ");
    Serial.println(myData.y);
    Serial.println();
    */
}

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

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW.");
        return;
    }

    // Register callbacks
    esp_now_register_send_cb(onDataSend);
    esp_now_register_recv_cb(onDataReceived);

    // Add the master as a peer
    esp_now_peer_info_t peerInfo = {};
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    memcpy(peerInfo.peer_addr, DEVICE_MAC_ADDRESSES[0], 6);

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add the master as a peer.");
        return;
    }
}

void loop() {}
