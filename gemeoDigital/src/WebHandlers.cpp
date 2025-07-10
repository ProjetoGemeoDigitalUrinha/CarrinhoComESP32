#include "WebHandlers.h"
#include "main_globals.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "L9110H.h"
#include "Encoders.h"
#include "Ultrasonic.h"

// Externas necessárias
extern AsyncWebServer server;
extern L9110H robot;

namespace WebHandlers {

void handleStaticFiles() {
    auto serveFile = [](const char* path, const char* type = nullptr) {
        return [=](AsyncWebServerRequest *request) {
            if (type)
                request->send(SPIFFS, path, type);
            else
                request->send(SPIFFS, path);
        };
    };
    server.on("/", HTTP_GET, serveFile("/index.html"));
    server.on("/favicon.png", HTTP_GET, serveFile("/favicon.png", "image/png"));
    server.on("/style.css", HTTP_GET, serveFile("/style.css"));
    server.on("/logo.svg", HTTP_GET, serveFile("/logo.svg"));
    server.on("/downbutton.svg", HTTP_GET, serveFile("/downbutton.svg"));
    server.on("/leftbutton.svg", HTTP_GET, serveFile("/leftbutton.svg"));
    server.on("/line.svg", HTTP_GET, serveFile("/line.svg"));
    server.on("/rightbutton.svg", HTTP_GET, serveFile("/rightbutton.svg"));
    server.on("/upbutton.svg", HTTP_GET, serveFile("/upbutton.svg"));
    server.on("/stopbutton.svg", HTTP_GET, serveFile("/stopbutton.svg"));
    server.on("/script.js", HTTP_GET, serveFile("/script.js"));
}

void handleRobotCommands() {
    server.on("/frt", HTTP_GET, [](AsyncWebServerRequest *request) {
        robot.forward(255, 150, true);
        Serial.println("command frt");
        request->send(204);
    });
    server.on("/trs", HTTP_GET, [](AsyncWebServerRequest *request) {
        robot.backward(255, 150, true);
        Serial.println("command trs");
        request->send(204);
    });
    server.on("/esq", HTTP_GET, [](AsyncWebServerRequest *request) {
        robot.left(255, 150, true);
        Serial.println("command esq");
        request->send(204);
    });
    server.on("/drt", HTTP_GET, [](AsyncWebServerRequest *request) {
        robot.right(255, 150, true);
        Serial.println("command drt");
        request->send(204);
    });
    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest* request) {
        robot.stop();
        Serial.println("command stop");
        request->send(204);
    });
    server.on("/action", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("action")) {
            String action = request->getParam("action")->value();
            request->send(204);
            if (action == "quadrado") {
                for (int i = 0; i < 4; ++i) {
                    robot.forward(255, 2000, true);
                    delay(2000);
                    robot.right(255, 300, true);
                    delay(2000);
                }
                robot.stop();
            }
        }
    });
}

void handleVibrationEndpoints() {
    server.on("/mpu1/vibration_magnitude", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(currentVibrationMagnitude1, 2));
    });
    server.on("/mpu1/vibration_frequency", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(dominantVibrationFrequency1, 2));
    });
    server.on("/mpu1/vibration_amplitude", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(dominantVibrationAmplitude1, 2));
    });
    server.on("/mpu2/vibration_magnitude", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(currentVibrationMagnitude2, 2));
    });
    server.on("/mpu2/vibration_frequency", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(dominantVibrationFrequency2, 2));
    });
    server.on("/mpu2/vibration_amplitude", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(dominantVibrationAmplitude2, 2));
    });
}

void handleMPU6050DataEndpoints() {
    server.on("/mpu1/acceleration", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<100> data;
        data["x"] = String(accelerationX_1, 2); data["y"] = String(accelerationY_1, 2); data["z"] = String(accelerationZ_1, 2);
        String jsonResponse; serializeJson(data, jsonResponse); request->send(200, "application/json", jsonResponse);
    });
    server.on("/mpu1/rotation", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<100> data;
        data["x"] = String(rotationX_1, 2); data["y"] = String(rotationY_1, 2); data["z"] = String(rotationZ_1, 2);
        String jsonResponse; serializeJson(data, jsonResponse); request->send(200, "application/json", jsonResponse);
    });
    server.on("/mpu1/orientation", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<100> data;
        data["pitch"] = String(orientationX_1, 2); data["roll"] = String(orientationY_1, 2); data["yaw"] = String(orientationZ_1, 2);
        String jsonResponse; serializeJson(data, jsonResponse); request->send(200, "application/json", jsonResponse);
    });
    server.on("/mpu2/acceleration", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<100> data;
        data["x"] = String(accelerationX_2, 2); data["y"] = String(accelerationY_2, 2); data["z"] = String(accelerationZ_2, 2);
        String jsonResponse; serializeJson(data, jsonResponse); request->send(200, "application/json", jsonResponse);
    });
    server.on("/mpu2/rotation", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<100> data;
        data["x"] = String(rotationX_2, 2); data["y"] = String(rotationY_2, 2); data["z"] = String(rotationZ_2, 2);
        String jsonResponse; serializeJson(data, jsonResponse); request->send(200, "application/json", jsonResponse);
    });
    server.on("/mpu2/orientation", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<100> data;
        data["pitch"] = String(orientationX_2, 2); data["roll"] = String(orientationY_2, 2); data["yaw"] = String(orientationZ_2, 2);
        String jsonResponse; serializeJson(data, jsonResponse); request->send(200, "application/json", jsonResponse);
    });
}

void handleUltrasonicEndpoint() {
    server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(distance_cm, 2));
    });
}

void handleEncoderRpmEndpoints() {
    server.on("/rpm_dir", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(rpmDir, 2));
    });
    server.on("/rpm_esq", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(rpmEsq, 2));
    });
}

void setupAll() {
    handleStaticFiles();
    handleRobotCommands();
    handleVibrationEndpoints();
    handleMPU6050DataEndpoints();
    handleUltrasonicEndpoint();
    handleEncoderRpmEndpoints();
}

} // namespace WebHandlers