#ifndef VIBRATIONSENSOR_H
#define VIBRATIONSENSOR_H
#include <Arduino.h>

class VibrationSensor {
public:
    VibrationSensor(int pin);
    // Retorna true se vibração for detectada, false caso contrário
    bool isVibrationDetected();
    // Retorna o nível atual de vibração (0-1023)
    int getVibrationLevel();
    // Retorna o número do pino usado para detecção de vibração
    int getPin() const;
    // Define o número do pino usado para detecção de vibração
    void setPin(int pin);
    // Configura o pino como entrada
    void setup();
private:
    int pin; // Número do pino para o sensor de vibração
};

#endif // VIBRATIONSENSOR_H