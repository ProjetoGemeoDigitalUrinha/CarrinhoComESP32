#include "vibrationsensor.h"

VibrationSensor::VibrationSensor(int pin) : pin(pin) {
    pinMode(pin, INPUT);
}
bool VibrationSensor::isVibrationDetected() {
    return digitalRead(pin) == HIGH; // Retorna true se o pino estiver em nível alto
}
int VibrationSensor::getVibrationLevel() {
    return analogRead(pin); // Retorna o nível de vibração (0-1023)
}
int VibrationSensor::getPin() const {
    return pin; // Retorna o número do pino
}
void VibrationSensor::setPin(int pin) {
    this->pin = pin; // Define o número do pino
    pinMode(pin, INPUT); // Configura o novo pino como entrada
}

void VibrationSensor::setup() {
    pinMode(pin, INPUT); // Configura o pino como entrada
}