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
| R01 | Compilar para ESP32 com PlatformIO/Arduino | `platformio.ini` | Ambiente `esp32dev`, framework Arduino e dependências de web, MQTT, sensores, FFT e FTP. | **Implementado** |
| R02 | Inicializar sensores e periféricos | `src/main.cpp`, `src/MPU6050Handler.cpp`, `src/Encoders.cpp`, `src/Ultrasonic.cpp` | `setup()` inicializa MPU6050, encoders e pinos ultrassônicos antes de montar SPIFFS. Falha de MPU entra em espera infinita. | **Parcial** |
| R03 | Montar SPIFFS e ler configuração JSON | `src/main.cpp`, `src/Config.cpp`, `data/config.json` | SPIFFS é montado com formatação em caso de falha; `loadConfig()` abre `/config.json` e desserializa para documento estático de 200 bytes. Os valores do documento não são usados para configurar a rede. | **Parcial** |
| R04 | Conectar ao Wi-Fi | `src/main.cpp`, `src/Network.cpp`, `src/main_globals.cpp` | `Network::connectWiFi()` tem timeout de 10 s, mas seu retorno é ignorado e o `setup()` continua esperando conexão indefinidamente. Credenciais vêm de constantes compiladas. | **Parcial** |
| R05 | Disponibilizar hostname e serviço mDNS | `src/main.cpp`, `src/Network.cpp`, `src/main_globals.cpp` | Inicia hostname `twinesp32`; registra `_http`/`tcp` na porta 1080. O servidor web efetivo é iniciado na porta 80. | **Parcial** |
| R06 | Servir a interface web estática | `src/WebHandlers.cpp`, `data/index.html`, `data/style.css`, `data/script.js`, `data/*.svg` | Arquivos são servidos do SPIFFS nas rotas raiz e assets. É necessário gravar o filesystem com os dados do diretório `data/`. | **Implementado** |
| R07 | Controlar avanço, ré, esquerda, direita e parada via HTTP | `src/WebHandlers.cpp`, `src/L9110H.cpp`, `data/index.html`, `data/script.js` | Botões enviam GET para `/frt`, `/trs`, `/esq`, `/drt` e `/stop`; cada movimento direcional dura 150 ms e para. O funcionamento físico depende da compatibilidade do hardware/pinos. | **Implementado** |
| R08 | Controlar motores pelo driver L9110H | `src/L9110H.cpp`, `include/L9110H.h` | Métodos usam `analogWrite()` e mapeiam os dois motores para pinos no método `motor()`. Construtor configura os membros antes do mapeamento; GPIO 6/7 do motor 1 exigem validação em placa ESP32 comum. | **Parcial** |
| R09 | Executar ação predefinida `quadrado` pela web | `src/WebHandlers.cpp`, `data/index.html`, `data/script.js` | Servidor espera `GET /action?action=quadrado`; o formulário envia POST com `input`, procura campo inexistente e usa jQuery que não está carregado. A sequência não é alcançável pelo formulário atual. | **Divergente** |
| R10 | Contar pulsos dos encoders | `src/Encoders.cpp`, `include/Encoders.h`, `src/main_globals.cpp` | ISRs nas GPIOs 26 e 34 incrementam contadores em borda de subida; `setupEncoders()` é chamado em `setup()`. | **Implementado** |
| R11 | Calcular RPM das rodas | `src/Encoders.cpp`, `include/Encoders.h`, `src/main.cpp` | `calcularRPMRodas()` calcula a cada 100 ms usando 20 pulsos por volta, mas não é chamado pelo `loop()`. | **Não integrado** |
| R12 | Medir distância pelo ultrassônico | `src/Ultrasonic.cpp`, `include/Ultrasonic.h`, `src/main.cpp` | A rotina usa `pulseIn()` com timeout de 25 ms e limita valores fora de faixa, mas `readUltrasonicDistance()` não é chamada pelo `loop()`. | **Não integrado** |
| R13 | Ler dois MPU6050 | `src/MPU6050Handler.cpp`, `src/MPUProcessing.cpp`, `src/main_globals.cpp` | MPU1 usa endereço padrão e MPU2 recebe `105` (`0x69`); ambos são processados a cada intervalo de 10 ms. | **Implementado** |
| R14 | Calcular pitch e roll dos MPU6050 | `src/MPUProcessing.cpp`, `src/main_globals.cpp` | Calculados a partir da aceleração e ajustados por multiplicadores/offsets específicos por sensor. | **Implementado** |
| R15 | Calcular yaw do MPU1 com bússola | `src/MPUProcessing.cpp` | Inicializa QMC5883L, aplica calibração fixa, integra giroscópio Z e combina a leitura magnética. A qualidade depende do hardware e calibração. | **Parcial** |
| R16 | Disponibilizar orientação/yaw do MPU2 | `src/MPUProcessing.cpp`, `src/WebHandlers.cpp` | O campo `orientationZ_2` é velocidade angular Z convertida para graus/s, não yaw angular acumulado, embora a API o exponha como `yaw`. | **Parcial** |
| R17 | Analisar vibração por FFT | `src/MPUProcessing.cpp`, `src/FFTHandler.cpp`, `src/main_globals.cpp` | Janela de 64 amostras a 100 Hz, remoção da média, janela Hamming, FFT e busca do pico dominante. Atualizado ao completar cada bloco. | **Implementado** |
| R18 | Consultar leituras dos sensores via HTTP | `src/WebHandlers.cpp` | Endpoints de aceleração, rotação, orientação, vibração, distância e RPM. Distância/RPM permanecem desatualizados enquanto as rotinas de aquisição não forem integradas; dados JSON dos eixos são strings. | **Parcial** |
| R19 | Conectar e reconectar ao broker MQTT | `src/MQTTHandler.cpp`, `src/main_globals.cpp`, `src/main.cpp` | PubSubClient configura broker e callback; `handleMQTT()` tenta reconectar em loop bloqueante e publica status após conectar. | **Parcial** |
| R20 | Publicar telemetria MQTT | `src/MQTTHandler.cpp`, `src/main_globals.cpp`, `src/main.cpp` | Publica somente giroscópio do MPU1 no tópico `R/IOT/CTRL`; tentativa de publicação é checada por `millis() % 10 == 0`, não por temporizador preciso. | **Parcial** |
| R21 | Publicar status MQTT | `src/MQTTHandler.cpp` | Publica `Online` em `home/status`; não usa a variável `topic_status` definida no estado global. | **Implementado** |
| R22 | Receber comandos MQTT | `src/MQTTHandler.cpp` | Assina `home/commands`, mas callback só trata mensagens se tópico recebido for `R/IOT/CTRL`. | **Divergente** |
| R23 | Disponibilizar FTP | `src/main.cpp` | `ftpSrv.begin(ssid, password)` é chamado depois do servidor web e `handleFTP()` roda no loop. A configuração efetiva depende da biblioteca SimpleFTPServer e das credenciais passadas. | **Implementado** |
| R24 | Imprimir telemetria de diagnóstico na Serial | `src/Debug.cpp`, `src/main.cpp` | Imprime RPM, distância, orientação e vibração aproximadamente a cada 500 ms. Campos RPM/distância dependem de rotinas que atualmente não são chamadas. | **Parcial** |
| R25 | Acessar o dispositivo remotamente via zrok | `zrok-digitaltwin.sh`, `ZROK_README.md` | Script tenta resolver `twinesp32.local` e executa `zrok reserve public` e `zrok share reserved`; depende de zrok, mDNS e compatibilidade da CLI/porta. | **Parcial** |
| R26 | Manter estado compartilhado entre módulos | `include/main_globals.h`, `src/main_globals.cpp` | Declara objetos dos sensores, arrays FFT, resultados, contadores, credenciais e tópicos como variáveis globais. | **Implementado** |
| R27 | Executar testes automatizados | `test/README` | Há apenas instruções genéricas do diretório de testes; nenhum teste específico do firmware está presente. | **Não integrado** |

