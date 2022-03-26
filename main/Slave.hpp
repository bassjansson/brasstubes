#pragma once

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "Defines.h"
#include "Utils.h"

#define MOTOR_A1_PIN 26 // Motor Forward pin
#define MOTOR_A2_PIN 18 // Motor Reverse pin

#define MOTOR_B1_PIN 19 // Motor Forward pin
#define MOTOR_B2_PIN 23 // Motor Reverse pin

#define ENCODER_A1_PIN 22 // Encoder Output 'A' must connected with interupt pin of arduino
#define ENCODER_A2_PIN 21 // Encoder Output 'B' must connected with interupt pin of arduino

#define ENCODER_B1_PIN 16 // Encoder Output 'A' must connected with interupt pin of arduino
#define ENCODER_B2_PIN 17 // Encoder Output 'B' must connected with interupt pin of arduino

#define ENCODER_STEPS_PER_REV 28
#define GEAR_RATIO_MOTOR_A 298
#define GEAR_RATIO_MOTOR_B 50

#define CMD_NOTE_ON 144

volatile int lastEncodedA = 0;   // Here updated value of encoder store.
volatile long encoderValueA = 0; // Raw encoder value

volatile int lastEncodedB = 0;   // Here updated value of encoder store.
volatile long encoderValueB = 0; // Raw encoder value

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
}

void rotateQuarterMotorA() {
    static int eva = 0;
    eva += GEAR_RATIO_MOTOR_A * ENCODER_STEPS_PER_REV / 4;

    digitalWrite(MOTOR_A1_PIN, LOW);
    digitalWrite(MOTOR_A2_PIN, HIGH);

    while (encoderValueA < eva)
        delay(1);

    digitalWrite(MOTOR_A1_PIN, LOW);
    digitalWrite(MOTOR_A2_PIN, LOW);
}

void rotateQuarterMotorB() {
    static int evb = 0;
    evb += GEAR_RATIO_MOTOR_B * ENCODER_STEPS_PER_REV / 4;

    digitalWrite(MOTOR_B1_PIN, LOW);
    digitalWrite(MOTOR_B2_PIN, HIGH);

    while (encoderValueB < evb)
        delay(1);

    digitalWrite(MOTOR_B1_PIN, LOW);
    digitalWrite(MOTOR_B2_PIN, LOW);
}

void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Send to ");
    Serial.print(macAddressToReadableString(mac_addr));
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? " succeeded." : " failed.");
}

void onDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len) {
    test_struct test;
    memcpy(&test, incomingData, sizeof(test));

    Serial.print(test.x);
    Serial.print(", ");
    Serial.println(test.y);

    if (test.x == CMD_NOTE_ON) {
        if (test.y == DEVICE_NOTES[DEVICE_NUMBER][0]) {
            rotateQuarterMotorA();
        } else if (test.y == DEVICE_NOTES[DEVICE_NUMBER][1]) {
            rotateQuarterMotorB();
        }
    }

    // delay(TOF_DELAY);
    // esp_now_send(DEVICE_MAC_ADDRESSES[0], (uint8_t *)&test, sizeof(test_struct));

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
}

void loop() {}
