#include "Ultrasonic.h"
#include "main_globals.h"

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;

// Variável para delay assíncrono do print
static unsigned long lastDistancePrintTime = 0;
const unsigned long DISTANCE_PRINT_INTERVAL_MS = 500;

void setupUltrasonic() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
}

void readUltrasonicDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 25000);
    distance_cm = (duration * 0.0343) / 2.0;
    if (distance_cm > 400 || distance_cm <= 0) {
        distance_cm = 0;
    }
}