## Interfaces registradas

### HTTP

| Rota | Método | Função |
|---|---|---|
| `/`, `/style.css`, `/script.js`, `/favicon.png`, `/logo.svg`, `/line.svg`, `/upbutton.svg`, `/downbutton.svg`, `/leftbutton.svg`, `/rightbutton.svg`, `/stopbutton.svg` | GET | Arquivos estáticos servidos do SPIFFS. |
| `/frt`, `/trs`, `/esq`, `/drt`, `/stop` | GET | Comandos de movimento e parada. |
| `/action?action=quadrado` | GET | Sequência de movimento predefinida; formulário atual não corresponde a esta interface. |
| `/mpu1/acceleration`, `/mpu1/rotation`, `/mpu1/orientation` | GET | Valores do MPU1. |
| `/mpu2/acceleration`, `/mpu2/rotation`, `/mpu2/orientation` | GET | Valores do MPU2. |
| `/mpu1/vibration_magnitude`, `/mpu1/vibration_frequency`, `/mpu1/vibration_amplitude` | GET | Resultados do MPU1. |
| `/mpu2/vibration_magnitude`, `/mpu2/vibration_frequency`, `/mpu2/vibration_amplitude` | GET | Resultados do MPU2. |
| `/distance`, `/rpm_dir`, `/rpm_esq` | GET | Valores do ultrassônico e encoders, atualmente sem atualização pelo loop principal. |

### MQTT

| Direção | Tópico | Conteúdo |
|---|---|---|
| Publicação | `R/IOT/CTRL` | Objeto JSON com giroscópio do MPU1. |
| Publicação | `home/status` | Status textual `Online` na conexão. |
| Assinatura | `home/commands` | Tópico de comando atualmente assinado. |
| Callback | `R/IOT/CTRL` | Tópico que o callback exige antes de aceitar comandos; divergente da assinatura. |

## Observações de manutenção

- Credenciais de rede e MQTT existem como valores compilados no firmware; não copiá-las para documentação nem para logs públicos. Revisar também histórico Git e configurações locais antes de distribuir.
- `data/config.json` é lido, mas seus campos não são usados para conexão Wi-Fi.
- O ponto de acesso está comentado; é necessária uma rede Wi-Fi existente.
- O servidor HTTP de controle não implementa autenticação. O acesso remoto pode expor comandos de movimento.
- Confirme o mapeamento elétrico das GPIOs, sobretudo GPIO 6 e 7 em placas ESP32 com flash nessas linhas.
- A documentação de operação e compilação está em [README.md](README.md); o guia zrok complementar é [ZROK_README.md](ZROK_README.md).
