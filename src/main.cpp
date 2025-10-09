/*
#include <Arduino.h>
#include <Wire.h>
#include "QMC5883LCompass.h"

QMC5883LCompass compass;

int x_min = 32767, x_max = -32768;
int y_min = 32767, y_max = -32768;
int z_min = 32767, z_max = -32768;

void setup() {
  Serial.begin(1152200);
  Wire.begin();
  compass.init();
  Serial.println("==============================================");
  Serial.println("Iniciando calibração do ROBÔ (via USB).");
  Serial.println("Afaste o robô do PC e abra o Monitor Serial.");
  Serial.println("Comece a girar o robô LENTAMENTE em TODAS as direções...");
  Serial.println("==============================================");
}

void loop() {
  compass.read();
  
  int x = compass.getX();
  int y = compass.getY();
  int z = compass.getZ();

  if (x < x_min) x_min = x; if (x > x_max) x_max = x;
  if (y < y_min) y_min = y; if (y > y_max) y_max = y;
  if (z < z_min) z_min = z; if (z > z_max) z_max = z;

  Serial.print("x_min/max: "); Serial.print(x_min); Serial.print("/"); Serial.print(x_max);
  Serial.print("\ty_min/max: "); Serial.print(y_min); Serial.print("/"); Serial.print(y_max);
  Serial.print("\tz_min/max: "); Serial.print(z_min); Serial.print("/"); Serial.print(z_max);
  Serial.println();
  
  delay(100);
}
*/
// FIM DO CÓDIGO UTILIZADO PARA CALIBRAR A BÚSSOLA

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
#include "MQTTHandler.h"
#include "Config.h"
#include "Debug.h"
#include "Network.h"

// ===================================================================================
// --- O HMC NA VERDADE É FALSIFICADO. ELE É O QMC. PASSARAM A PERNA ---
#include <Wire.h>
#include "QMC5883LCompass.h"
// ===================================================================================


// --- OBJETOS GLOBAIS ---
L9110H robot;
AsyncWebServer server(80);
FtpServer ftpSrv;
StaticJsonDocument<200> document;

// ===================================================================================
QMC5883LCompass compass;
float yaw = 0.0;
float yaw_offset = 0.0;
unsigned long lastYawReadTime = 0;
// ===================================================================================

unsigned long debugLastPrintTime = 0;
const unsigned long DEBUG_PRINT_INTERVAL_MS = 1000;

void setup() {
    Serial.begin(115200);
    // ===================================================================================
    Serial.println("Iniciando o sensor de Yaw (QMC5883L)...");
    Wire.begin();
    compass.init();
    // --- VALORES DE CALIBRAÇÃO OBTIDOS PELO CÓDIGO COMENTADO NO INÍCIO. VOCÊ RODA ELE E
    //GIRA O SENSOR DE TODAS AS FORMAS POSSÍVELS POR ALGUNS MINUTOS ATÉ OS VALORES ESTABILIZAREM ---
    compass.setCalibration(-792, 997, 1287, 3073, -1963, -546);
    delay(500);
    compass.read();
    yaw_offset = compass.getAzimuth();
    Serial.println("Sensor de Yaw calibrado e pronto.");
    // ===================================================================================
    MPU6050Handler::setupMPU6050s();
    setupEncoders();
    setupUltrasonic();
    // Inicialização do SPIFFS e leitura de configuração WiFi
    if (!SPIFFS.begin(true)) {
        Serial.println("Can't initialize SPIFFS, please restart and try again.");
        return;
    }
    loadConfig(document);
    // Network setup
    Network::connectWiFi(ssid, password);
    while (!Network::isWiFiConnected()) {
        Serial.print(".");
        delay(50);
    }
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    while (!Network::beginMDNS(mdns_hostname)) {
        Serial.println("Error setting up MDNS responder!");
    }
    Network::addMDNSService(mdns_service, mdns_proto, mdns_port);
    Serial.println("mDNS responder started");
    delay(10000);
    // ===================================================================================
    // Configuração da rede WiFi como ponto de acesso
    // const char* SSID = document["ssid"] | "ORIBot002";
    // const char* PASSWORD = document["password"] | "l9110ura";
    // WiFi.softAP(SSID, PASSWORD);
    // Serial.println("WiFi AP started!");
    // Serial.println(WiFi.softAPIP());
    // Configuração da rede WiFi para conexão externa
    // Inicialização dos endpoints HTTP e FTP
    WebHandlers::setupAll();
    server.begin();
    // ftpSrv.begin(SSID, PASSWORD);
    ftpSrv.begin(ssid , password);
    MQTTHandler::setupMQTT(MQTTHandler::mqttCallback);
}

void loop() {
    delay(1);
    ftpSrv.handleFTP();
    MQTTHandler::handleMQTT();
    processMPU1();
    processMPU2();
    // ===================================================================================
    if (millis() - lastYawReadTime > 100) {
        lastYawReadTime = millis();
        compass.read();
        float angulo_atual = compass.getAzimuth();
        yaw = angulo_atual - yaw_offset;
        if (yaw < 0) {
            yaw += 360;
        }
        Serial.print("Yaw do QMC5883L: ");
        Serial.println(yaw, 2);
    }
    if (millis() % 10UL == 0UL) {
        MQTTHandler::publishSensorData();
    }
    static unsigned long timeDebug = 0;
    if ((millis() - timeDebug) > 500UL) {
        timeDebug = millis();
        printDebug();
    }
    //  ===================================================================================
}