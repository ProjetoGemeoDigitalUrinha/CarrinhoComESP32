#ifndef CONFIG_H
#define CONFIG_H

#include <ArduinoJson.h>

void loadConfig(StaticJsonDocument<200>& document);

#endif