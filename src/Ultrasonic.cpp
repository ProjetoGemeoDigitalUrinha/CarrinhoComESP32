#include "Ultrasonic.h"
#include "main_globals.h"
#include "Pins.h"

const int TRIG_PIN = PIN_US_TRIG;
const int ECHO_PIN = PIN_US_ECHO;

// O HC-SR04 exige ~60 ms entre disparos para evitar interferência de eco
// e não bloquear o loop() em toda iteração (preservando a amostragem de 100 Hz do MPU).
const unsigned long ULTRASONIC_READ_INTERVAL_MS = 60;

void setupUltrasonic() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
}

void readUltrasonicDistance() {
    unsigned long now = millis();
    if (now - last_ultrasonic_read_time < ULTRASONIC_READ_INTERVAL_MS) {
        return;
    }
    last_ultrasonic_read_time = now;

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 25000);
    distance_cm = (duration * 0.0343f) / 2.0f;
    if (distance_cm > 400 || distance_cm <= 0) {
        distance_cm = 0;
    }
}