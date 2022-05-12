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

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// SD SPI pins
#define FSPI_CLK 12
#define FSPI_MISO 13
#define FSPI_MOSI 11
#define FSPI_CS0 8
#define FSPI_SCK SD_SCK_MHZ(20)

// TFT SPI pins
#define TFT_MISO_PIN 4
#define TFT_MOSI_PIN 35
#define TFT_CLK_PIN 36
#define TFT_CS_PIN 34
#define TFT_RST_PIN 38
#define TFT_DC_PIN 37
#define TFT_BKLT_PIN 33
#define TFT_PWR_PIN 14

// Button pins
#define START_LED_PIN 2
#define START_BUTTON_PIN 1
#define SETUP_BUTTON_PIN 3

SPIClass FSPI_SPI(FSPI);
SPIClass TFT_SPI(HSPI);

SdFat SD;
MD_MIDIFile SMF;
Adafruit_ST7789 tft = Adafruit_ST7789(&TFT_SPI, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

AsyncWebServer server(80);

uint32_t currentTicks;
double msPerTick;

String midiData[NUMBER_OF_DEVICES];
int midiDataCount[NUMBER_OF_DEVICES];

bool deviceDataChecks[NUMBER_OF_DEVICES];
bool deviceSyncStarts[NUMBER_OF_DEVICES];
bool deviceHardResets[NUMBER_OF_DEVICES];

bool isSetupButtonPressed() {
    if (digitalRead(SETUP_BUTTON_PIN) == BUTTON_PRESSED) {
        delay(DEBOUNCE_DELAY);
        if (digitalRead(SETUP_BUTTON_PIN) == BUTTON_PRESSED) {
            while (digitalRead(SETUP_BUTTON_PIN) == BUTTON_PRESSED)
                delay(1);

            return true;
        }
    }

    return false;
}

bool isStartButtonPressed() {
    if (digitalRead(START_BUTTON_PIN) == BUTTON_PRESSED) {
        delay(DEBOUNCE_DELAY);
        if (digitalRead(START_BUTTON_PIN) == BUTTON_PRESSED) {
            while (digitalRead(START_BUTTON_PIN) == BUTTON_PRESSED)
                delay(1);

            return true;
        }
    }

    return false;
}

bool waitAndCheckForForceContinue(int delayTimeInMs, bool startOrStop = true) {
    unsigned long wait = millis() + delayTimeInMs;
    bool forceContinue = false;
    while (wait > millis()) {
        forceContinue = startOrStop ? isStartButtonPressed() : isSetupButtonPressed();
        if (forceContinue)
            break;
        delay(10);
    }
    return forceContinue;
}

static void onMidiDataGetRequest(AsyncWebServerRequest *request) {
    int deviceNumber = request->arg("slave").toInt();
    Serial.print("Got request from slave with number: ");
    Serial.println(deviceNumber);

    if (midiData[deviceNumber].length() > 1)
        request->send_P(200, "text/plain", midiData[deviceNumber].c_str());
    else
        request->send_P(500, "text/plain", "No processed MIDI data present yet.");
}

// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
void midiCallback(midi_event *pev) {
    if (pev->data[0] == CMD_NOTE_ON) {
        uint32_t timeInMs = (uint32_t)(currentTicks * msPerTick + 0.6);

        for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
            if (pev->data[1] == DEVICE_NOTES[i][0]) {
                midiData[i] += String((unsigned long)timeInMs) + ",0;";
                midiDataCount[i]++;
            } else if (pev->data[1] == DEVICE_NOTES[i][1]) {
                midiData[i] += String((unsigned long)timeInMs) + ",1;";
                midiDataCount[i]++;
            }
        }
    }
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
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.print("Send to slave ");
        Serial.print(getDeviceNumberFromMacAddress(mac_addr));
        Serial.println(status == ESP_NOW_SEND_SUCCESS ? " succeeded." : " failed.");
    }
}

