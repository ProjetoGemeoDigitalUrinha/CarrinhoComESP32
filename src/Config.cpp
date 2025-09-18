#include "Config.h"
#include <SPIFFS.h>
#include <Arduino.h>

void loadConfig(StaticJsonDocument<200>& document) {
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Can't read config.json, please restart and try again.");
        return;
    }
    DeserializationError error = deserializeJson(document, configFile);
    configFile.close();
    if (error) {
        Serial.println(error.f_str());
        return;
    }
}