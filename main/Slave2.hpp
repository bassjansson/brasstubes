#include <HTTPClient.h>
#include <WiFi.h>

#include "Defines.h"

const char *ssid = "BrassTubesAP";
const char *password = "123456789";

// Your IP address or domain name with URL path
const String getMidiDataUrl = "http://192.168.4.1/mididata";

WiFiClient client;
HTTPClient http;

String httpGETRequest(const char *url) {
    // Your Domain name with URL path or IP address with path
    http.begin(client, url);

    // Send HTTP POST request
    int httpResponseCode = http.GET();

    String payload = "---";

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        payload = http.getString();
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();

    return payload;
}

void setup() {
    Serial.begin(115200);

    Serial.println("Connecting...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    if (WiFi.status() == WL_CONNECTED) {
        String url = getMidiDataUrl + "?slave=" + DEVICE_NUMBER;
        String midiData = httpGETRequest(url.c_str());
        Serial.println(url);

        Serial.print(midiData.length());
        Serial.print(", ");
        Serial.println(sizeof(test_struct));

        if (midiData.length() == sizeof(test_struct)) {
            test_struct *test = (test_struct *)(midiData.c_str());

            Serial.print("Midi Data: ");
            Serial.print(test->x);
            Serial.print(", ");
            Serial.println(test->y);
        }
    }
}

void loop() {}
