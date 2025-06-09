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

// --- Constantes e configurações ---
constexpr uint16_t SAMPLES = 256;           // Número de amostras para FFT (potência de 2)
constexpr double SAMPLING_FREQ = 500.0;     // Frequência de amostragem em Hz

// --- Objetos globais ---
L9110H robot;
AsyncWebServer server(80);
FtpServer ftpSrv;
StaticJsonDocument<200> document;
Adafruit_MPU6050 mpu;
ArduinoFFT<double> FFT;

// --- Variáveis globais ---
double vReal[SAMPLES];
double vImag[SAMPLES];
float currentVibrationMagnitude = 0.0;
float dominantVibrationFrequency = 0.0;
float dominantVibrationAmplitude = 0.0;
unsigned long lastSampleTime = 0;
uint16_t sampleCounter = 0;

// Novas variáveis para os dados do MPU6050
float accelerationX, accelerationY, accelerationZ; // Aceleração linear em m/s^2
float rotationX, rotationY, rotationZ;             // Velocidade angular (inclinação/rotação) em rad/s ou deg/s
float orientationX, orientationY, orientationZ;     // Estimativa de direção/orientação em graus (pitch, roll, yaw)

// --- Encapsulamento de funcionalidades ---

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
    server.on("/vibration_magnitude", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", String(currentVibrationMagnitude, 2));
    });
    server.on("/vibration_frequency", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", String(dominantVibrationFrequency, 2));
    });
    server.on("/vibration_amplitude", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", String(dominantVibrationAmplitude, 2));
    });
  }

  // Novos handlers para os dados do MPU6050
  void handleMPU6050DataEndpoints() {
    server.on("/acceleration", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<100> data;
      data["x"] = String(accelerationX, 2);
      data["y"] = String(accelerationY, 2);
      data["z"] = String(accelerationZ, 2);
      String jsonResponse;
      serializeJson(data, jsonResponse);
      request->send(200, "application/json", jsonResponse);
    });

    server.on("/rotation", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<100> data;
      data["x"] = String(rotationX, 2);
      data["y"] = String(rotationY, 2);
      data["z"] = String(rotationZ, 2);
      String jsonResponse;
      serializeJson(data, jsonResponse);
      request->send(200, "application/json", jsonResponse);
    });

    server.on("/orientation", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<100> data;
      data["pitch"] = String(orientationX, 2); // Pitch (inclinação para frente/trás)
      data["roll"] = String(orientationY, 2);  // Roll (inclinação para os lados)
      data["yaw"] = String(orientationZ, 2);   // Yaw (guinada/direção horizontal) - mais complexo de obter só com acelerômetro/giroscópio
      String jsonResponse;
      serializeJson(data, jsonResponse);
      request->send(200, "application/json", jsonResponse);
    });
  }

  void setupAll() {
    handleStaticFiles();
    handleRobotCommands();
    handleVibrationEndpoints();
    handleMPU6050DataEndpoints(); // Chamar os novos handlers
  }
}

namespace MPU6050Handler {
  void setupMPU6050() {
    Wire.begin();
    if (!mpu.begin()) {
      Serial.println("Falha ao encontrar MPU6050. Verifique as conexoes!");
      while (1) delay(10);
    }
    // Configurações do MPU6050 (podem ser ajustadas conforme a necessidade)
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 conectado e configurado.");
  }
}

namespace FFTHandler {
  void processFFT() {
    // FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Opcional: janela Hamming
    FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.complexToMagnitude(vReal, vImag, SAMPLES);

    double peakAmplitude = 0.0;
    int peakIndex = 0;
    for (int i = 1; i < (SAMPLES / 2); i++) {
      double currentAmplitude = vReal[i] * (2.0 / SAMPLES);
      if (currentAmplitude > peakAmplitude) {
        peakAmplitude = currentAmplitude;
        peakIndex = i;
      }
    }
    dominantVibrationFrequency = (peakIndex * SAMPLING_FREQ) / SAMPLES;
    dominantVibrationAmplitude = peakAmplitude;

    Serial.print("Magnitude Total (RMS): "); Serial.print(currentVibrationMagnitude, 2); Serial.println(" m/s^2");
    Serial.print("Frequencia Dominante: "); Serial.print(dominantVibrationFrequency, 2); Serial.println(" Hz");
    Serial.print("Amplitude Dominante: "); Serial.print(dominantVibrationAmplitude, 2); Serial.println(" m/s^2");
    Serial.println("---");
  }
}

