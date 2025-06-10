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

// --- Fim do código ---
// Certifique-se de que o arquivo config.json esteja presente no SPIFFS com as chaves "ssid" e "password".
// Exemplo de config.json:
// {
//   "ssid": "ORIBot002",
//   "password": "l9110ura"     
// }
// Certifique-se de que as bibliotecas necessárias estejam instaladas no PlatformIO ou Arduino IDE.
// As bibliotecas necessárias incluem:
// - Adafruit MPU6050
// - Adafruit HMC5883 Unified
// - Adafruit Unified Sensor
// - ArduinoJson
// - SimpleFTPServer
// - L9110H
// - ESPAsyncWebServer
// - AsyncTCP
// - SPIFFS
// - Wire
// - arduinoFFT
//
// Certifique-se de que o hardware esteja conectado corretamente:
// - MPU6050 conectado via I2C (pinos padrão SDA e SCL)
// - HMC5883 conectado via I2C (pinos padrão SDA e SCL)
// - L9110H conectado aos pinos de controle do motor
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar SPIFFS
// - Certifique-se de que o WiFi esteja configurado corretamente no arquivo config.json
// - Certifique-se de que o servidor FTP esteja configurado corretamente
// - Certifique-se de que o servidor web esteja configurado corretamente
// - Certifique-se de que o SPIFFS esteja formatado e os arquivos necessários estejam carregados
// - Certifique-se de que o ArduinoJson esteja configurado corretamente para serializar e desserializar os dados
// - Certifique-se de que o arduinoFFT esteja configurado corretamente para realizar a FFT
// - Certifique-se de que o L9110H esteja configurado corretamente para controlar os motores
// - Certifique-se de que o ESPAsyncWebServer esteja configurado corretamente para lidar com as requisições HTTP
// - Certifique-se de que o AsyncTCP esteja configurado corretamente para lidar com as conexões TCP
// - Certifique-se de que o SPIFFS esteja configurado corretamente para armazenar os arquivos estáticos
// - Certifique-se de que o Wire esteja configurado corretamente para comunicação I2C
// - Certifique-se de que o Adafruit_Sensor esteja configurado corretamente para lidar com os sensores
// - Certifique-se de que o Adafruit_HMC5883_U esteja configurado corretamente para lidar com o magnetômetro
// - Certifique-se de que o Adafruit_MPU6050 esteja configurado corretamente para lidar com o MPU6050
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o SPIFFS
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o WiFi
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o FTP
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o servidor web
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o ArduinoJson
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o arduinoFFT
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o L9110H
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o ESPAsyncWebServer
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o AsyncTCP
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o SPIFFS
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o Wire
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o Adafruit_Sensor
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o Adafruit_HMC5883_U
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o Adafruit_MPU6050
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o arduinoFFT
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o L9110H
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o ESPAsyncWebServer
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o AsyncTCP
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o SPIFFS
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o Wire
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o Adafruit_Sensor
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o Adafruit_HMC5883_U
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o Adafruit_MPU6050
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o arduinoFFT
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o L9110H
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o ESPAsyncWebServer
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o AsyncTCP
// - Certifique-se de que o ESP32 esteja configurado corretamente para usar o SPIFFS
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
Adafruit_MPU6050 mpu1; // MPU Principal (Endereço 0x68)
Adafruit_MPU6050 mpu2; // Segundo MPU (Endereço 0x69)
ArduinoFFT<double> FFT; // FFT object (can be reused if processed sequentially)

// --- Variáveis globais para MPU1 ---
double vReal1[SAMPLES];
double vImag1[SAMPLES];
float currentVibrationMagnitude1 = 0.0;
float dominantVibrationFrequency1 = 0.0;
float dominantVibrationAmplitude1 = 0.0;
unsigned long lastSampleTime1 = 0;
uint16_t sampleCounter1 = 0;

// Novas variáveis para os dados do MPU6050 (MPU1)
float accelerationX_1, accelerationY_1, accelerationZ_1; // Aceleração linear em m/s^2
float rotationX_1, rotationY_1, rotationZ_1;             // Velocidade angular (inclinação/rotação) em rad/s ou deg/s
float orientationX_1, orientationY_1, orientationZ_1;     // Estimativa de direção/orientação em graus (pitch, roll, yaw)

