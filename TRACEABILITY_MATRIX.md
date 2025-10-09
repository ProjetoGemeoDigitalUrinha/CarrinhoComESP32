
# Matriz de Rastreabilidade

| ID | Requisito / Descrição | Arquivo(s) | Função(s) / Símbolos | Observações |
|---:|---|---|---|---|
| R1 | Ler configuração JSON do SPIFFS | `src/Config.cpp` | `loadConfig(StaticJsonDocument<200>&)` | Abre `/config.json` via SPIFFS; registra erro em Serial se falha. Usa `ArduinoJson`. |
| R2 | Inicializar e configurar WiFi (conexão e mDNS) | `src/Network.cpp`, `src/main.cpp` | `Network::connectWiFi`, `Network::beginMDNS`, chamadas em `setup()` | `Network::connectWiFi` implementa timeout; `main.cpp` chama `Network::connectWiFi` e `Network::beginMDNS`. `main.cpp` também usa `WiFi.begin` diretamente em alguns trechos. |
| R3 | Fornecer API HTTP para arquivos estáticos e comandos do robô | `src/WebHandlers.cpp`, `data/` | `WebHandlers::setupAll`, várias rotas em `handleStaticFiles`, `handleRobotCommands` | Serve `index.html`, assets e endpoints `/frt`, `/trs`, `/esq`, `/drt`, `/stop`, `/action`. Arquivos estáticos estão em `data/`. Usa `ESPAsyncWebServer`. |
| R4 | Controlar motores via driver L9110H | `src/L9110H.cpp`, `include/L9110H.h` | `L9110H::motor`, `L9110H::forward`, `backward`, `left`, `right`, `stop` | Classe `L9110H` encapsula lógica de direção e PWM (usa `analogWrite`). Valores de pinos definidos dinamicamente por `motor()` (pode ser melhorado). |
| R5 | Ler encoders e calcular RPM | `src/Encoders.cpp`, `include/Encoders.h`, `src/main_globals.cpp` | ISRs `isrEncoderDir`, `isrEncoderEsq`, `calcularRPMRodas`, `setupEncoders` | Usa interrupções em pinos 26 e 34; PPR = 20; cálculo de RPM a cada 100 ms. Variáveis globais: `encoderDirCount`, `encoderEsqCount`, `rpmDir`, `rpmEsq`. |
| R6 | Ler distância ultrassônica | `src/Ultrasonic.cpp`, `include/Ultrasonic.h` | `setupUltrasonic`, `readUltrasonicDistance` | Usa TRIG_PIN=5, ECHO_PIN=18; `pulseIn` com timeout; distância em `distance_cm`. |
| R7 | Inicializar e ler sensores MPU6050 (dois) | `src/MPU6050Handler.cpp`, `src/MPUProcessing.cpp`, `include/MPU6050Handler.h` | `MPU6050Handler::setupMPU6050s`, `processMPU1`, `processMPU2` | Cria dois objetos `Adafruit_MPU6050 mpu1, mpu2` em `main_globals.cpp`. `setup` usa `mpu1.begin()` e `mpu2.begin(105)`. Processamento calcula orientação (pitch/roll/yaw) e popula arrays para FFT. |
| R8 | Calcular FFT para análise de vibração | `src/FFTHandler.cpp`, `include/FFTHandler.h`, `src/MPUProcessing.cpp` | `FFTHandler::processFFT` | Usa `arduinoFFT` para janela, FFT e detecção de pico. Constantes `SAMPLES=64`, `SAMPLING_FREQ=100.0` definidas em `main_globals.cpp`. Produz `dominantFrequency` e `dominantAmplitude`. |
| R9 | Publicar dados e status por MQTT | `src/MQTTHandler.cpp`, `include/MQTTHandler.h`, `src/main_globals.cpp` | `MQTTHandler::setupMQTT`, `handleMQTT`, `publishSensorData`, `publishStatus`, `mqttCallback` | Usa `PubSubClient` e `WiFiClient`. Variáveis MQTT em `main_globals.cpp` (`mqtt_server`, `mqtt_port`, `mqtt_user`, `mqtt_password`, tópicos). `mqttCallback` aceita comandos textuais (`frt`, `trs`, `esq`, `drt`, `stop`) e aciona `robot`. `handleMQTT` reconecta e publica status. |
| R10 | Servidor FTP para arquivos | `src/main.cpp`, `data/` | `FtpServer ftpSrv; ftpSrv.begin(...)`, `ftpSrv.handleFTP()` | Usa `SimpleFTPServer`. Iniciado em `setup()` e processado em `loop()`. Credenciais hard-coded em `main.cpp`. |
| R11 | Fornecer endpoints para leituras de sensores (HTTP JSON/text) | `src/WebHandlers.cpp`, `src/MPUProcessing.cpp`, `src/Ultrasonic.cpp`, `src/Encoders.cpp` | Rotas como `/mpu1/acceleration`, `/mpu1/vibration_magnitude`, `/distance`, `/rpm_dir` | Endpoints retornam `String` ou JSON serializado com `ArduinoJson`. |
| R12 | Depuração via serial com resumo periódico | `src/Debug.cpp`, `src/main.cpp` | `printDebug()`, uso potencial em `loop()` | `printDebug()` imprime RPM, distância, orientação/vibração dos MPUs; `main.cpp` tem lógica de temporização para prints (variáveis `DEBUG_PRINT_INTERVAL_MS`). |
| R13 | Leitura/calibração de bússola QMC5883L para yaw | `src/main.cpp` (trechos), `src/main_globals.cpp` | Uso de `QMC5883LCompass compass; compass.init(); compass.setCalibration(...); compass.getAzimuth()` | `main.cpp` contém inicialização e calibração com valores fixos e offset de yaw. |
| R14 | Arquitetura: variáveis globais e objetos compartilhados | `src/main_globals.cpp`, diversos includes | Declara `mpu1`, `mpu2`, arrays de FFT, contadores, credenciais, etc. | Centraliza estado compartilhado para sensores, MQTT e rede. |

## Observações gerais e potenciais riscos / melhorias
- Vários valores sensíveis (WiFi SSID/senha, MQTT credenciais, FTP credenciais) estão hard-coded em `main_globals.cpp` e `main.cpp` e também existe suporte para `config.json` em SPIFFS (`loadConfig`) — seria bom unificar e priorizar `config.json`.
- `L9110H::L9110H()` usa `pinMode(pinA, OUTPUT)` e `pinA` é definido só dentro de `motor()`; inicialização de pinos no construtor pode referenciar pinos antes de serem configurados. Recomendo mover a definição de pinos para o construtor ou aceitar pinos no construtor.
- `Config::loadConfig` não valida campos esperados do JSON; considere adicionar verificações e valores defaults.
- `Network::connectWiFi` introduz timeout argument (implementado) enquanto chamadas em `main.cpp` usam `WiFi.begin` direto; unificar para usar `Network::connectWiFi` com timeout é mais consistente.
- Interrupções dos encoders usam pinos 26 e 34; garantir que correspondem ao hardware (pinos de interrupção no ESP32).
- `analogWrite` no ESP32 pode precisar de configuração de PWM (ledc) dependendo da plataforma; verificar compatibilidade da função usada.

## Como usar / onde colocar o arquivo
- Este arquivo está no projeto como `TRACEABILITY_MATRIX.md` (raiz). Mantê-lo atualizado quando alterar requisitos ou mover código.

## Status
- Gerar matriz: concluído