#ifndef ENCODERS_H
#define ENCODERS_H

#include <Arduino.h>

void IRAM_ATTR isrEncoderDir();
void IRAM_ATTR isrEncoderEsq();
void calcularRPMRodas();
void setupEncoders();

#endif