void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len) {
    int deviceNumber = getDeviceNumberFromMacAddress(mac);

    if (deviceNumber < 1 || deviceNumber >= NUMBER_OF_DEVICES)
        return;

    EspNowEvent event;
    memcpy(&event, incomingData, sizeof(event));

    if (event.cmd == ESP_NOW_EVENT_NO_MIDI_DATA) {
        Serial.print("No MIDI data on slave: ");
        Serial.println(deviceNumber);
    } else if (event.cmd == ESP_NOW_EVENT_CHECK_CONFIRM) {
        // Serial.print("MIDI data count: ");
        // Serial.println(midiDataCount[deviceNumber]);
        // Serial.print("Check data count: ");
        // Serial.println(event.value);
        deviceDataChecks[deviceNumber] = midiDataCount[deviceNumber] == event.value ? 1 : 0;

        Serial.print("Event from S");
        Serial.print(deviceNumber);
        Serial.print(": Check - ");
        Serial.println(deviceDataChecks[deviceNumber]);
    } else if (event.cmd == ESP_NOW_EVENT_START_CONFIRM) {
        deviceSyncStarts[deviceNumber] = event.value > 0 ? 1 : 0;

        Serial.print("Event from S");
        Serial.print(deviceNumber);
        Serial.print(": Start - ");
        Serial.println(deviceSyncStarts[deviceNumber]);
    } else if (event.cmd == ESP_NOW_EVENT_RESET_CONFIRM) {
        deviceHardResets[deviceNumber] = event.value > 0 ? 1 : 0;

        Serial.print("Event from S");
        Serial.print(deviceNumber);
        Serial.print(": Stop - ");
        Serial.println(deviceHardResets[deviceNumber]);
    }
}

void tftClearScreen() {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
}

bool checkValidDataOnAllSlaves() {
    tftClearScreen();
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Checking MIDI data\non all droppers:");

    unsigned long startTime = millis();
    unsigned long nextTime;
    EspNowEvent event;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        nextTime = startTime + i * DEVICE_ITERATE_DELAY;

        event.cmd = ESP_NOW_EVENT_CHECK_DATA;
        event.value = midiDataCount[i];

        while (nextTime > millis())
            delay(1);

        if (deviceDataChecks[i] == 0)
            esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&event, sizeof(event));

        tft.setTextColor(ST77XX_YELLOW);
        tft.print("d" + String(i) + " ");
    }

    delay(DEVICE_ITERATE_DELAY);

    // Give slaves time to download data
    tft.setTextColor(ST77XX_YELLOW);
    for (int i = 0; i < 3; ++i) {
        tft.print(".");
        delay(1000);
    }

    tft.println();

    bool success = true;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        if (deviceDataChecks[i] > 0) {
            tft.setTextColor(ST77XX_GREEN);
            tft.print(" v ");
        } else {
            tft.setTextColor(ST77XX_RED);
            tft.print(" X ");

            success = false;
        }
    }

    tft.println();

    if (!success) {
        tft.setTextColor(ST77XX_YELLOW);
        tft.println("Press START to\nforce continue.");
    }

    return success;
}

bool startPlaybackOnAllSlaves() {
    tftClearScreen();
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Starting playback on\nall droppers:");

    unsigned long startTime = millis();
    unsigned long nextTime;
    EspNowEvent event;

    // Reset all check flags
    for (int i = 1; i < NUMBER_OF_DEVICES; ++i)
        deviceSyncStarts[i] = 0;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        nextTime = startTime + i * DEVICE_ITERATE_DELAY;

        event.cmd = ESP_NOW_EVENT_START_SYNC;
        event.value = START_PLAYBACK_DELAY + (NUMBER_OF_DEVICES - i) * DEVICE_ITERATE_DELAY;

        while (nextTime > millis())
            delay(1);

        esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&event, sizeof(event));

        tft.setTextColor(ST77XX_YELLOW);
        tft.print("d" + String(i) + " ");
    }

    delay(DEVICE_ITERATE_DELAY);

    tft.println();

    bool success = true;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        if (deviceSyncStarts[i] > 0) {
            tft.setTextColor(ST77XX_GREEN);
            tft.print(" v ");
        } else {
            tft.setTextColor(ST77XX_RED);
            tft.print(" X ");

            success = false;
        }
    }

    tft.println();

    if (!success) {
        tft.setTextColor(ST77XX_YELLOW);
        tft.println("Press START to\nforce continue.");
    }

    return success;
}

