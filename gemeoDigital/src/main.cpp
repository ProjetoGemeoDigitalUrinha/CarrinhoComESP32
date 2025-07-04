#include <Arduino.h>
#include <SimpleFTPServer.h>
#include <ArduinoJson.h>
#include "L9110H.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "AsyncTCP.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_HMC5883_U.h>
#include <Adafruit_Sensor.h>
#include <arduinoFFT.h>

// --- PARÂMETROS FÍSICOS ---
const float DIAMETRO_RODA_CM = 6.5;
const int PPR_ENCODER = 20;
const int GEAR_RATIO = 48;

// --- FATOR DE CONVERSÃO ---
const float DISTANCIA_POR_PULSO_CM = (DIAMETRO_RODA_CM * PI) / (PPR_ENCODER * GEAR_RATIO);

// --- PARÂMETROS DE AJUSTE MANUAL DOS MPUS ---
const float PITCH_MULTIPLIER_1 = 1.0;
const float ROLL_MULTIPLIER_1  = 1.0;
const float YAW_MULTIPLIER_1   = 1.0;
const float PITCH_OFFSET_1 = -3.6;
const float ROLL_OFFSET_1  = 1.3;
const float YAW_OFFSET_1   = 0.0;
const float PITCH_MULTIPLIER_2 = -1.0;
const float ROLL_MULTIPLIER_2  = -1.0;
const float YAW_MULTIPLIER_2   = -1.0;
const float PITCH_OFFSET_2 = 1.0;
const float ROLL_OFFSET_2  = -0.8;
const float YAW_OFFSET_2   = 0.0;

// --- SENSOR ULTRASSÔNICO ---
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;

// --- CONSTANTES E CONFIGURAÇÕES ---
constexpr uint16_t SAMPLES = 256;
constexpr double SAMPLING_FREQ = 1000.0;

// --- PINOS DOS ENCODERS (1 CANAL) ---
#define ENCODER_DIR_PIN 34
#define ENCODER_ESQ_PIN 26

// --- OBJETOS GLOBAIS ---
L9110H robot;
AsyncWebServer server(80);
FtpServer ftpSrv;
StaticJsonDocument<200> document;
Adafruit_MPU6050 mpu1;
Adafruit_MPU6050 mpu2;
ArduinoFFT<double> FFT;

// --- VARIÁVEIS GLOBAIS PARA OS ENCODERS (1 CANAL) ---
volatile long encoderDirCount = 0;
volatile long encoderEsqCount = 0;

// --- VARIÁVEIS GLOBAIS PARA O MPU 1 (0x68)---
double vReal1[SAMPLES];
double vImag1[SAMPLES];
float currentVibrationMagnitude1 = 0.0;
float dominantVibrationFrequency1 = 0.0;
float dominantVibrationAmplitude1 = 0.0;
unsigned long lastSampleTime1 = 0;
uint16_t sampleCounter1 = 0;
float accelerationX_1, accelerationY_1, accelerationZ_1;
float rotationX_1, rotationY_1, rotationZ_1;
float orientationX_1, orientationY_1, orientationZ_1;

// --- VARIÁVEIS GLOBAIS PARA O MPU 2 (0x69) ---
double vReal2[SAMPLES];
double vImag2[SAMPLES];
float currentVibrationMagnitude2 = 0.0;
float dominantVibrationFrequency2 = 0.0;
float dominantVibrationAmplitude2 = 0.0;
unsigned long lastSampleTime2 = 0;
uint16_t sampleCounter2 = 0;
float accelerationX_2, accelerationY_2, accelerationZ_2;
float rotationX_2, rotationY_2, rotationZ_2;
float orientationX_2, orientationY_2, orientationZ_2;

// --- VARIÁVEIS GLOBAIS PARA O SENSOR ULTRASSÔNICO ---
float distance_cm = 0.0;
unsigned long last_ultrasonic_read_time = 0;

// --- FUNÇÕES DE INTERRUPÇÃO (ISR) PARA OS ENCODERS ---
void IRAM_ATTR isrEncoderDir() {
  encoderDirCount++;
}

void IRAM_ATTR isrEncoderEsq() {
  encoderEsqCount++;
}

// --- FUNÇÃO PARA LER O SENSOR ULTRASSÔNICO ---
void readUltrasonicDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 25000);
    distance_cm = (duration * 0.0343) / 2.0;
    if (distance_cm > 400 || distance_cm <= 0) {
        distance_cm = 0;
    }
}

// --- ENCAPSULAMENTO DE FUNCIONALIDADES ---
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

  void setupAll() {
    handleStaticFiles();
    handleRobotCommands();
    handleVibrationEndpoints();
    handleMPU6050DataEndpoints();
    handleUltrasonicEndpoint();
  }
}

