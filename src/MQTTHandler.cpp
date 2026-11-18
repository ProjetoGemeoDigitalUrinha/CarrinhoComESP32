#include "MQTTHandler.h"
#include "main_globals.h"
#include "MPUProcessing.h"
#include "WebSocketControl.h"
#include "Network.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// Telemetria publicada em topic_sensors (Payload v2 com encoders):
// {
//   "t_ms": 123456,
//   "ts": 1795014300123,                     // Unix ms (UTC) via NTP; null se não sincronizado
//   "time": "2026-11-18T14:05:00.123-03:00", // ISO 8601 local; null se não sincronizado
//   "avg": true,
//   "cmd": "frt",
//   "accelerometer": { "x": 0.12, "y": -0.03, "z": 9.79 },   // m/s², média da janela MPU1
//   "gyroscope":     { "x": 0.00, "y": 0.01, "z": 1.52 },    // rad/s, média da janela MPU1
//   "temperature": 25.5,                                     // °C, MPU1
//   "Ultrassonic": { "distance": 63.40 },                    // cm; 0 sem eco
//   "rpmEsq": 0.00,
//   "rpmDir": 0.00
// }
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
        mqttClient.setBufferSize(512);
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

        TelemetryWindow w = consumeMpu1TelemetryWindow();
        bool hasWindow = (w.n > 0);
        float ax = hasWindow ? (w.ax / w.n) : accelerationX_1;
        float ay = hasWindow ? (w.ay / w.n) : accelerationY_1;
        float az = hasWindow ? (w.az / w.n) : accelerationZ_1;
        float gx = hasWindow ? (w.gx / w.n) : rotationX_1;
        float gy = hasWindow ? (w.gy / w.n) : rotationY_1;
        float gz = hasWindow ? (w.gz / w.n) : rotationZ_1;

        StaticJsonDocument<512> doc;
        doc["t_ms"] = millis();
        // Horário real (NTP) do momento do envio. Enquanto o relógio não sincroniza,
        // os dois campos vão como null (t_ms continua valendo para intervalos).
        char isoTime[32];
        if (Network::isoTimestamp(isoTime, sizeof(isoTime))) {
            doc["ts"] = Network::epochMillis();  // Unix em ms (UTC)
            doc["time"] = isoTime;               // ISO 8601, hora local
        } else {
            doc["ts"] = nullptr;
            doc["time"] = nullptr;
        }
        doc["avg"] = hasWindow;
        doc["cmd"] = WebSocketControl::activeCommandName();

        // serialized(String(v, 2)) mantém o valor como número, com 2 casas decimais.
        JsonObject accel = doc.createNestedObject("accelerometer");
        accel["x"] = serialized(String(ax, 2));
        accel["y"] = serialized(String(ay, 2));
        accel["z"] = serialized(String(az, 2));
        JsonObject gyro = doc.createNestedObject("gyroscope");
        gyro["x"] = serialized(String(gx, 2));
        gyro["y"] = serialized(String(gy, 2));
        gyro["z"] = serialized(String(gz, 2));
        doc["temperature"] = serialized(String(temperature_1, 1));
        JsonObject us = doc.createNestedObject("Ultrassonic");
        us["distance"] = serialized(String(distance_cm, 2));
        doc["rpmEsq"] = serialized(String(rpmEsq, 2));
        doc["rpmDir"] = serialized(String(rpmDir, 2));

        char payload[448];
        size_t len = serializeJson(doc, payload, sizeof(payload));
        mqttClient.publish(topic_sensors, reinterpret_cast<const uint8_t*>(payload), len);
    }
    void publishStatus(const char* status) {
        mqttClient.publish("home/status", status);
    }
}