#include <Arduino.h>
#include <Servo.h>

Servo myservo;

#define SERVO_PIN 9
#define SENSOR_PIN A0
#define CENTER (90+3)
#define LEFT (CENTER+35)
#define RIGHT (CENTER-35)

#define LEFT_MIN_SENS 250
#define LEFT_MAX_SENS 450 // 400 + slack
#define RIGHT_MIN_SENS 550 // 600 - slack
#define RIGHT_MAX_SENS 700
// activated left: 262
// activated right: 212
// centered empty: 246
#define MAX_OFFSET_POSITION 4

#define SENSOR_AVERAGE_COUNT 20
#define LOOP_DELAY 5

int pos = 0;    // variable to store the servo position
int prevPos = 0;
bool printSensorValue = false;
int totalLeft = 0;
int totalRight = 0;
unsigned long lastPosChange = 0;
int posOffset = 0;

void changePos(int value) {
    static bool firstChange = true;
    static int prevValue = 0;

    if (value == prevValue) {
        if (pos != value) {
            lastPosChange = millis();
        }
        pos = value;

        if (pos == LEFT) {
            totalLeft++;
            Serial.print("left  ");
        } else if (pos == RIGHT) {
            totalRight++;
            Serial.print("right ");
        } else {
            return;
        }

        posOffset = 0;
        Serial.print(totalLeft);
        Serial.print(" - ");
        Serial.println(totalRight);
        return;
    }
    prevValue = value;
}

void handleInactive() {
    static int direction = 1;
    static unsigned long lastOffsetChange = 0;
    if (lastPosChange + 500 > millis()) {
        return;
    }
//
//    if (lastOffsetChange + 3 * SENSOR_AVERAGE_COUNT * LOOP_DELAY > millis()) {
//        return;
//    }
//    lastOffsetChange = millis();
//
//    posOffset += direction;
//    if (posOffset >= MAX_OFFSET_POSITION || posOffset <= -1 * MAX_OFFSET_POSITION) {
//        direction *= -1;
//    }

    myservo.write(CENTER + 5);
    delay(50);
    myservo.write(CENTER - 5);
    delay(50);

    changePos(CENTER + 3 * direction);
    changePos(CENTER + 3 * direction);
    direction *= -1;
}

void handleSensor() {
    static long sensorValueSum = 0;
    static int sensorValueSumCounter = 0;

    sensorValueSum += analogRead(SENSOR_PIN);
    if (++sensorValueSumCounter < SENSOR_AVERAGE_COUNT) {
        return;
    }

    int sensorValue = sensorValueSum / sensorValueSumCounter;
    sensorValueSum = 0;
    sensorValueSumCounter = 0;

    if (printSensorValue) {
        Serial.print("Sensor: ");
        Serial.println(sensorValue);
    }

    if (pos != CENTER) {
        changePos(CENTER);
        return;
    }

    if (sensorValue > LEFT_MIN_SENS && sensorValue < LEFT_MAX_SENS) {
        changePos(LEFT);
    } else if (sensorValue > RIGHT_MIN_SENS && sensorValue < RIGHT_MAX_SENS) {
        changePos(RIGHT);
    } else {
        changePos(CENTER);
    }
}

void handleSerial() {
    static int buf[3];
    static int bufIndex = 0;

    if (Serial.available() <= 0) {
        return;
    }
    char input = Serial.read();

    if (input == 'l') {
        pos = LEFT;
        bufIndex = 0;
        return;
    } else if (input == 'c') {
        pos = CENTER;
        bufIndex = 0;
        return;
    } else if (input == 'r') {
        pos = RIGHT;
        bufIndex = 0;
        return;
    } else if (input == 'v') {
        printSensorValue = !printSensorValue;
        return;
    }

    if (input >= 48 && input <= 58) {
        buf[bufIndex++] = input - 48;
    } else if (!(bufIndex > 0 && bufIndex <= 3)) {
        bufIndex = 0;
        return;
    }

    Serial.print(bufIndex);
    Serial.print(" buff: ");

    if (bufIndex == 1) {
        pos = buf[0];
        Serial.print(buf[0]);
    } else if (bufIndex == 2) {
        pos = buf[0] * 10 + buf[1];
        Serial.print(buf[0]);
        Serial.print(" ");
        Serial.print(buf[1]);
    } else if (bufIndex == 3) {
        pos = buf[0] * 100 + buf[1] * 10 + buf[2];
        Serial.print(buf[0]);
        Serial.print(" ");
        Serial.print(buf[1]);
        Serial.print(" ");
        Serial.print(buf[2]);
    }

    if (pos >= 180) {
        pos = 180 - 1;
    }

    Serial.print(" => ");
    Serial.println(pos);
    bufIndex = 0;
}

void setup() {
    Serial.begin(115200);
    myservo.attach(SERVO_PIN);  // attaches the servo on pin 9 to the servo object
    pinMode(SENSOR_PIN, INPUT);
    pos = CENTER;
    Serial.println("Ready");
}

void loop() {
    handleSensor();
    handleInactive();
    handleSerial();

    myservo.write(pos + posOffset);

    if (pos != prevPos) {
        delay(10);
        prevPos = pos;
    }

    delay(LOOP_DELAY);
}