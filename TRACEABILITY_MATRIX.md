# Matriz de rastreabilidade

Esta matriz relaciona as funcionalidades observadas no firmware aos seus pontos de implementação e ao estado de integração. O código atual é a fonte de verdade: uma função existente, mas não chamada pelo fluxo principal, não é considerada operacional de ponta a ponta.

O guia funcional e de operação está em [README.md](README.md).

## Estados

| Estado | Significado |
|---|---|
| **Implementado** | O caminho descrito está presente e conectado nos pontos relevantes do firmware. |
| **Parcial** | Há implementação, mas o fluxo tem limitações ou não está completo. |
| **Não integrado** | A lógica existe, mas não é chamada pelo fluxo ativo ou não produz o comportamento esperado. |
| **Divergente** | As partes envolvidas usam interfaces/configurações incompatíveis entre si. |

## Requisitos e funcionalidades

| ID | Requisito / funcionalidade | Implementação | Evidência funcional e observações | Estado |
|---|---|---|---|---|
| R01 | Compilar para ESP32-S3 com PlatformIO/Arduino | `platformio.ini` | Ambiente `esp32dev` com placa `esp32-s3-devkitc-1`, framework Arduino e dependências de web, MQTT, sensores, FFT e FTP. | **Implementado** |
| R02 | Inicializar sensores e periféricos | `src/main.cpp`, `src/MPU6050Handler.cpp`, `src/Encoders.cpp`, `src/Ultrasonic.cpp` | `setup()` inicializa I²C (SDA 8 / SCL 9), MPU6050 e pinos ultrassônicos (TRIG 13 / ECHO 14) antes de montar LittleFS; os encoders não existem no shield REV 4.0. Falha de MPU entra em espera infinita. | **Parcial** |
| R03 | Montar SPIFFS e ler configuração JSON | `src/main.cpp`, `src/Config.cpp`, `data/config.json` | SPIFFS é montado com formatação em caso de falha; `loadConfig()` abre `/config.json` e desserializa para documento estático de 200 bytes. Os valores do documento não são usados para configurar a rede. | **Parcial** |
| R04 | Conectar ao Wi-Fi | `src/main.cpp`, `src/Network.cpp`, `src/main_globals.cpp` | `Network::connectWiFi()` tem timeout de 10 s, mas seu retorno é ignorado e o `setup()` continua esperando conexão indefinidamente. Credenciais vêm de constantes compiladas. | **Parcial** |
| R05 | Disponibilizar hostname e serviço mDNS | `src/main.cpp`, `src/Network.cpp`, `src/main_globals.cpp` | Inicia hostname `twinesp32`; registra `_http`/`tcp` na porta 80, a mesma do servidor web. | **Implementado** |
| R06 | Servir a interface web estática | `src/WebHandlers.cpp`, `data/index.html`, `data/style.css`, `data/script.js`, `data/*.svg` | Arquivos são servidos do SPIFFS nas rotas raiz e assets. É necessário gravar o filesystem com os dados do diretório `data/`. | **Implementado** |
| R07 | Controlar avanço, ré, esquerda, direita e parada via WebSocket na rede local | `src/WebSocketControl.cpp`, `include/WebSocketControl.h`, `src/L9110H.cpp`, `data/index.html`, `data/script.js` | A página envia `frt`, `trs`, `esq`, `drt` e `stop` por `ws://<ip>/ws`. O movimento dura enquanto o botão estiver pressionado (reenvio a cada 150 ms), com parada automática após 500 ms sem comando ou quando o cliente desconecta. O funcionamento físico depende da compatibilidade do hardware/pinos. | **Implementado** |
| R08 | Controlar motores pelo driver L9110S | `src/L9110H.cpp`, `include/L9110H.h`, `include/Pins.h`, `src/main_globals.cpp` | Objeto global `robot`; `analogWrite()` nos pinos do shield REV 4.0: motor 1 = A1_A 7 / A1_B 6, motor 2 = B1_A 4 / B1_B 5. | **Implementado** |
| R09 | Executar ação predefinida `quadrado` pela web | `src/WebSocketControl.cpp`, `data/index.html`, `data/script.js` | O campo de comando envia `quadrado` pelo WebSocket; a sequência roda na tarefa dos motores sem bloquear e é interrompida por qualquer outro comando. | **Implementado** |
| R10 | Contar pulsos dos encoders | `src/Encoders.cpp`, `include/Encoders.h`, `include/Pins.h` | O shield REV 4.0 não tem conexão para encoders; nenhuma interrupção é registrada. | **Não integrado** |
| R11 | Calcular RPM das rodas | `src/Encoders.cpp`, `include/Encoders.h`, `src/main.cpp` | `calcularRPMRodas()` é chamado pelo `loop()` a cada 100 ms, mas o resultado é sempre 0, porque não há encoders no shield REV 4.0. | **Não integrado** |
| R12 | Medir distância pelo ultrassônico | `src/Ultrasonic.cpp`, `include/Ultrasonic.h`, `src/main.cpp` | TRIG 13 / ECHO 14 (ECHO via divisor 1k/2k). `readUltrasonicDistance()` é chamada pelo `loop()`, usa `pulseIn()` com timeout de 25 ms e zera valores fora de faixa. | **Implementado** |
| R13 | Ler dois MPU6050 | `src/MPU6050Handler.cpp`, `src/MPUProcessing.cpp`, `src/main_globals.cpp` | MPU1 usa endereço padrão e MPU2 recebe `105` (`0x69`); ambos são processados a cada intervalo de 10 ms. | **Implementado** |
| R14 | Calcular pitch e roll dos MPU6050 | `src/MPUProcessing.cpp`, `src/main_globals.cpp` | Calculados a partir da aceleração e ajustados por multiplicadores/offsets específicos por sensor. | **Implementado** |
| R15 | Calcular yaw do MPU1 com bússola | `src/MPUProcessing.cpp` | Inicializa QMC5883L, aplica calibração fixa, integra giroscópio Z e combina a leitura magnética. A qualidade depende do hardware e calibração. | **Parcial** |
| R16 | Disponibilizar orientação/yaw do MPU2 | `src/MPUProcessing.cpp`, `src/WebHandlers.cpp` | O campo `orientationZ_2` é velocidade angular Z convertida para graus/s, não yaw angular acumulado, embora a API o exponha como `yaw`. | **Parcial** |
| R17 | Analisar vibração por FFT | `src/MPUProcessing.cpp`, `src/FFTHandler.cpp`, `src/main_globals.cpp` | Janela de 64 amostras a 100 Hz, remoção da média, janela Hamming, FFT e busca do pico dominante. Atualizado ao completar cada bloco. | **Implementado** |
| R18 | Consultar leituras dos sensores via HTTP | `src/WebHandlers.cpp` | Endpoints de aceleração, rotação, orientação, vibração, distância e RPM. RPM é sempre 0 (sem encoders no shield); dados JSON dos eixos são strings. | **Parcial** |
| R19 | Conectar e reconectar ao broker MQTT | `src/MQTTHandler.cpp`, `src/main_globals.cpp`, `src/main.cpp` | PubSubClient configura broker e callback; `handleMQTT()` tenta reconectar a cada 5 s sem travar o loop e publica status após conectar. | **Implementado** |
| R20 | Publicar telemetria MQTT | `src/MQTTHandler.cpp`, `src/main_globals.cpp`, `src/main.cpp` | Publica somente giroscópio do MPU1 no tópico `R/IOT/CTRL`; tentativa de publicação é checada por `millis() % 10 == 0`, não por temporizador preciso. | **Parcial** |
| R21 | Publicar status MQTT | `src/MQTTHandler.cpp` | Publica `Online` em `home/status`; não usa a variável `topic_status` definida no estado global. | **Implementado** |
| R22 | Receber mensagens MQTT | `src/MQTTHandler.cpp` | Assina `home/commands`; o callback só registra a mensagem na Serial. A movimentação é exclusiva do WebSocket (R07). | **Implementado** |
| R23 | Disponibilizar FTP | `src/main.cpp` | `ftpSrv.begin(ssid, password)` é chamado depois do servidor web e `handleFTP()` roda no loop. A configuração efetiva depende da biblioteca SimpleFTPServer e das credenciais passadas. | **Implementado** |
| R24 | Imprimir telemetria de diagnóstico na Serial | `src/Debug.cpp`, `src/main.cpp` | Imprime RPM, distância, orientação e vibração aproximadamente a cada 500 ms. O campo RPM é sempre 0 no shield REV 4.0. | **Parcial** |
| R25 | Acessar o controle pela mesma rede Wi-Fi | `src/main.cpp`, `src/Network.cpp`, `src/WebSocketControl.cpp` | O celular acessa `http://<ip>/` ou `http://twinesp32.local/` na mesma rede do ESP32; não há acesso remoto pela internet. O IP é impresso na Serial. | **Implementado** |
| R26 | Manter estado compartilhado entre módulos | `include/main_globals.h`, `src/main_globals.cpp` | Declara objetos dos sensores, arrays FFT, resultados, contadores, credenciais e tópicos como variáveis globais. | **Implementado** |
| R27 | Executar testes automatizados | `test/README` | Há apenas instruções genéricas do diretório de testes; nenhum teste específico do firmware está presente. | **Não integrado** |

