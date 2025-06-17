#include "vibrationsensor.h"

VibrationSensor::VibrationSensor(int pin) : pin(pin) {
    pinMode(pin, INPUT);
}
bool VibrationSensor::isVibrationDetected() {
    return digitalRead(pin) == HIGH; // RETURONA TRUE SE O PINO ESTIVER ALTO
}
int VibrationSensor::getVibrationLevel() {
    return analogRead(pin); // RETORNA O NÍVEL DE VIBRAÇÃO (0-1023)
}
int VibrationSensor::getPin() const {
    return pin; // RETORNA O NÚMERO DO PINO
}
void VibrationSensor::setPin(int pin) {
    this->pin = pin; // DEFINE O NÚMERO DO PINO
    pinMode(pin, INPUT); // CONFIGURA O NOVO PINO COMO ENTRADA
}

void VibrationSensor::setup() {
    pinMode(pin, INPUT); // CONFIGURA O PINO COMO ENTRADA
}