#include "Encoders.h"
#include "main_globals.h"

// O shield REV 4.0 (ver include/Pins.h) não tem conexão para os encoders das
// rodas, então nenhuma interrupção é registrada e rpmDir/rpmEsq ficam em 0.
// Quando o hardware tiver os encoders, defina os pinos em Pins.h e faça o
// attachInterrupt() de isrEncoderDir/isrEncoderEsq em setupEncoders().
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
    }
}

void setupEncoders() {
    Serial.println("Encoders: sem conexao no shield REV 4.0 - RPM desativado.");
}