## Interfaces registradas

### HTTP

| Rota | Método | Função |
|---|---|---|
| `/`, `/style.css`, `/script.js`, `/favicon.png`, `/logo.svg`, `/line.svg`, `/upbutton.svg`, `/downbutton.svg`, `/leftbutton.svg`, `/rightbutton.svg`, `/stopbutton.svg` | GET | Arquivos estáticos servidos do LittleFS. |
| `/mpu1/acceleration`, `/mpu1/rotation`, `/mpu1/orientation` | GET | Valores do MPU1. |
| `/mpu2/acceleration`, `/mpu2/rotation`, `/mpu2/orientation` | GET | Valores do MPU2. |
| `/mpu1/vibration_magnitude`, `/mpu1/vibration_frequency`, `/mpu1/vibration_amplitude` | GET | Resultados do MPU1. |
| `/mpu2/vibration_magnitude`, `/mpu2/vibration_frequency`, `/mpu2/vibration_amplitude` | GET | Resultados do MPU2. |
| `/distance`, `/rpm_dir`, `/rpm_esq` | GET | Distância do ultrassônico; RPM sempre 0 (sem encoders no shield REV 4.0). |

### WebSocket

| Endpoint | Mensagens (texto) | Função |
|---|---|---|
| `/ws` | `frt`, `trs`, `esq`, `drt`, `stop`, `quadrado` | Movimentação do carrinho. Direções expiram em 500 ms sem reenvio; desconexão para os motores. |

### MQTT

| Direção | Tópico | Conteúdo |
|---|---|---|
| Publicação | `R/IOT/CTRL` | Objeto JSON com giroscópio do MPU1. |
| Publicação | `home/status` | Status textual `Online` na conexão. |
| Assinatura | `home/commands` | Tópico de comando atualmente assinado. |
| Callback | qualquer tópico assinado | Apenas registra a mensagem na Serial; não move o carrinho. |

## Observações de manutenção

- Credenciais de rede e MQTT existem como valores compilados no firmware; não copiá-las para documentação nem para logs públicos. Revisar também histórico Git e configurações locais antes de distribuir.
- `data/config.json` é lido, mas seus campos não são usados para conexão Wi-Fi.
- O ponto de acesso está comentado; é necessária uma rede Wi-Fi existente.
- O WebSocket de controle não implementa autenticação: qualquer aparelho na mesma rede Wi-Fi pode mover o carrinho.
- A pinagem segue o esquemático do shield REV 4.0 e está centralizada em `include/Pins.h`.
- A documentação de operação e compilação está em [README.md](README.md).