// --- Funções principais ---

void setup() {
  Serial.begin(115200);

  MPU6050Handler::setupMPU6050();

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
  ftpSrv.handleFTP();

  if (micros() - lastSampleTime >= (1000000 / SAMPLING_FREQ)) {
    lastSampleTime = micros();

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Atualiza as variáveis de aceleração
    accelerationX = a.acceleration.x;
    accelerationY = a.acceleration.y;
    accelerationZ = a.acceleration.z;

    // Atualiza as variáveis de rotação (velocidade angular)
    rotationX = g.gyro.x; // Velocidade angular em rad/s (por padrão na Adafruit_MPU6050)
    rotationY = g.gyro.y;
    rotationZ = g.gyro.z;

    // Calculando a orientação (pitch e roll) a partir do acelerômetro
    // Estes são os ângulos de inclinação (pitch) e rolamento (roll)
    // baseados na gravidade. O yaw (direção horizontal) requer um magnetômetro
    // ou um algoritmo de fusão de sensores mais complexo.
    orientationX = atan2(accelerationY, sqrt(accelerationX * accelerationX + accelerationZ * accelerationZ)) * 180.0 / PI; // Pitch
    orientationY = atan2(-accelerationX, sqrt(accelerationY * accelerationY + accelerationZ * accelerationZ)) * 180.0 / PI; // Roll
    // --- Cálculo aprimorado da guinada (Yaw) com correção por magnetômetro e filtro complementar ---
    static Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
    static bool magInitialized = false;
    static float fusedYaw = 0.0f;
    static unsigned long lastYawTime = 0;

    // Inicializa o magnetômetro apenas uma vez
    if (!magInitialized) {
      if (mag.begin()) {
      magInitialized = true;
      Serial.println("Magnetômetro inicializado.");
      } else {
      Serial.println("Magnetômetro não encontrado!");
      }
    }

    // Calcula o delta de tempo
    unsigned long now = micros();
    float dt = (lastYawTime == 0) ? 0.0f : (now - lastYawTime) / 1e6f;
    lastYawTime = now;

    // Leitura do magnetômetro
    float magYaw = NAN;
    if (magInitialized) {
      sensors_event_t mag_event;
      mag.getEvent(&mag_event);
      magYaw = atan2(mag_event.magnetic.y, mag_event.magnetic.x);
      if (magYaw < 0) magYaw += 2 * PI;
      magYaw = magYaw * 180.0f / PI; // graus
    }

    // Integração do giroscópio para estimativa incremental do yaw
    if (dt > 0 && fabs(rotationZ) < 100.0f) {
      fusedYaw += rotationZ * dt * 180.0f / PI; // rad/s * s -> graus
    }

    // Filtro complementar: combina giroscópio e magnetômetro para reduzir drift
    if (!isnan(magYaw)) {
      const float alpha = 0.96f; // Peso do giroscópio (ajuste conforme necessário)
      fusedYaw = alpha * fusedYaw + (1.0f - alpha) * magYaw;
    }

    // Mantém yaw no intervalo [0, 360)
    while (fusedYaw < 0.0f) fusedYaw += 360.0f;
    while (fusedYaw >= 360.0f) fusedYaw -= 360.0f;

    orientationZ = fusedYaw; // Guinada corrigida em graus

    currentVibrationMagnitude = sqrt(
      pow(a.acceleration.x, 2) +
      pow(a.acceleration.y, 2) +
      pow(a.acceleration.z, 2)
    );

    vReal[sampleCounter] = a.acceleration.z;
    vImag[sampleCounter] = 0.0;
    sampleCounter++;

    if (sampleCounter >= SAMPLES) {
      FFTHandler::processFFT();
      sampleCounter = 0;
    }
  }
}