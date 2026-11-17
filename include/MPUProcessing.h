#ifndef MPUPROCESSING_H
#define MPUPROCESSING_H

#include <stdint.h>

// Acumulador de amostras do MPU1 (100 Hz) entre duas publicações MQTT (Payload v2).
struct TelemetryWindow {
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    uint16_t n;
};

void processMPU1();
void processMPU2();
TelemetryWindow consumeMpu1TelemetryWindow();

#endif