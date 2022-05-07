#pragma once

#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <vector>

#include "Defines.h"
#include "Utils.h"

#define SYNC_LED_PIN 22

#define MSWITCH_A_PIN 17 // Motor A micro switch
#define MSWITCH_B_PIN 16 // Motor A micro switch

#define MOTOR_A1_PIN 23 // Motor Forward pin
#define MOTOR_A2_PIN 19 // Motor Reverse pin

#define MOTOR_B1_PIN 18 // Motor Forward pin
#define MOTOR_B2_PIN 26 // Motor Reverse pin

#define ENCODER_A1_PIN 25 // Encoder Output 'A' must connected with interupt pin of arduino
#define ENCODER_A2_PIN 27 // Encoder Output 'B' must connected with interupt pin of arduino

#define ENCODER_B1_PIN 4  // Encoder Output 'A' must connected with interupt pin of arduino
#define ENCODER_B2_PIN 32 // Encoder Output 'B' must connected with interupt pin of arduino

#define ENCODER_STEPS_PER_REV 28
#define GEAR_RATIO_MOTOR_A 298
#define GEAR_RATIO_MOTOR_B 298

volatile int lastEncodedA = 0;   // Here updated value of encoder store.
volatile long encoderValueA = 0; // Raw encoder value
volatile long targetMotorA = 0;  // Target value to stop at

volatile int lastEncodedB = 0;   // Here updated value of encoder store.
volatile long encoderValueB = 0; // Raw encoder value
volatile long targetMotorB = 0;  // Target value to stop at

std::vector<MotorEvent> motorEvents;

unsigned long motorEventsStartTime = 0;
unsigned long motorEventsPos = 0;

bool restartFlag = false;

WiFiClient client;
HTTPClient http;

void httpGETRequest(const char *url, String &payload) {
    // Your Domain name with URL path or IP address with path
    http.begin(client, url);

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        payload = http.getString();
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        payload = "";
    }

    // Free resources
    http.end();
}

void IRAM_ATTR updateEncoderA() {
    int MSB = digitalRead(ENCODER_A1_PIN); // MSB = most significant bit
    int LSB = digitalRead(ENCODER_A2_PIN); // LSB = least significant bit

    int encoded = (MSB << 1) | LSB;          // converting the 2 pin value to single number
    int sum = (lastEncodedA << 2) | encoded; // adding it to the previous encoded value

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
        encoderValueA--;
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
        encoderValueA++;

    lastEncodedA = encoded; // store this value for next time

    // Stop at target value
    if (encoderValueA >= targetMotorA) {
        digitalWrite(MOTOR_A1_PIN, LOW);
        digitalWrite(MOTOR_A2_PIN, LOW);
    }
}

void IRAM_ATTR updateEncoderB() {
    int MSB = digitalRead(ENCODER_B1_PIN); // MSB = most significant bit
    int LSB = digitalRead(ENCODER_B2_PIN); // LSB = least significant bit

    int encoded = (MSB << 1) | LSB;          // converting the 2 pin value to single number
    int sum = (lastEncodedB << 2) | encoded; // adding it to the previous encoded value

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
        encoderValueB--;
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
        encoderValueB++;

    lastEncodedB = encoded; // store this value for next time

    // Stop at target value
    if (encoderValueB >= targetMotorB) {
        digitalWrite(MOTOR_B1_PIN, LOW);
        digitalWrite(MOTOR_B2_PIN, LOW);
    }
}

void rotateMotorQuarter(int motor) {
    if (motor <= 0) {
        targetMotorA += GEAR_RATIO_MOTOR_A * ENCODER_STEPS_PER_REV / 4;

        digitalWrite(MOTOR_A1_PIN, LOW);
        digitalWrite(MOTOR_A2_PIN, HIGH);
    } else {
        targetMotorB += GEAR_RATIO_MOTOR_B * ENCODER_STEPS_PER_REV / 4;

        digitalWrite(MOTOR_B1_PIN, LOW);
        digitalWrite(MOTOR_B2_PIN, HIGH);
    }

    // Serial.print("Motor turned: ");
    // Serial.println(motor ? 'B' : 'A');
}

void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Send to ");
    Serial.print(macAddressToReadableString(mac_addr));
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? " succeeded." : " failed.");
}

void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len) {
    unsigned long receiveTime = millis();

    EspNowEvent event;
    memcpy(&event, incomingData, sizeof(event));

    if (event.cmd == ESP_NOW_EVENT_CHECK_DATA) {
        event.cmd = ESP_NOW_EVENT_CHECK_CONFIRM;
        event.value = motorEvents.size();
        esp_now_send(DEVICE_MAC_ADDRESSES[0], (uint8_t *)&event, sizeof(event));
    } else if (event.cmd == ESP_NOW_EVENT_START_SYNC) {
        if (motorEvents.size() < 1) {
            motorEventsStartTime = 0;
            motorEventsPos = 0;

            event.cmd = ESP_NOW_EVENT_NO_MIDI_DATA;
            event.value = 1;
            esp_now_send(DEVICE_MAC_ADDRESSES[0], (uint8_t *)&event, sizeof(event));
        } else {
            motorEventsStartTime = receiveTime + event.value;
            motorEventsPos = 0;

            event.cmd = ESP_NOW_EVENT_START_CONFIRM;
            event.value = 1;
            esp_now_send(DEVICE_MAC_ADDRESSES[0], (uint8_t *)&event, sizeof(event));
        }
    } else if (event.cmd == ESP_NOW_EVENT_RESET) {
        restartFlag = event.value > 0;

        motorEventsStartTime = 0;
        motorEventsPos = motorEvents.size();

        // Stop motors immediately
        targetMotorA = encoderValueA;
        targetMotorB = encoderValueB;

        event.cmd = ESP_NOW_EVENT_RESET_CONFIRM;
        event.value = 1;
        esp_now_send(DEVICE_MAC_ADDRESSES[0], (uint8_t *)&event, sizeof(event));
    }
}

