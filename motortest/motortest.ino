#define MOTOR_A1_PIN 26 // Motor Forward pin
#define MOTOR_A2_PIN 18 // Motor Reverse pin

#define MOTOR_B1_PIN 19 // Motor Forward pin
#define MOTOR_B2_PIN 23 // Motor Reverse pin

#define ENCODER_A1_PIN 16 // Encoder Output 'A' must connected with interupt pin of arduino
#define ENCODER_A2_PIN 17 // Encoder Output 'B' must connected with interupt pin of arduino

#define ENCODER_B1_PIN 22 // Encoder Output 'A' must connected with interupt pin of arduino
#define ENCODER_B2_PIN 21 // Encoder Output 'B' must connected with interupt pin of arduino

volatile int lastEncodedA = 0;   // Here updated value of encoder store.
volatile long encoderValueA = 0; // Raw encoder value

volatile int lastEncodedB = 0;   // Here updated value of encoder store.
volatile long encoderValueB = 0; // Raw encoder value

void IRAM_ATTR updateEncoderA() {
    int MSB = digitalRead(ENCODER_A1_PIN); // MSB = most significant bit
    int LSB = digitalRead(ENCODER_A2_PIN); // LSB = least significant bit

    int encoded = (MSB << 1) | LSB;         // converting the 2 pin value to single number
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

    int encoded = (MSB << 1) | LSB;         // converting the 2 pin value to single number
    int sum = (lastEncodedB << 2) | encoded; // adding it to the previous encoded value

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
        encoderValueB--;
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
        encoderValueB++;

    lastEncodedB = encoded; // store this value for next time
}

void setup() {
    // initialize serial comunication
    Serial.begin(115200);

    pinMode(MOTOR_A1_PIN, OUTPUT);
    pinMode(MOTOR_A2_PIN, OUTPUT);

    pinMode(MOTOR_B1_PIN, OUTPUT);
    pinMode(MOTOR_B2_PIN, OUTPUT);

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
}

void loop() {
    static int eva = 0;
    static int evb = 0;

    eva += 298 * 7;
    evb += 50 * 7;

    digitalWrite(MOTOR_A1_PIN, LOW);
    digitalWrite(MOTOR_A2_PIN, HIGH);

    while (encoderValueA < eva) {
        delay(1);
    }

    digitalWrite(MOTOR_A1_PIN, LOW);
    digitalWrite(MOTOR_A2_PIN, LOW);

    delay(500);

    digitalWrite(MOTOR_B1_PIN, HIGH);
    digitalWrite(MOTOR_B2_PIN, LOW);

    while (encoderValueB < evb) {
        delay(1);
    }

    digitalWrite(MOTOR_B1_PIN, LOW);
    digitalWrite(MOTOR_B2_PIN, LOW);

    delay(500);
}