bool resetDataOnAllSlaves() {
    tftClearScreen();
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Stopping playback on\nall droppers:");

    unsigned long startTime = millis();
    unsigned long nextTime;
    EspNowEvent event;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        nextTime = startTime + i * DEVICE_ITERATE_DELAY;

        event.cmd = ESP_NOW_EVENT_RESET;
        event.value = 1;

        while (nextTime > millis())
            delay(1);

        if (deviceHardResets[i] == 0)
            esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&event, sizeof(event));

        tft.setTextColor(ST77XX_YELLOW);
        tft.print("d" + String(i) + " ");
    }

    delay(DEVICE_ITERATE_DELAY);

    tft.println();

    bool success = true;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        if (deviceHardResets[i] > 0) {
            tft.setTextColor(ST77XX_GREEN);
            tft.print(" v ");
        } else {
            tft.setTextColor(ST77XX_RED);
            tft.print(" X ");

            success = false;
        }
    }

    tft.println();

    if (!success) {
        tft.setTextColor(ST77XX_YELLOW);
        tft.println("Press STOP to\nforce continue.");
    }

    return success;
}

void setStartButtonLed(bool on) { digitalWrite(START_LED_PIN, !on); }

void onCheckAllSlaves() {
    // Reset all check flags
    for (int i = 1; i < NUMBER_OF_DEVICES; ++i)
        deviceDataChecks[i] = 0;

    // Check all droppers if they have the MIDI data
    setStartButtonLed(false);
    while (!checkValidDataOnAllSlaves()) {
        if (waitAndCheckForForceContinue(FORCE_CONTINUE_WAIT_TIME, true)) // START button
            break;
    }
    setStartButtonLed(true);

    // Finished and ready!
    tftClearScreen();
    tft.setTextColor(ST77XX_WHITE);
    tft.println("All data on all\ndroppers checked!\n");
    tft.setTextColor(ST77XX_GREEN);
    tft.println("Ready to press\nSTART\n:) :) :)");
}

void onSetupButtonPressed() {
    // Reset all check flags
    for (int i = 1; i < NUMBER_OF_DEVICES; ++i)
        deviceHardResets[i] = 0;

    // Stop all slaves
    setStartButtonLed(false);
    while (!resetDataOnAllSlaves()) {
        if (waitAndCheckForForceContinue(FORCE_CONTINUE_WAIT_TIME, false)) // STOP button
            break;
    }
    setStartButtonLed(true);

    // Stopped successfully!
    tftClearScreen();
    tft.setTextColor(ST77XX_RED);
    tft.println("\nPlayback STOPPED!");
}

void onStartButtonPressed() {
    // Start all slaves
    setStartButtonLed(false);
    while (!startPlaybackOnAllSlaves()) {
        if (waitAndCheckForForceContinue(FORCE_CONTINUE_WAIT_TIME, true)) // START button
            break;
    }
    setStartButtonLed(true);

    // Started successfully!
    tftClearScreen();
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Sync success,\nfinal countdown:");
    tft.setTextColor(ST77XX_YELLOW);
    for (int i = (START_PLAYBACK_DELAY / 1000) - 1; i > 0; --i) {
        tft.print(String(i) + " ");
        delay(1000);
    }
    tft.setTextColor(ST77XX_GREEN);
    tft.println("\n!!! STARTED !!!\n:D :D :D");
}

