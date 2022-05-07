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

bool deviceDataChecks[NUMBER_OF_DEVICES];
bool deviceSyncStarts[NUMBER_OF_DEVICES];
bool deviceHardResets[NUMBER_OF_DEVICES];

bool allSlavesAreChecked = false;

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
            } else if (pev->data[1] == DEVICE_NOTES[i][1]) {
                midiData[i] += String((unsigned long)timeInMs) + ",1;";
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
        // Serial.print("MIDI data length: ");
        // Serial.println(midiData[deviceNumber].length());
        // Serial.print("Check data length: ");
        // Serial.println(event.value);
        deviceDataChecks[deviceNumber] = event.value > 0 ? 1 : 0;

        Serial.print(deviceNumber);
        Serial.println(": Check");
    } else if (event.cmd == ESP_NOW_EVENT_START_CONFIRM) {
        deviceSyncStarts[deviceNumber] = 1;

        Serial.print(deviceNumber);
        Serial.println(": Start");
    } else if (event.cmd == ESP_NOW_EVENT_RESET_CONFIRM) {
        deviceHardResets[deviceNumber] = 1;

        Serial.print(deviceNumber);
        Serial.println(": Reset");
    }
}

void tftClearScreen() {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
}

int checkValidDataOnAllSlaves() {
    unsigned long startTime = millis();
    unsigned long nextTime;
    EspNowEvent event;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i)
        deviceDataChecks[i] = 0;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        nextTime = startTime + i * DEVICE_ITERATE_DELAY;

        event.cmd = ESP_NOW_EVENT_CHECK_DATA;
        event.value = 1;

        while (nextTime > millis())
            delay(1);

        esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&event, sizeof(event));
    }

    delay(DEVICE_ITERATE_DELAY + 1000);

    for (int i = 1; i < (NUMBER_OF_SLAVES_IN_USE + 1); ++i) {
        if (deviceDataChecks[i] == 0)
            return i;
    }

    return 0;
}

int startPlaybackOnAllSlaves() {
    unsigned long startTime = millis();
    unsigned long nextTime;
    EspNowEvent event;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i)
        deviceSyncStarts[i] = 0;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        nextTime = startTime + i * DEVICE_ITERATE_DELAY;

        event.cmd = ESP_NOW_EVENT_START_SYNC;
        event.value = START_PLAYBACK_DELAY - i * DEVICE_ITERATE_DELAY;

        while (nextTime > millis())
            delay(1);

        esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&event, sizeof(event));
    }

    delay(DEVICE_ITERATE_DELAY + 1000);

    for (int i = 1; i < (NUMBER_OF_SLAVES_IN_USE + 1); ++i) {
        if (deviceSyncStarts[i] == 0)
            return i;
    }

    return 0;
}

int resetDataOnAllSlaves(bool hard = false) {
    unsigned long startTime = millis();
    unsigned long nextTime;
    EspNowEvent event;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i)
        deviceHardResets[i] = 0;

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i) {
        nextTime = startTime + i * DEVICE_ITERATE_DELAY;

        event.cmd = ESP_NOW_EVENT_RESET;
        event.value = hard ? 1 : 0; // 0 = soft, 1 = hard

        while (nextTime > millis())
            delay(1);

        esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&event, sizeof(event));
    }

    delay(DEVICE_ITERATE_DELAY + 1000);

    if (hard)
        delay(2000);

    for (int i = 1; i < (NUMBER_OF_SLAVES_IN_USE + 1); ++i) {
        if (deviceHardResets[i] == 0)
            return i;
    }

    return 0;
}

void setStartButtonLed(bool on) { digitalWrite(START_LED_PIN, !on); }

void onSetupButtonPressed() {
    // Stop all slaves

    tftClearScreen();
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Stopping playback on all slaves");

    tft.setTextColor(ST77XX_RED);
    int resetted = 0;
    while (resetted = resetDataOnAllSlaves(false)) {
        tft.print("s");
        tft.print(resetted);
        tft.print(" ");
    }

    tft.setTextColor(ST77XX_RED);
    tft.println("\nPlayback stopped!");
}