namespace MPU6050Handler {
  void setupMPU6050s() {
    Wire.begin();
    Serial.println("Initializing MPU6050 sensors...");
    if (!mpu1.begin()) {
      Serial.println("Failed to find MPU6050 #1. Check connections!");
      while (1) delay(10);
    }
    mpu1.setAccelerometerRange(MPU6050_RANGE_8_G); mpu1.setGyroRange(MPU6050_RANGE_500_DEG); mpu1.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 #1 connected and configured.");
    if (!mpu2.begin((uint8_t)105U)) {
      Serial.println("Failed to find MPU6050 #2. Check connections and AD0 jumper!");
      while (1) delay(10);
    }
    mpu2.setAccelerometerRange(MPU6050_RANGE_8_G); mpu2.setGyroRange(MPU6050_RANGE_500_DEG); mpu2.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 #2 connected and configured.");
  }
}

namespace FFTHandler {
  void processFFT(double* vReal, double* vImag, float& vibrationMagnitude, float& dominantFrequency, float& dominantAmplitude) {
    FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.complexToMagnitude(vReal, vImag, SAMPLES);
    double peakAmplitude = 0.0;
    int peakIndex = 0;
    for (int i = 1; i < (SAMPLES/2); i++) {
      double currentAmplitude = vReal[i]*(2.0/SAMPLES);
      if (currentAmplitude > peakAmplitude) {
        peakAmplitude = currentAmplitude;
        peakIndex = i;
      }
    }
    dominantFrequency = (peakIndex*SAMPLING_FREQ)/SAMPLES;
    dominantAmplitude = peakAmplitude;
    Serial.print("Magnitude de Vibração Total (RMS): "); Serial.print(vibrationMagnitude, 2); Serial.println(" m/s^2");
    Serial.print("Frequencia de Vibração Dominante: "); Serial.print(dominantFrequency, 2); Serial.println(" Hz");
    Serial.print("Amplitude de Vibração Dominante: "); Serial.print(dominantAmplitude, 2); Serial.println(" m/s^2");
  }
}

// --- FUNÇÕES PRINCIPAIS ---
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("Ultrasonic sensor pins configured.");

  pinMode(ENCODER_DIR_PIN, INPUT);
  pinMode(ENCODER_ESQ_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DIR_PIN), isrEncoderDir, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_ESQ_PIN), isrEncoderEsq, RISING);
  Serial.println("Encoders configurados.");
  
  MPU6050Handler::setupMPU6050s();
  
  if (!SPIFFS.begin(true)) {
    Serial.println("Can't initialize SPIFFS, please restart and try again.");
    return;
  }
  
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
  
  const char* SSID = document["ssid"] | "ORIBot002";
  const char* PASSWORD = document["password"] | "l9110ura";
  WiFi.softAP(SSID, PASSWORD);
  Serial.println("WiFi AP started!");
  Serial.println(WiFi.softAPIP());
  
  WebHandlers::setupAll();
  server.begin();
  ftpSrv.begin(SSID, PASSWORD);
}