void setup() {
    // Setup start button LED
    pinMode(START_LED_PIN, OUTPUT);
    digitalWrite(START_LED_PIN, HIGH);

    // Setup button pins
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SETUP_BUTTON_PIN, INPUT_PULLUP);

    digitalWrite(START_BUTTON_PIN, HIGH);
    digitalWrite(SETUP_BUTTON_PIN, HIGH);

    // Init Serial
    Serial.begin(115200);
    Serial.println();
    Serial.println("This is the MASTER device.");
    Serial.println();

    // Init WiFi
    WiFi.mode(WIFI_AP_STA);
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

    // Setting the ESP as an access point
    Serial.println("Setting up AP...");
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Start server
    Serial.println("Setting up HTTP server...");
    server.on("/mididata", HTTP_GET, onMidiDataGetRequest);
    server.begin();

    // Turn on peripherals power
    pinMode(TFT_PWR_PIN, OUTPUT);
    digitalWrite(TFT_PWR_PIN, HIGH);

    // Turn on backlight
    pinMode(TFT_BKLT_PIN, OUTPUT);
    digitalWrite(TFT_BKLT_PIN, HIGH);

    // Init SPI for TFT screen
    TFT_SPI.begin(TFT_CLK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, TFT_CS_PIN);

    // Init ST7789 1.14" 135x240 TFT
    tft.init(135, 240);                      // width, height
    tft.setSPISpeed(26ul * 1000ul * 1000ul); // 26 MHz
    tft.setRotation(1);
    tft.setTextSize(2);
    tft.setTextWrap(true);
    tft.setTextColor(ST77XX_WHITE);
    tftClearScreen();

    // Init SPI for SD card
    FSPI_SPI.begin(FSPI_CLK, FSPI_MISO, FSPI_MOSI, FSPI_CS0);

    // Initialize SD
    if (!SD.begin(SdSpiConfig(FSPI_CS0, DEDICATED_SPI, FSPI_SCK, &FSPI_SPI))) {
        Serial.println("ERROR init SD card...");
        Serial.println("Rebooting in 10 seconds.");

        tft.setTextColor(ST77XX_RED);
        tft.println("ERROR init SD card...");
        tft.println("Rebooting in 10 seconds.");

        delay(10000);
        ESP.restart();
    }

    // Clear midi data string buffer
    for (int i = 0; i < NUMBER_OF_DEVICES; ++i) {
        midiData[i] = "";
        midiDataCount[i] = 0;
    }

    // Initialize MIDIFile
    SMF.begin(&SD);
    SMF.setMidiHandler(midiCallback);
    SMF.setSysexHandler(sysexCallback);

    // Process entire MIDI file
    if (SMF.load(MIDI_FILE_NAME) == SMF.E_OK) {
        msPerTick = 1000.0 / ((double)SMF.getTempo() / 60.0 * SMF.getTicksPerQuarterNote());
        currentTicks = 0;

        while (!SMF.isEOF()) {
            SMF.processEvents(1);
            currentTicks++;
        }

        SMF.close();

        tft.setTextColor(ST77XX_GREEN);
        tft.println("MIDI file loaded!");
        delay(2000);
    } else {
        Serial.println("ERROR loading MIDI file...");
        Serial.println("Rebooting in 10 seconds.");

        tft.setTextColor(ST77XX_RED);
        tft.println("ERROR loading MIDI file...");
        tft.println("Rebooting in 10 seconds.");

        delay(10000);
        ESP.restart();
    }
}

void loop() {
    static bool allSlavesAreChecked = false;
    if (allSlavesAreChecked == false) {
        onCheckAllSlaves();
        allSlavesAreChecked = true;
    }

    if (isSetupButtonPressed())
        onSetupButtonPressed();

    if (isStartButtonPressed())
        onStartButtonPressed();

    delay(10);
}
