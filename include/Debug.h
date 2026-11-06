#ifndef DEBUG_H
#define DEBUG_H

void printDebug();

// Motivo do último reinício do ESP, em texto (ex.: "BROWNOUT (queda de tensao ...)").
const char* resetReasonText();

#endif