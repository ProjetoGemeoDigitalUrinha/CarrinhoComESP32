#include "Encoders.h"
#include "main_globals.h"
#include "Pins.h"

const int PPR_ENCODER = 20;

static volatile long lastEncoderDirCount = 0;
static volatile long lastEncoderEsqCount = 0;
static unsigned long lastRpmCalcTime = 0;
const unsigned long RPM_INTERVAL_MS = 100;

void IRAM_ATTR isrEncoderDir() { encoderDirCount++; }
void IRAM_ATTR isrEncoderEsq() { encoderEsqCount++; }

void calcularRPMRodas() {
    unsigned long now = millis();
    unsigned long elapsed = now - lastRpmCalcTime;
    if (elapsed >= RPM_INTERVAL_MS) {
        lastRpmCalcTime = now;
        noInterrupts();
        long currentDirCount = encoderDirCount;
        long currentEsqCount = encoderEsqCount;
        interrupts();

        long deltaDir = currentDirCount - lastEncoderDirCount;
        long deltaEsq = currentEsqCount - lastEncoderEsqCount;

        rpmDir = (deltaDir / (float)PPR_ENCODER) * (60000.0f / (float)elapsed);
        rpmEsq = (deltaEsq / (float)PPR_ENCODER) * (60000.0f / (float)elapsed);

        lastEncoderDirCount = currentDirCount;
        lastEncoderEsqCount = currentEsqCount;
    }
}

void setupEncoders() {
    pinMode(PIN_ENCODER_ESQ, INPUT_PULLUP);
    pinMode(PIN_ENCODER_DIR, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_ESQ), isrEncoderEsq, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_DIR), isrEncoderDir, RISING);
    Serial.printf("Encoders configurados: ESQ=GPIO %d, DIR=GPIO %d\n",
                  PIN_ENCODER_ESQ, PIN_ENCODER_DIR);
}