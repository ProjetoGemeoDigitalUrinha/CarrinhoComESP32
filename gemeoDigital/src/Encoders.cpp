#include "Encoders.h"
#include "main_globals.h"

#define ENCODER_DIR_PIN 34
#define ENCODER_ESQ_PIN 26
const int PPR_ENCODER = 20;

static volatile long lastEncoderDirCount = 0;
static volatile long lastEncoderEsqCount = 0;
static unsigned long lastRpmCalcTime = 0;
const unsigned long RPM_INTERVAL_MS = 100;

// Variável para delay assíncrono do print
static unsigned long lastRpmPrintTime = 0;
const unsigned long RPM_PRINT_INTERVAL_MS = 500;

void IRAM_ATTR isrEncoderDir() { encoderDirCount++; }
void IRAM_ATTR isrEncoderEsq() { encoderEsqCount++; }

void calcularRPMRodas() {
    if (millis() - lastRpmCalcTime >= RPM_INTERVAL_MS) {
        lastRpmCalcTime = millis();
        noInterrupts();
        long currentDirCount = encoderDirCount;
        long currentEsqCount = encoderEsqCount;
        interrupts();

        long deltaDir = currentDirCount - lastEncoderDirCount;
        long deltaEsq = currentEsqCount - lastEncoderEsqCount;

        rpmDir = (deltaDir / (float)PPR_ENCODER) * (60000.0 / RPM_INTERVAL_MS);
        rpmEsq = (deltaEsq / (float)PPR_ENCODER) * (60000.0 / RPM_INTERVAL_MS);

        lastEncoderDirCount = currentDirCount;
        lastEncoderEsqCount = currentEsqCount;

        // Print RPM das rodas a cada RPM_PRINT_INTERVAL_MS
        if (millis() - lastRpmPrintTime >= RPM_PRINT_INTERVAL_MS) {
            lastRpmPrintTime = millis();
            Serial.print("RPM Direita: ");
            Serial.print(rpmDir, 2);
            Serial.print(" | RPM Esquerda: ");
            Serial.println(rpmEsq, 2);
        }
    }
}

void setupEncoders() {
    pinMode(ENCODER_DIR_PIN, INPUT);
    pinMode(ENCODER_ESQ_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DIR_PIN), isrEncoderDir, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_ESQ_PIN), isrEncoderEsq, RISING);
}