void onStartButtonPressed() {
    // Start all slaves

    tftClearScreen();
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Starting playback on all slaves");

    tft.setTextColor(ST77XX_RED);
    int started = 0;
    while (started = startPlaybackOnAllSlaves()) {
        tft.print("s");
        tft.print(started);
        tft.print(" ");
    }

    tft.setTextColor(ST77XX_WHITE);
    tft.println("\nSync success, final countdown:");
    tft.setTextColor(ST77XX_YELLOW);
    for (int i = (START_PLAYBACK_DELAY / 1000) - 1; i >= 0; --i) {
        tft.print(i);
        tft.print(" ");
        delay(1000);
    }

    tft.setTextColor(ST77XX_GREEN);
    tft.println("\n\nSTARTED!!! :D");
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
        Serial.println("Rebooting in 5 seconds.");

        tft.setTextColor(ST77XX_RED);
        tft.println("ERROR init SD card...");
        tft.println("Rebooting in 5 seconds.");

        delay(5000);
        ESP.restart();
    }

    // Clear midi data string buffer
    for (int i = 0; i < NUMBER_OF_DEVICES; ++i)
        midiData[i] = "";

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
    } else {
        Serial.println("ERROR loading MIDI file...");
        Serial.println("Rebooting in 5 seconds.");

        tft.setTextColor(ST77XX_RED);
        tft.println("ERROR loading MIDI file...");
        tft.println("Rebooting in 5 seconds.");

        delay(5000);
        ESP.restart();
    }
}

void loop() {
    if (!allSlavesAreChecked) {
        // Resetting all slaves
        tft.setTextColor(ST77XX_WHITE);
        tft.print("\nResetting all slaves");
        tft.setTextColor(ST77XX_RED);
        int resetted = 0;
        while (resetted = resetDataOnAllSlaves(true)) {
            tft.print("s");
            tft.print(resetted);
            tft.print(" ");
        }

        // Wait for clients to connect
        tftClearScreen();
        tft.setTextColor(ST77XX_WHITE);
        tft.print("\nWaiting for them to connect\n");
        tft.setTextColor(ST77XX_YELLOW);
        for (int i = 9; i >= 0; --i) {
            tft.print(i);
            tft.print(" ");
            delay(1000);
        }

        // Check them all if everything is okay
        tftClearScreen();
        tft.setTextColor(ST77XX_WHITE);
        tft.println("Checking data on all slaves");
        setStartButtonLed(false);
        tft.setTextColor(ST77XX_RED);
        int checked = 0;
        while (checked = checkValidDataOnAllSlaves()) {
            tft.print("s");
            tft.print(checked);
            tft.print(" ");
        }
        setStartButtonLed(true);

        // Finished and ready!
        tftClearScreen();
        tft.setTextColor(ST77XX_WHITE);
        tft.println("All data on all slaves checked!");
        tft.setTextColor(ST77XX_GREEN);
        tft.println("\nReady to press START\n\n:) :) :)");

        allSlavesAreChecked = true;
    }

    static const uint8_t BUTTON_PRESSED = LOW;
    static const uint8_t DEBOUNCE_DELAY = 50; // ms

    if (digitalRead(SETUP_BUTTON_PIN) == BUTTON_PRESSED) {
        delay(DEBOUNCE_DELAY);
        if (digitalRead(SETUP_BUTTON_PIN) == BUTTON_PRESSED) {
            while (digitalRead(SETUP_BUTTON_PIN) == BUTTON_PRESSED)
                delay(1);

            onSetupButtonPressed();
        }
    }

    if (digitalRead(START_BUTTON_PIN) == BUTTON_PRESSED) {
        delay(DEBOUNCE_DELAY);
        if (digitalRead(START_BUTTON_PIN) == BUTTON_PRESSED) {
            while (digitalRead(START_BUTTON_PIN) == BUTTON_PRESSED)
                delay(1);

            onStartButtonPressed();
        }
    }

    delay(10);
}