void loop() {
  delay(1); 
  ftpSrv.handleFTP();

  if (millis() - last_ultrasonic_read_time > 250) {
      last_ultrasonic_read_time = millis();
      readUltrasonicDistance();
  }

  if (micros() - lastSampleTime1 >= (1000000 / SAMPLING_FREQ)) {
    lastSampleTime1 = micros();
    sensors_event_t a1, g1, temp1;
    mpu1.getEvent(&a1, &g1, &temp1);
    accelerationX_1 = a1.acceleration.x;
    accelerationY_1 = a1.acceleration.y;
    accelerationZ_1 = a1.acceleration.z;
    rotationX_1 = g1.gyro.x;
    rotationY_1 = g1.gyro.y;
    rotationZ_1 = g1.gyro.z;
    float raw_pitch_1 = atan2(-accelerationX_1, sqrt(accelerationY_1 * accelerationY_1 + accelerationZ_1 * accelerationZ_1)) * 180.0 / PI;
    float raw_roll_1  = atan2(accelerationY_1, accelerationZ_1) * 180.0 / PI;
    orientationX_1 = (raw_pitch_1 * PITCH_MULTIPLIER_1) + PITCH_OFFSET_1;
    orientationY_1 = (raw_roll_1 * ROLL_MULTIPLIER_1) + ROLL_OFFSET_1;
    static Adafruit_HMC5883_Unified mag1 = Adafruit_HMC5883_Unified(12345);
    static bool magInitialized1 = false;
    static float fusedYaw1 = 0.0f;
    static unsigned long lastYawTime1 = 0;
    if (!magInitialized1) {
      if (mag1.begin()) {
        magInitialized1 = true;
        Serial.println("Magnetometer #1 initialized.");
      } else {
        Serial.println("Magnetometer #1 not found!");
      }
    }
    unsigned long now1 = micros();
    float dt1 = (lastYawTime1 == 0) ? 0.0f : (now1 - lastYawTime1) / 1e6f;
    lastYawTime1 = now1;
    float magYaw1 = NAN;
    if (magInitialized1) {
      sensors_event_t mag_event1;
      mag1.getEvent(&mag_event1);
      magYaw1 = atan2(mag_event1.magnetic.y, mag_event1.magnetic.x);
      if (magYaw1 < 0) magYaw1 += 2 * PI;
      magYaw1 = magYaw1 * 180.0f / PI;
    }
    if (dt1 > 0 && fabs(rotationZ_1) < 100.0f) {
      fusedYaw1 += rotationZ_1 * dt1 * 180.0f / PI;
    }
    if (!isnan(magYaw1)) {
      const float alpha = 0.98f;
      fusedYaw1 = alpha * fusedYaw1 + (1.0f - alpha) * magYaw1;
    }
    orientationZ_1 = (fusedYaw1 * YAW_MULTIPLIER_1) + YAW_OFFSET_1;
    while (orientationZ_1 < 0.0f) orientationZ_1 += 360.0f;
    while (orientationZ_1 >= 360.0f) orientationZ_1 -= 360.0f;
    currentVibrationMagnitude1 = sqrt(pow(a1.acceleration.x, 2) + pow(a1.acceleration.y, 2) + pow(a1.acceleration.z, 2));
    vReal1[sampleCounter1] = a1.acceleration.z;
    vImag1[sampleCounter1] = 0.0;
    sampleCounter1++;
    if (sampleCounter1 >= SAMPLES) {
      Serial.print("MPU 1: ");
      Serial.print("Pitch: "); Serial.print(orientationX_1, 2); Serial.print(" graus, ");
      Serial.print("Roll: "); Serial.print(orientationY_1, 2); Serial.print(" graus, ");
      Serial.print("Yaw: "); Serial.print(orientationZ_1, 2); Serial.println(" graus");
      FFTHandler::processFFT(vReal1, vImag1, currentVibrationMagnitude1, dominantVibrationFrequency1, dominantVibrationAmplitude1);
      sampleCounter1 = 0;
    }
  }

  if (micros() - lastSampleTime2 >= (1000000 / SAMPLING_FREQ)) {
    lastSampleTime2 = micros();
    sensors_event_t a2, g2, temp2;
    mpu2.getEvent(&a2, &g2, &temp2);
    accelerationX_2 = a2.acceleration.x;
    accelerationY_2 = a2.acceleration.y;
    accelerationZ_2 = a2.acceleration.z;
    rotationX_2 = g2.gyro.x;
    rotationY_2 = g2.gyro.y;
    rotationZ_2 = g2.gyro.z;
    float raw_pitch_2 = atan2(-accelerationX_2, sqrt(accelerationY_2 * accelerationY_2 + accelerationZ_2 * accelerationZ_2)) * 180.0 / PI;
    float raw_roll_2  = atan2(accelerationY_2, accelerationZ_2) * 180.0 / PI;
    orientationX_2 = (raw_pitch_2 * PITCH_MULTIPLIER_2) + PITCH_OFFSET_2;
    orientationY_2 = (raw_roll_2 * ROLL_MULTIPLIER_2) + ROLL_OFFSET_2;
    float raw_yaw_2 = rotationZ_2 * (180.0f / PI);
    orientationZ_2 = (raw_yaw_2 * YAW_MULTIPLIER_2) + YAW_OFFSET_2;
    currentVibrationMagnitude2 = sqrt(pow(a2.acceleration.x, 2) + pow(a2.acceleration.y, 2) + pow(a2.acceleration.z, 2));
    vReal2[sampleCounter2] = a2.acceleration.z;
    vImag2[sampleCounter2] = 0.0;
    sampleCounter2++;
    if (sampleCounter2 >= SAMPLES) {
      Serial.print("MPU 2: ");
      Serial.print("Pitch: "); Serial.print(orientationX_2, 2); Serial.print(" graus, ");
      Serial.print("Roll: "); Serial.print(orientationY_2, 2); Serial.print(" graus, ");
      Serial.print("Taxa do Yaw: "); Serial.print(orientationZ_2, 2); Serial.println(" m/s^2");
      FFTHandler::processFFT(vReal2, vImag2, currentVibrationMagnitude2, dominantVibrationFrequency2, dominantVibrationAmplitude2);
      
      // --- IMPRESSÃO DOS DADOS DE DISTÂNCIA ---
      noInterrupts();
      long currentDirCount = encoderDirCount;
      long currentEsqCount = encoderEsqCount;
      interrupts();
      float distanciaDir_cm = currentDirCount * DISTANCIA_POR_PULSO_CM;
      float distanciaEsq_cm = currentEsqCount * DISTANCIA_POR_PULSO_CM;
      Serial.print("Distância (Sonar): ");
      Serial.print(distance_cm, 2);
      Serial.println(" cm");
      Serial.print("Distância Direita (Encoder): ");
      Serial.print(distanciaDir_cm, 2);
      Serial.print(" cm | Distância Esquerda (Encoder): ");
      Serial.print(distanciaEsq_cm, 2);
      Serial.println(" cm");
      Serial.println("--------------------------------------------------");
      sampleCounter2 = 0;
    }
  }
}