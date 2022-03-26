#pragma once

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

const uint16_t WAIT_DELAY = 2000; // ms

const char *tuneList[] = {"MIDITEST.MID"};

SPIClass FSPI_SPI(FSPI);
SdFat SD;
MD_MIDIFile SMF;

// unsigned long sendTimes[NUMBER_OF_DEVICES];

// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
void midiCallback(midi_event *pev) {
    Serial.print(pev->data[0]);
    Serial.print(", ");
    Serial.println(pev->data[1]);

    test_struct test;
    test.x = pev->data[0];
    test.y = pev->data[1];

    for (int i = 1; i < NUMBER_OF_DEVICES; ++i)
        esp_now_send(DEVICE_MAC_ADDRESSES[i], (uint8_t *)&test, sizeof(test_struct));

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

// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
void midiSilence() {
    midi_event ev;

    // All sound off
    // When All Sound Off is received all oscillators will turn off, and their volume
    // envelopes are set to zero as soon as possible.
    ev.size = 0;
    ev.data[ev.size++] = 0xb0;
    ev.data[ev.size++] = 120;
    ev.data[ev.size++] = 0;

    for (ev.channel = 0; ev.channel < 16; ev.channel++)
        midiCallback(&ev);
}

void tickMetronome() {}

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

    static enum { S_IDLE, S_PLAYING, S_END, S_WAIT_BETWEEN } state = S_IDLE;
    static uint16_t currTune = ARRAY_SIZE(tuneList);
    static uint32_t timeStart;

    switch (state) {
    case S_IDLE: // now idle, set up the next tune
    {
        int err;
        currTune++;
        if (currTune >= ARRAY_SIZE(tuneList))
            currTune = 0;

        // use the next file name and play it
        Serial.print("File: ");
        Serial.println(tuneList[currTune]);
        err = SMF.load(tuneList[currTune]);
        if (err != MD_MIDIFile::E_OK) {
            Serial.print(" - SMF load Error ");
            Serial.println(err);
            timeStart = millis();
            state = S_WAIT_BETWEEN;
        } else {
            state = S_PLAYING;
        }
    } break;

    case S_PLAYING: // play the file
        if (!SMF.isEOF()) {
            if (SMF.getNextEvent())
                tickMetronome();
        } else
            state = S_END;
        break;

    case S_END: // done with this one
        SMF.close();
        midiSilence();
        timeStart = millis();
        state = S_WAIT_BETWEEN;
        break;

    case S_WAIT_BETWEEN: // signal finished with a dignified pause
        if (millis() - timeStart >= WAIT_DELAY)
            state = S_IDLE;
        break;

    default:
        state = S_IDLE;
        break;
    }
}
