#pragma once

#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "Defines.h"
#include "Utils.h"

#include <MD_MIDIFile.h>
#include <SPI.h>
#include <SdFat.h>

// SD SPI pins
#define FSPI_CLK 12
#define FSPI_MISO 13
#define FSPI_MOSI 11
#define FSPI_CS0 10
#define FSPI_SCK SD_SCK_MHZ(20)

AsyncWebServer server(80);

SPIClass FSPI_SPI(FSPI);
SdFat SD;
MD_MIDIFile SMF;

uint64_t currentTicks;
double msPerTick;

// unsigned long sendTimes[NUMBER_OF_DEVICES];

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

// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
void midiCallback(midi_event *pev) {
    if (pev->data[0] == CMD_NOTE_ON) {
        Serial.print((uint64_t)(currentTicks * msPerTick + 0.6));
        Serial.print("\t-\t");
        Serial.print(pev->data[1]);
        Serial.println();
    }

    // test_struct test;
    // test.x = pev->data[0];
    // test.y = pev->data[1];

    // for (int i = 1; i < NUMBER_OF_DEVICES; ++i)
    //     esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&test, sizeof(test_struct));

    // if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0)) {
    //     Serial.write(pev->data[0] | pev->channel);
    //     Serial.write(&pev->data[1], pev->size - 1);
    // } else
    //     Serial.write(pev->data, pev->size);
    // DEBUG("\n", millis());
    // DEBUG("\tM T", pev->track);
    // DEBUG(":  Ch ", pev->channel + 1);
    // DEBUGS(" Data");
    // for (uint8_t i = 0; i < pev->size; i++)
    //     DEBUGX(" ", pev->data[i]);
}

// Called by the MIDIFile library when a system Exclusive (sysex) file event needs
// to be processed through the midi communications interface. Most sysex events cannot
// really be processed, so we just ignore it here.
// This callback is set up in the setup() function.
void sysexCallback(sysex_event *pev) {
    // DEBUG("\nS T", pev->track);
    // DEBUGS(": Data");
    // for (uint8_t i = 0; i < pev->size; i++)
    //     DEBUGX(" ", pev->data[i]);
}

void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Serial.print("Send to ");
    // Serial.print(macAddressToReadableString(mac_addr));
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? " succeeded." : " failed.");
}

void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len) {
    // unsigned long recvTime = micros();
    //
    // int i = getDeviceNumberFromMacAddress(mac);
    // if (i < 0)
    //     return;
    //
    // double timeOfFlight = (recvTime - sendTimes[i]) / 1000.0 - TOF_DELAY;
    //
    // Serial.print("Slave ");
    // Serial.print(getDeviceNumberFromMacAddress(mac));
    // Serial.print(" TOF:  ");
    // Serial.print(timeOfFlight);
    // Serial.println(" ms");
}

void setup() {
    // Init Serial
    Serial.begin(115200);
    Serial.println();
    Serial.println("This is the MASTER device.");
    Serial.println();

    // Init WiFi
    WiFi.mode(WIFI_AP); // WIFI_STA_AP
    WiFi.disconnect();

    // Set MAC address
    Serial.print(" Default MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.print("Changing MAC Address:  ");
    Serial.println(esp_err_to_name(esp_wifi_set_mac(WIFI_IF_STA, &DEVICE_MAC_ADDRESSES[DEVICE_NUMBER][0])));
    Serial.print("  Forced MAC Address:  ");
    Serial.println(WiFi.macAddress());

    // Setting the ESP as an access point
    Serial.print("Setting up AP...");
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Start server
    Serial.println("Setting up HTTP server...");
    server.on("/mididata", HTTP_GET, onMidiDataGetRequest);
    server.begin();

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

    // Init SPI for SD card
    FSPI_SPI.begin(FSPI_CLK, FSPI_MISO, FSPI_MOSI, FSPI_CS0);

    // Initialize SD
    if (!SD.begin(SdSpiConfig(FSPI_CS0, DEDICATED_SPI, FSPI_SCK, &FSPI_SPI))) {
        Serial.println("SD init fail!");
        while (true)
            ;
    }

    // Initialize MIDIFile
    SMF.begin(&SD);
    SMF.setMidiHandler(midiCallback);
    SMF.setSysexHandler(sysexCallback);

    // Process entire MIDI file
    if (SMF.load(midiFileName) == SMF.E_OK) {
        msPerTick = 1000.0 / ((double)SMF.getTempo() / 60.0 * SMF.getTicksPerQuarterNote());
        currentTicks = 0;

        while (!SMF.isEOF()) {
            SMF.processEvents(1);
            currentTicks++;
        }

        SMF.close();

        Serial.println("MIDI file processed!");
    } else {
        Serial.println("Error loading midi file!");
    }

    // Sync slaves on boot
    // TODO: sync slaves on start button and in a continues fasion
    /*
    unsigned long startTime = millis();

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        unsigned long nextTime = startTime + DEVICE_SYNC_TIMES[i];

        while (nextTime > millis())
            delay(1);

        test_struct test;
        test.x = 1;
        test.y = DEVICE_SYNC_TIMES[i];

        esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&test, sizeof(test_struct));
    }
    */
}

void loop() {
    // Serial.println();
    //
    // test_struct test;
    // test.x = random(0, 20);
    // test.y = random(0, 20);
    //
    // for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
    //     sendTimes[i] = micros();
    //     esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&test, sizeof(test_struct));
    //     delay(15);
    // }
    //
    // delay(1000);
}
