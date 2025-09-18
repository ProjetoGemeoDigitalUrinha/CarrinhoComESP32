#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H
#include <Arduino.h>
namespace MQTTHandler {
    void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
    void setupMQTT(std::function<void (char *, uint8_t *, unsigned int)> callback);
    void handleMQTT();
    void publishSensorData();
    void publishStatus(const char* status);
}
#endif