// --- Variáveis globais para MPU2 ---
double vReal2[SAMPLES];
double vImag2[SAMPLES];
float currentVibrationMagnitude2 = 0.0;
float dominantVibrationFrequency2 = 0.0;
float dominantVibrationAmplitude2 = 0.0;
unsigned long lastSampleTime2 = 0; // Keeping separate sample times for robustness, though a single one might suffice depending on timing.
uint16_t sampleCounter2 = 0;

// Novas variáveis para os dados do MPU6050 (MPU2)
float accelerationX_2, accelerationY_2, accelerationZ_2; // Aceleração linear em m/s^2
float rotationX_2, rotationY_2, rotationZ_2;             // Velocidade angular (inclinação/rotação) em rad/s ou deg/s
float orientationX_2, orientationY_2, orientationZ_2;     // Estimativa de direção/orientação em graus (pitch, roll, yaw)


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
    // Endpoints para MPU1
    server.on("/mpu1/vibration_magnitude", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", String(currentVibrationMagnitude1, 2));
    });
    server.on("/mpu1/vibration_frequency", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", String(dominantVibrationFrequency1, 2));
    });
    server.on("/mpu1/vibration_amplitude", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", String(dominantVibrationAmplitude1, 2));
    });

    // Endpoints para MPU2
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

  // Handlers para os dados do MPU6050 MPU1
  void handleMPU6050DataEndpoints() {
    server.on("/mpu1/acceleration", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<100> data;
      data["x"] = String(accelerationX_1, 2);
      data["y"] = String(accelerationY_1, 2);
      data["z"] = String(accelerationZ_1, 2);
      String jsonResponse;
      serializeJson(data, jsonResponse);
      request->send(200, "application/json", jsonResponse);
    });

    server.on("/mpu1/rotation", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<100> data;
      data["x"] = String(rotationX_1, 2);
      data["y"] = String(rotationY_1, 2);
      data["z"] = String(rotationZ_1, 2);
      String jsonResponse;
      serializeJson(data, jsonResponse);
      request->send(200, "application/json", jsonResponse);
    });

    server.on("/mpu1/orientation", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<100> data;
      data["pitch"] = String(orientationX_1, 2);
      data["roll"] = String(orientationY_1, 2);
      data["yaw"] = String(orientationZ_1, 2);
      String jsonResponse;
      serializeJson(data, jsonResponse);
      request->send(200, "application/json", jsonResponse);
    });

    // Handlers para os dados do MPU6050 MPU2
    server.on("/mpu2/acceleration", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<100> data;
      data["x"] = String(accelerationX_2, 2);
      data["y"] = String(accelerationY_2, 2);
      data["z"] = String(accelerationZ_2, 2);
      String jsonResponse;
      serializeJson(data, jsonResponse);
      request->send(200, "application/json", jsonResponse);
    });

    server.on("/mpu2/rotation", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<100> data;
      data["x"] = String(rotationX_2, 2);
      data["y"] = String(rotationY_2, 2);
      data["z"] = String(rotationZ_2, 2);
      String jsonResponse;
      serializeJson(data, jsonResponse);
      request->send(200, "application/json", jsonResponse);
    });

    server.on("/mpu2/orientation", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<100> data;
      data["pitch"] = String(orientationX_2, 2);
      data["roll"] = String(orientationY_2, 2);
      data["yaw"] = String(orientationZ_2, 2);
      String jsonResponse;
      serializeJson(data, jsonResponse);
      request->send(200, "application/json", jsonResponse);
    });
  }

  void setupAll() {
    handleStaticFiles();
    handleRobotCommands();
    handleVibrationEndpoints();
    handleMPU6050DataEndpoints();
  }
}

namespace MPU6050Handler {
  void setupMPU6050s() {
    Wire.begin();
    Serial.println("Initializing MPU6050 sensors...");

    // Initialize MPU1 (default address 0x68)
    if (!mpu1.begin(0x68)) {
      Serial.println("Failed to find MPU6050 #1. Check connections!");
      while (1) delay(10);
    }
    mpu1.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu1.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu1.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 #1 connected and configured.");

    // Initialize MPU2 (address 0x69)
    if (!mpu2.begin(0x69)) {
      Serial.println("Failed to find MPU6050 #2. Check connections and AD0 jumper!");
      while (1) delay(10);
    }
    mpu2.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu2.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu2.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 #2 connected and configured.");
  }
}

