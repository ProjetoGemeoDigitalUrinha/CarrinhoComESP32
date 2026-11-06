#include "L9110H.h"
#include "Pins.h"
#include <Arduino.h>

// O construtor roda antes do Arduino inicializar os periféricos, então não mexe
// em pinos; a configuração fica em begin().
L9110H::L9110H() {}

// Deve ser a PRIMEIRA coisa do setup(): até aqui as entradas da ponte H ficam
// flutuando e o L9110S liga os motores com ruído. Força as 4 entradas em LOW.
void L9110H::begin() {
    const int pins[] = {PIN_MOTOR1_A, PIN_MOTOR1_B, PIN_MOTOR2_A, PIN_MOTOR2_B};
    for (int pin : pins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
}

void L9110H::motor(int motorNumber, String instruction, int speed) {
    switch (motorNumber) {
        case 1: {
            pinA = PIN_MOTOR1_A;  // A1_A
            pinB = PIN_MOTOR1_B;  // A1_B
            break;
        }
        case 2: {
            pinA = PIN_MOTOR2_A;  // B1_A
            pinB = PIN_MOTOR2_B;  // B1_B
            break;
        }
        default: {
            return;
        }
    }
    if (instruction == "FORWARD") {
        analogWrite(pinB, 0);
        analogWrite(pinA, speed);
    } else if (instruction == "BACKWARD") {
        analogWrite(pinA, 0);
        analogWrite(pinB, speed);
    } else if (instruction == "STOP") {
        analogWrite(pinA, 0);
        analogWrite(pinB, 0);
    }
}

void L9110H::forward(int i, int seconds, bool hasStop) {
    motor(1, "BACKWARD", i);
    motor(2, "FORWARD", i);
    delay(seconds);
    if (hasStop) {
        stop();
    }
}

void L9110H::backward(int i, int seconds, bool hasStop) {
    motor(1, "FORWARD", i);
    motor(2, "BACKWARD", i);
    delay(seconds);
    if (hasStop) {
        stop();
    }
}

void L9110H::right(int i, int seconds, bool hasStop) {
    motor(1, "BACKWARD", i);
    motor(2, "BACKWARD", i);
    delay(seconds);
    if (hasStop) {
        stop();
    }
}

void L9110H::left(int i, int seconds, bool hasStop) {
    motor(1, "FORWARD", i);
    motor(2, "FORWARD", i);
    delay(seconds);
    if (hasStop) {
        stop();
    }
}

void L9110H::stop() {
    motor(1, "STOP", 0);
    motor(2, "STOP", 0);
}
