#include "MQTTHandler.h"
#include "main_globals.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// Telemetria publicada em topic_sensors (um único JSON por mensagem):
// {
//   "accelerometer": { "x": 0.00, "y": 0.00, "z": 0.00 },   // m/s², MPU1
//   "gyroscope":     { "x": 0.00, "y": 0.00, "z": 0.00 },   // rad/s, MPU1
//   "temperature": 25.5,                                    // °C, MPU1
//   "Ultrassonic": { "distance": 100.23 }
// }
// "distance" em cm; 0 quando não há eco (fora de 2-400 cm).
WiFiClient espClient;
PubSubClient mqttClient(espClient);

namespace MQTTHandler {
    // A movimentação do carrinho é feita só pelo WebSocket (WebSocketControl.cpp).
    // O MQTT fica para telemetria; mensagens recebidas apenas são registradas.
    void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
        String msg;
        msg.reserve(length);
        for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
        Serial.printf("MQTT [%s]: %s\n", topic, msg.c_str());
    }
    void setupMQTT(std::function<void (char *, uint8_t *, unsigned int)> callback) {
        mqttClient.setServer(mqtt_server, mqtt_port);
        mqttClient.setCallback(callback);
    }
    void handleMQTT() {
        // Sem broker acessível, tenta de novo a cada 5 s em vez de travar o loop().
        static unsigned long lastAttemptMs = 0;
        const unsigned long RETRY_INTERVAL_MS = 5000;
        if (WiFi.status() != WL_CONNECTED) {
            return;  // sem Wi-Fi, abrir socket derruba o ESP (assert do lwIP)
        }
        if (!mqttClient.connected()) {
            if (lastAttemptMs == 0 || millis() - lastAttemptMs >= RETRY_INTERVAL_MS) {
                lastAttemptMs = millis();
                if (mqttClient.connect("ORIBotClient", mqtt_user, mqtt_password)) {
                    mqttClient.subscribe("home/commands");
                    publishStatus("Online");
                }
            }
            return;
        }
        mqttClient.loop();
    }
    void publishSensorData() {
        if (!mqttClient.connected()) return;

        StaticJsonDocument<384> doc;
        // serialized(String(v, 2)) mantém o valor como número, com 2 casas decimais.
        JsonObject accel = doc.createNestedObject("accelerometer");
        accel["x"] = serialized(String(accelerationX_1, 2));
        accel["y"] = serialized(String(accelerationY_1, 2));
        accel["z"] = serialized(String(accelerationZ_1, 2));
        JsonObject gyro = doc.createNestedObject("gyroscope");
        gyro["x"] = serialized(String(rotationX_1, 2));
        gyro["y"] = serialized(String(rotationY_1, 2));
        gyro["z"] = serialized(String(rotationZ_1, 2));
        doc["temperature"] = serialized(String(temperature_1, 1));
        JsonObject us = doc.createNestedObject("Ultrassonic");
        us["distance"] = serialized(String(distance_cm, 2));

        char payload[256];
        size_t len = serializeJson(doc, payload, sizeof(payload));
        mqttClient.publish(topic_sensors, reinterpret_cast<const uint8_t*>(payload), len);
    }
    void publishStatus(const char* status) {
        mqttClient.publish("home/status", status);
    }
}