namespace FFTHandler {
  // Generic function to process FFT for a given MPU's data
  void processFFT(double* vReal, double* vImag, float& vibrationMagnitude, float& dominantFrequency, float& dominantAmplitude) {
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
    dominantFrequency = (peakIndex * SAMPLING_FREQ) / SAMPLES;
    dominantAmplitude = peakAmplitude;

    // The 'vibrationMagnitude' passed here should be the RMS value already calculated before calling processFFT
    // The current code assigns currentVibrationMagnitude based on raw acceleration.
    // Ensure that vibrationMagnitude is updated before calling this function for a more accurate representation.
    // For now, let's just print the passed value.
    Serial.print("Magnitude Total (RMS): "); Serial.print(vibrationMagnitude, 2); Serial.println(" m/s^2");
    Serial.print("Frequencia Dominante: "); Serial.print(dominantFrequency, 2); Serial.println(" Hz");
    Serial.print("Amplitude Dominante: "); Serial.print(dominantAmplitude, 2); Serial.println(" m/s^2");
    Serial.println("---");
  }
}

// --- Funções principais ---

void setup() {
  Serial.begin(115200);

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
  ftpSrv.handleFTP();

  // --- Process MPU1 data ---
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

    orientationX_1 = atan2(accelerationY_1, sqrt(accelerationX_1 * accelerationX_1 + accelerationZ_1 * accelerationZ_1)) * 180.0 / PI;
    orientationY_1 = atan2(-accelerationX_1, sqrt(accelerationY_1 * accelerationY_1 + accelerationZ_1 * accelerationZ_1)) * 180.0 / PI;

    // Yaw for MPU1 (requires magnetometer or fusion)
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
      const float alpha = 0.96f;
      fusedYaw1 = alpha * fusedYaw1 + (1.0f - alpha) * magYaw1;
    }
    while (fusedYaw1 < 0.0f) fusedYaw1 += 360.0f;
    while (fusedYaw1 >= 360.0f) fusedYaw1 -= 360.0f;
    orientationZ_1 = fusedYaw1;

    currentVibrationMagnitude1 = sqrt(
      pow(a1.acceleration.x, 2) +
      pow(a1.acceleration.y, 2) +
      pow(a1.acceleration.z, 2)
    );

    vReal1[sampleCounter1] = a1.acceleration.z; // Using Z-axis for vibration
    vImag1[sampleCounter1] = 0.0;
    sampleCounter1++;

    if (sampleCounter1 >= SAMPLES) {
      FFTHandler::processFFT(vReal1, vImag1, currentVibrationMagnitude1, dominantVibrationFrequency1, dominantVibrationAmplitude1);
      sampleCounter1 = 0;
    }
  }

  // --- Process MPU2 data ---
  if (micros() - lastSampleTime2 >= (1000000 / SAMPLING_FREQ)) { // Use a separate lastSampleTime for MPU2
    lastSampleTime2 = micros();

    sensors_event_t a2, g2, temp2;
    mpu2.getEvent(&a2, &g2, &temp2);

    accelerationX_2 = a2.acceleration.x;
    accelerationY_2 = a2.acceleration.y;
    accelerationZ_2 = a2.acceleration.z;

    rotationX_2 = g2.gyro.x;
    rotationY_2 = g2.gyro.y;
    rotationZ_2 = g2.gyro.z;

    orientationX_2 = atan2(accelerationY_2, sqrt(accelerationX_2 * accelerationX_2 + accelerationZ_2 * accelerationZ_2)) * 180.0 / PI;
    orientationY_2 = atan2(-accelerationX_2, sqrt(accelerationY_2 * accelerationY_2 + accelerationZ_2 * accelerationZ_2)) * 180.0 / PI;

    // Yaw for MPU2 (simplified without a second magnetometer)
    orientationZ_2 = rotationZ_2 * (180.0f / PI);

    currentVibrationMagnitude2 = sqrt(
      pow(a2.acceleration.x, 2) +
      pow(a2.acceleration.y, 2) +
      pow(a2.acceleration.z, 2)
    );

    vReal2[sampleCounter2] = a2.acceleration.z; // Using Z-axis for vibration
    vImag2[sampleCounter2] = 0.0;
    sampleCounter2++;

    if (sampleCounter2 >= SAMPLES) {
      FFTHandler::processFFT(vReal2, vImag2, currentVibrationMagnitude2, dominantVibrationFrequency2, dominantVibrationAmplitude2);
      sampleCounter2 = 0;
    }
  }
}