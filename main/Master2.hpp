#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "Defines.h"

// Set your access point network credentials
const char *ssid = "BrassTubesAP";
const char *password = "123456789";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

static void onMidiDataGetRequest(AsyncWebServerRequest *request) {
    int deviceNumber = request->arg("slave").toInt();
    Serial.print("Got request from slave with number: ");
    Serial.println(deviceNumber);

    // String data = "Hello World! Slave = " + String(deviceNumber);
    // request->send_P(200, "text/plain", data.c_str());

    static test_struct test;
    test.x = deviceNumber;
    test.y = 12345;
    request->send_P(200, "application/octet-stream", (uint8_t *)&test, sizeof(test_struct));
}

void setup() {
    // Serial port for debugging purposes
    Serial.begin(115200);
    Serial.println();

    // Setting the ESP as an access point
    Serial.print("Setting up AP...");
    // Remove the password parameter if you want the AP to be open
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Start server
    Serial.println("Setting up HTTP server...");
    server.on("/mididata", HTTP_GET, onMidiDataGetRequest);
    server.begin();
}

void loop() {}
