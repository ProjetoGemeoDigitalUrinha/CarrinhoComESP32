#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <SimpleFTPServer.h>
#include <ESPAsyncWebServer.h>
#include "main_globals.h"
#include "L9110H.h"
#include "Encoders.h"
#include "Ultrasonic.h"
#include "MPU6050Handler.h"
#include "MPUProcessing.h"
#include "FFTHandler.h"
#include "WebHandlers.h"
#include "Config.h"
#include "Debug.h"

// --- OBJETOS GLOBAIS ---
L9110H robot;
AsyncWebServer server(80);
FtpServer ftpSrv;
StaticJsonDocument<200> document;

unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL_MS = 1000;

void setup() {
    Serial.begin(115200);

    // Inicialização dos sensores e periféricos
    setupEncoders();
    setupUltrasonic();
    MPU6050Handler::setupMPU6050s();

    // Inicialização do SPIFFS e leitura de configuração WiFi
    if (!SPIFFS.begin(true)) {
        Serial.println("Can't initialize SPIFFS, please restart and try again.");
        return;
    }
    loadConfig(document);

    // Configuração da rede WiFi
    const char* SSID = document["ssid"] | "ORIBot002";
    const char* PASSWORD = document["password"] | "l9110ura";
    WiFi.softAP(SSID, PASSWORD);
    Serial.println("WiFi AP started!");
    Serial.println(WiFi.softAPIP());

    // Inicialização dos endpoints HTTP e FTP
    WebHandlers::setupAll();
    server.begin();
    ftpSrv.begin(SSID, PASSWORD);
}

void loop() {
    delay(1);
    ftpSrv.handleFTP();

    // Atualização dos sensores e cálculos
    calcularRPMRodas();

    if (millis() - last_ultrasonic_read_time > 250) {
        last_ultrasonic_read_time = millis();
        readUltrasonicDistance();
    }

    // Processamento dos MPUs e FFT
    processMPU1();
    processMPU2();

    // Chama o módulo de debug para prints sincronizados
    if (millis() - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = millis();
        printDebug();
    }
}