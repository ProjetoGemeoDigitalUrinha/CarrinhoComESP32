#include "MQTTHandler.h"
#include "main_globals.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include "L9110H.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

namespace MQTTHandler {
    void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
        String msg = String((char*)payload).substring(0, length);
        if (String(topic) == "home/commands") {
            if (msg == "frt") {
                robot.forward(255, 150, true);
            } else if (msg == "trs") {
                robot.backward(255, 150, true);
            } else if (msg == "esq") {
                robot.left(255, 150, true);
            } else if (msg == "drt") {
                robot.right(255, 150, true);
            } else if (msg == "stop") {
                robot.stop();
            } else {
                Serial.println("Unknown command");
            }
        }
    }
    void setupMQTT(std::function<void (char *, uint8_t *, unsigned int)> callback) {
        mqttClient.setServer(mqtt_server, mqtt_port);
        mqttClient.setCallback(callback);
    }
    void handleMQTT() {
        if (!mqttClient.connected()) {
            while (!mqttClient.connected()) {
                if (mqttClient.connect("ORIBotClient", mqtt_user, mqtt_password)) {
                    mqttClient.subscribe("home/commands");
                    publishStatus("Online");
                } else {
                    delay(1000);
                }
            }
        }
        mqttClient.loop();
    }
    void publishSensorData() {
        String payload = "{";
        payload += "\"rpmDir\":" + String(rpmDir) + ",";
        payload += "\"rpmEsq\":" + String(rpmEsq) + ",";
        payload += "\"distance_cm\":" + String(distance_cm) + ",";
        payload += "\"vibrationMagnitude1\":" + String(currentVibrationMagnitude1, 2) + ",";
        payload += "\"vibrationFrequency1\":" + String(dominantVibrationFrequency1, 2) + ",";
        payload += "\"vibrationAmplitude1\":" + String(dominantVibrationAmplitude1, 2) + ",";
        payload += "\"vibrationMagnitude2\":" + String(currentVibrationMagnitude2, 2) + ",";
        payload += "\"vibrationFrequency2\":" + String(dominantVibrationFrequency2, 2) + ",";
        payload += "\"vibrationAmplitude2\":" + String(dominantVibrationAmplitude2, 2) + ",";
        payload += "\"accelerationX1\":" + String(accelerationX_1, 2) + ",";
        payload += "\"accelerationY1\":" + String(accelerationY_1, 2) + ",";
        payload += "\"accelerationZ1\":" + String(accelerationZ_1, 2) + ",";
        payload += "\"rotationX1\":" + String(rotationX_1, 2) + ",";
        payload += "\"rotationY1\":" + String(rotationY_1, 2) + ",";
        payload += "\"rotationZ1\":" + String(rotationZ_1, 2) + ",";
        payload += "\"orientationX1\":" + String(orientationX_1, 2) + ",";
        payload += "\"orientationY1\":" + String(orientationY_1, 2) + ",";
        payload += "\"orientationZ1\":" + String(orientationZ_1, 2) + ",";
        payload += "\"accelerationX2\":" + String(accelerationX_2, 2) + ",";
        payload += "\"accelerationY2\":" + String(accelerationY_2, 2) + ",";
        payload += "\"accelerationZ2\":" + String(accelerationZ_2, 2) + ",";
        payload += "\"rotationX2\":" + String(rotationX_2, 2) + ",";
        payload += "\"rotationY2\":" + String(rotationY_2, 2) + ",";
        payload += "\"rotationZ2\":" + String(rotationZ_2, 2) + ",";
        payload += "\"orientationX2\":" + String(orientationX_2, 2) + ",";
        payload += "\"orientationY2\":" + String(orientationY_2, 2) + ",";
        payload += "\"orientationZ2\":" + String(orientationZ_2, 2);
        payload += "}";
        mqttClient.publish(topic_sensors, payload.c_str());
    }
    void publishStatus(const char* status) {
        mqttClient.publish("home/status", status);
    }
}