void parseMidiData(String &midiData) {
    MotorEvent e;
    char c;
    String s = "";

    motorEvents.clear();

    for (int i = 0; i < midiData.length(); ++i) {
        c = midiData[i];

        if (c == ',') {
            e.time = s.toInt();
            s = "";
        } else if (c == ';') {
            e.motor = s.toInt() > 0;
            s = "";

            motorEvents.push_back(e);
            motorEventsPos = motorEvents.size();

            e.time = 0;
            e.motor = 0;
        } else {
            s += c;
        }
    }

    /*
    // Print parsed MIDI data
    Serial.println();
    Serial.println(motorEvents.size());
    Serial.println();
    for (int i = 0; i < motorEvents.size(); ++i) {
        Serial.print(motorEvents[i].time);
        Serial.print(',');
        Serial.print(motorEvents[i].motor);
        Serial.println(';');
    }
    */
}

void setup() {
    // Setup LED pin
    pinMode(SYNC_LED_PIN, OUTPUT);
    digitalWrite(SYNC_LED_PIN, LOW);

    // Setup micro switch pins
    pinMode(MSWITCH_A_PIN, INPUT_PULLUP);
    pinMode(MSWITCH_B_PIN, INPUT_PULLUP);

    // turn pullup resistors on
    digitalWrite(MSWITCH_A_PIN, HIGH);
    digitalWrite(MSWITCH_A_PIN, HIGH);

    // Setup motor pins
    pinMode(MOTOR_A1_PIN, OUTPUT);
    pinMode(MOTOR_A2_PIN, OUTPUT);

    pinMode(MOTOR_B1_PIN, OUTPUT);
    pinMode(MOTOR_B2_PIN, OUTPUT);

    // turn motors off
    digitalWrite(MOTOR_A1_PIN, LOW);
    digitalWrite(MOTOR_A2_PIN, LOW);

    digitalWrite(MOTOR_B1_PIN, LOW);
    digitalWrite(MOTOR_B2_PIN, LOW);

    // Setup encoder pins
    pinMode(ENCODER_A1_PIN, INPUT_PULLUP);
    pinMode(ENCODER_A2_PIN, INPUT_PULLUP);

    pinMode(ENCODER_B1_PIN, INPUT_PULLUP);
    pinMode(ENCODER_B2_PIN, INPUT_PULLUP);

    // turn pullup resistors on
    digitalWrite(ENCODER_A1_PIN, HIGH);
    digitalWrite(ENCODER_A2_PIN, HIGH);

    digitalWrite(ENCODER_B1_PIN, HIGH);
    digitalWrite(ENCODER_B2_PIN, HIGH);

    // call updateEncoder() when any high/low changed on encoder pins
    attachInterrupt(ENCODER_A1_PIN, updateEncoderA, CHANGE);
    attachInterrupt(ENCODER_A2_PIN, updateEncoderA, CHANGE);

    attachInterrupt(ENCODER_B1_PIN, updateEncoderB, CHANGE);
    attachInterrupt(ENCODER_B2_PIN, updateEncoderB, CHANGE);

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

    // Connect to master device with WiFi
    Serial.println("Connecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Make sure motor events is empty
    motorEvents.clear();
}

void loop() {
    // Send GET request to get MIDI data from master server
    if (motorEvents.size() == 0 && WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected to WiFi network with IP Address: ");
        Serial.println(WiFi.localIP());

        String url = "http://192.168.4.1/mididata?slave=";
        url += DEVICE_NUMBER;
        Serial.println(url);

        String midiData;
        httpGETRequest(url.c_str(), midiData);

        Serial.print("MIDI data length: ");
        Serial.println(midiData.length());

        parseMidiData(midiData);

        WiFi.disconnect();
        Serial.print("Disconnected from WiFi.");
    }

    // Schedule motor events
    if (motorEventsPos < motorEvents.size() && millis() >= motorEventsStartTime) {
        if (motorEvents[motorEventsPos].time <= (millis() - motorEventsStartTime)) {
            rotateMotorQuarter(motorEvents[motorEventsPos].motor);
            motorEventsPos++;
        } else {
            digitalWrite(SYNC_LED_PIN, (millis() - motorEventsStartTime) % 4000ul < 100);
            delay(1);
        }
    } else {
        delay(20);
    }

    // Restart if requested
    if (restartFlag) {
        delay(500);
        ESP.restart();
    }
}
