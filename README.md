# Carrinho com ESP32 — URA / Gêmeo Digital

Firmware PlatformIO/Arduino para um carrinho robótico baseado em ESP32. O firmware controla dois motores, disponibiliza uma interface web na rede local, coleta dados de sensores e inclui integrações com MQTT, FTP e mDNS.

> **Estado da implementação:** este guia descreve o comportamento encontrado no código, não apenas as funções previstas. Algumas partes, como o RPM das rodas (sem encoders no shield REV 4.0), não estão disponíveis no hardware atual. Consulte [Limitações conhecidas](#limitações-conhecidas) e a [matriz de rastreabilidade](TRACEABILITY_MATRIX.md).

## Sumário

- [Arquitetura](#arquitetura)
- [Estrutura do repositório](#estrutura-do-repositório)
- [Hardware e conexões](#hardware-e-conexões)
- [Dependências](#dependências)
- [Compilação e gravação](#compilação-e-gravação)
- [Inicialização e operação](#inicialização-e-operação)
- [Controle por WebSocket](#controle-por-websocket)
- [Interface HTTP](#interface-http)
- [MQTT](#mqtt)
- [Configuração](#configuração)
- [Diagnóstico](#diagnóstico)
- [Limitações conhecidas](#limitações-conhecidas)
- [Documentação complementar](#documentação-complementar)

## Arquitetura

O ESP32 é o controlador central. Os módulos de firmware compartilham leituras e configurações declaradas em `main_globals`, enquanto `main.cpp` inicializa os componentes e coordena as tarefas periódicas.

```text
Navegador ── HTTP :80 + WebSocket /ws ┐
Broker MQTT ── Wi-Fi ────────────────┤
FTP ── Wi-Fi ────────────────────────┤
                                     v
                                  ESP32
             ┌───────────────────────┼──────────────────────┐
             v                       v                      v
       Driver L9110H        MPU6050 #1/#2 + QMC5883L   Encoders/ultrassônico
             │                       │                      │
        Dois motores           Orientação e FFT       Contagem/medição
```

### Fluxo de inicialização

1. Inicia Serial a 115200 baud.
2. Inicializa o I²C (SDA 8 / SCL 9), os dois MPU6050 e os pinos do sensor ultrassônico.
3. Monta LittleFS e tenta ler `/config.json`.
4. Conecta ao Wi-Fi usando valores compilados no firmware.
5. Inicia mDNS com hostname `twinesp32` e registra o serviço `_http`/`tcp` na porta 80.
6. Aguarda 10 segundos; em seguida, registra o WebSocket `/ws` (e inicia a tarefa dos motores) e as rotas HTTP, inicia o servidor na porta 80, o FTP e a configuração MQTT. O endereço da página de controle é impresso na Serial.

No `loop()`, chama o processamento do FTP, do WebSocket e do MQTT, processa os dois MPU6050 e lê o ultrassônico. Também verifica a condição de publicação MQTT e imprime um resumo serial aproximadamente a cada 500 ms.

### Fluxo dos comandos de movimento

- O celular (ou PC) e o ESP32 precisam estar na **mesma rede Wi-Fi**. A página é aberta em `http://<IP-do-ESP32>/` ou `http://twinesp32.local/`.
- A página abre um WebSocket em `ws://<IP-do-ESP32>/ws` e envia comandos de texto (ver [Controle por WebSocket](#controle-por-websocket)).
- O carrinho anda **enquanto o botão estiver pressionado** e para ao soltar. A página repete o comando a cada 150 ms; se nenhum comando chegar em 500 ms, o firmware para os motores sozinho.
- Os comandos são aplicados por uma tarefa FreeRTOS dedicada (`motorTask`, em `src/WebSocketControl.cpp`), independente do `loop()`. Assim, a parada por timeout funciona mesmo que o loop esteja ocupado.
- Não há mais rotas HTTP de movimento, e o MQTT não aciona os motores.

### Fluxo dos dados inerciais

Cada MPU é consultado com intervalo mínimo de 10 ms (100 Hz). O firmware atualiza aceleração e velocidade angular, calcula pitch e roll e coleta a magnitude da aceleração em blocos de 64 amostras para a FFT. Cada bloco cobre aproximadamente 0,64 s, com resolução de frequência próxima de 1,56 Hz.

- No MPU1, o yaw combina integração da velocidade angular Z com a leitura da bússola QMC5883L e uma calibração fixa.
- No MPU2, o valor chamado yaw é a velocidade angular Z convertida para graus por segundo; não é um ângulo de orientação acumulado.

## Estrutura do repositório

| Caminho | Responsabilidade |
|---|---|
| `platformio.ini` | Ambiente PlatformIO, alvo ESP32, framework e bibliotecas externas. |
| `src/main.cpp` | Inicialização do firmware e loop principal. |
| `src/main_globals.cpp`, `include/main_globals.h` | Estado global compartilhado: sensores, motores, configuração de rede e MQTT. |
| `src/L9110H.cpp`, `include/L9110H.h` | Comandos de direção e PWM dos dois motores. |
| `src/MPU6050Handler.cpp`, `src/MPUProcessing.cpp` | Inicialização e processamento dos sensores inerciais e da bússola. |
| `src/FFTHandler.cpp` | Processamento FFT da magnitude de aceleração. |
| `include/Pins.h` | Pinagem do shield REV 4.0 para o ESP32-S3-DevKitC-1 (única definição de GPIOs). |
| `src/Encoders.cpp` | Cálculo de RPM; desativado porque o shield REV 4.0 não tem encoders. |
| `src/Ultrasonic.cpp` | Inicialização e rotina de leitura do sensor ultrassônico. |
| `src/WebHandlers.cpp` | Arquivos estáticos e endpoints HTTP de telemetria. |
| `src/WebSocketControl.cpp`, `include/WebSocketControl.h` | WebSocket `/ws`, protocolo de comandos e tarefa que aciona os motores (com parada por timeout). |
| `src/MQTTHandler.cpp` | Conexão, publicação e callback MQTT. |
| `src/Network.cpp`, `src/Config.cpp` | Helpers de rede e leitura do JSON no LittleFS. |
| `src/Debug.cpp` | Resumo periódico enviado à Serial. |
| `data/` | Arquivos da interface web e `config.json`, destinados ao filesystem do dispositivo. |
| `include/HCSR04/` | Biblioteca local HCSR04 e exemplos; não é usada pela rotina ativa do ultrassônico, que usa GPIO diretamente. |
| `test/` | Diretório reservado para testes PlatformIO; não há testes automatizados do projeto atualmente. |

## Hardware e conexões

Placa: **ESP32-S3-DevKitC-1** montada no shield **"Projeto Gêmeo Digital" REV 4.0** (EasyEDA, URA). A pinagem segue o esquemático do shield e está centralizada em [`include/Pins.h`](include/Pins.h), que é o único lugar onde os GPIOs são definidos.

| Componente | Sinal no esquemático | GPIO | Observações |
|---|---|---|---|
| L9110S (U3), motor 1 | A1_A / A1_B | 7 / 6 | Canal A da ponte H. |
| L9110S (U3), motor 2 | B1_A / B1_B | 4 / 5 | Canal B da ponte H. |
| Barramento I²C | SDA / SCL | 8 / 9 | Compartilhado por MPU-1, MPU-2 e bússola. |
| MPU6050 #1 (J2) | I²C | — | Endereço `0x68` (AD0 sem ligação). |
| MPU6050 #2 (J3) | I²C | — | Endereço `0x69` (pino 5 do J3, AD0, ligado a 3V3). |
| Bússola (U5) | I²C | — | Footprint HMC5883L; o módulo usado é QMC5883L (`0x0D`). DRDY não conectado. |
| HC-SR04 (U2) | TRIG / ECHO | 13 / 14 | ECHO passa pelo divisor R3 1k / R4 2k (5 V → 3,3 V). Alimentado por Vin. |
| Encoders das rodas | — | — | **Não existem no shield REV 4.0.** O firmware não registra interrupções, e o RPM fica em 0. |

Alimentação: o MT3608 (U4) gera Vin, que alimenta o pino 5V da DevKitC-1, a ponte H e o HC-SR04. MPUs e bússola usam 3V3. Os pinos do driver são configurados pelo `analogWrite()` no primeiro comando de cada motor. Se uma roda girar no sentido errado, inverta os fios do motor no borne da ponte H.

## Dependências

O projeto usa o ambiente `[env:esp32dev]` do PlatformIO (o nome do ambiente é histórico), com framework Arduino e placa `esp32-s3-devkitc-1`. As bibliotecas declaradas em `platformio.ini` incluem:

- ESPAsyncWebServer e AsyncTCP;
- ArduinoJson;
- SimpleFTPServer;
- Adafruit MPU6050 e Adafruit Unified Sensor;
- QMC5883LCompass;
- arduinoFFT;
- PubSubClient;
- NewPing e HMC5883L (declaradas como dependências, embora não façam parte do caminho ativo principal descrito acima).

O gerenciador PlatformIO resolve as dependências declaradas durante a compilação. O diretório `.pio/` contém arquivos locais de build/dependências e não deve ser editado manualmente.

## Compilação e gravação

Instale o PlatformIO (por exemplo, pela extensão PlatformIO IDE do VS Code) e conecte a placa ESP32 por USB. Na raiz do projeto:

```sh
# Compilar
pio run -e esp32dev

# Gravar o firmware (ajuste a porta se necessário)
pio run -e esp32dev -t upload

# Gravar os arquivos da pasta data/ no filesystem LittleFS
pio run -e esp32dev -t uploadfs

# Abrir o monitor serial a 115200 baud
pio device monitor -b 115200
```

O firmware serve arquivos a partir do LittleFS (`board_build.filesystem = littlefs` no `platformio.ini`), portanto a interface web precisa estar gravada no filesystem. **Sempre que alterar algo em `data/`, rode `uploadfs` de novo.** A opção `LittleFS.begin(true)` formata a partição se a montagem falhar, o que apaga os arquivos existentes.

## Inicialização e operação

1. Verifique a fiação, em particular os pinos do driver, os níveis do sensor ECHO e os endereços I²C.
2. Configure as credenciais de rede e do broker no firmware **sem publicá-las em repositórios ou documentação**.
3. Compile e grave o firmware; grave também o filesystem com `uploadfs`.
4. Abra o monitor serial, reinicie a placa e acompanhe inicialização dos sensores, conexão Wi-Fi e endereço IP.
5. Conecte o celular na **mesma rede Wi-Fi** do ESP32 e acesse `http://<IP-do-ESP32>/` (ou `http://twinesp32.local/`, se o celular resolver mDNS; no Android isso nem sempre funciona, então prefira o IP).
6. Confira se o indicador abaixo de "CONTROLE" mostra **Conectado** (verde).
7. Segure um botão de direção para andar. Soltar, tocar em parar, trocar de aba ou bloquear a tela para o carrinho. No PC, as setas do teclado também funcionam (espaço ou Esc param).

O firmware não inicia seu ponto de acesso Wi-Fi no estado atual: o trecho de `WiFi.softAP()` está comentado. O dispositivo depende de uma rede Wi-Fi existente. Se um MPU6050 não for encontrado, a inicialização entra em espera infinita; se o Wi-Fi não conectar, o `setup()` também permanece esperando.

## Controle por WebSocket

Endpoint: `ws://<IP-do-ESP32>/ws` (mesma porta 80 do servidor web). As mensagens são de texto simples:

| Mensagem | Ação |
|---|---|
| `frt` | Frente. |
| `trs` | Ré. |
| `esq` | Gira à esquerda. |
| `drt` | Gira à direita. |
| `stop` | Para os motores. |
| `quadrado` | Executa 4× (anda 2 s, pausa 2 s, gira 300 ms, pausa 2 s), sem bloquear o firmware. Qualquer outro comando interrompe a sequência. |

Regras:

- Os comandos de direção valem por **500 ms**. Para manter o movimento, o cliente precisa reenviar o comando antes disso (a página reenvia a cada 150 ms).
- Quando um cliente WebSocket desconecta, o carrinho para.
- Comandos desconhecidos recebem como resposta `erro:comando desconhecido:<texto>`.
- A velocidade é fixa em PWM 255 (`MOTOR_SPEED` em `src/WebSocketControl.cpp`).

O campo "Digite um comando" da página envia o texto digitado pelo WebSocket (por exemplo, `quadrado`).

Teste rápido sem a página, pelo console do navegador aberto em `http://<IP>/`:

```js
const s = new WebSocket("ws://" + location.host + "/ws");
s.onopen = () => s.send("quadrado");
```

## Interface HTTP

O servidor assíncrono escuta na porta **80** e serve a página e as leituras dos sensores (GET). As rotas estão registradas em `src/WebHandlers.cpp`. Não há rotas HTTP de movimento: o controle é feito apenas pelo WebSocket.

### Telemetria HTTP

| GET | Resposta |
|---|---|
| `/mpu1/acceleration`, `/mpu2/acceleration` | JSON com `x`, `y` e `z`. Os valores são serializados como strings. |
| `/mpu1/rotation`, `/mpu2/rotation` | JSON com `x`, `y` e `z`, serializados como strings. |
| `/mpu1/orientation`, `/mpu2/orientation` | JSON com `pitch`, `roll` e `yaw`, serializados como strings. No MPU2, `yaw` representa taxa angular. |
| `/mpu1/vibration_magnitude`, `/mpu2/vibration_magnitude` | Texto simples com magnitude calculada. |
| `/mpu1/vibration_frequency`, `/mpu2/vibration_frequency` | Texto simples com frequência dominante da última janela FFT. |
| `/mpu1/vibration_amplitude`, `/mpu2/vibration_amplitude` | Texto simples com amplitude dominante da última janela FFT. |
| `/distance` | Texto simples com a distância do ultrassônico em cm (0 sem eco). |
| `/rpm_dir`, `/rpm_esq` | Texto simples com RPM; sempre 0 no shield REV 4.0, que não tem encoders. |

O HTML, o CSS e o JavaScript ficam em `data/index.html`, `data/style.css` e `data/script.js`. O script não depende de bibliotecas externas, então a página funciona numa rede local sem internet.

## MQTT

O cliente usa PubSubClient sobre Wi-Fi e tenta se conectar ao broker configurado em `src/main_globals.cpp`. Os valores de autenticação são definidos no firmware e não são repetidos aqui.

| Operação | Tópico atual | Conteúdo/estado |
|---|---|---|
| Publicar telemetria | `R/IOT/CTRL` (valor de `topic_sensors`) | JSON único com `accelerometer`, `gyroscope` e `temperature` (MPU1) e `Ultrassonic.distance`, a cada `MQTT_PUBLISH_INTERVAL_MS` (1 s). |
| Publicar status | `home/status` | Texto `Online` após conectar. |
| Assinar comandos | `home/commands` | Assinatura efetuada após conexão MQTT. |
| Callback | qualquer tópico assinado | Apenas registra a mensagem na Serial; **não move o carrinho**. |

O MQTT é usado só para telemetria e status; a movimentação é exclusiva do WebSocket. Se o broker não estiver acessível, o firmware tenta reconectar a cada 5 s sem travar o `loop()`. Cada tentativa ainda pode segurar o loop por alguns segundos (timeout de conexão TCP), mas isso não afeta os motores, que são controlados por uma tarefa separada.

Formato da telemetria:

```json
{
  "accelerometer": { "x": 0.00, "y": 0.00, "z": 0.00 },
  "gyroscope":     { "x": 0.00, "y": 0.00, "z": 0.00 },
  "temperature": 25.5,
  "Ultrassonic": { "distance": 100.23 }
}
```

- Aceleração em m/s², giroscópio em rad/s e temperatura em °C (sensor interno do MPU1).
- `distance` está em cm; vale 0 quando o sensor não recebe eco (fora de 2–400 cm).
- O intervalo é fixo (`MQTT_PUBLISH_INTERVAL_MS`, 1 s) porque o backend grava um arquivo por mensagem; sem broker conectado nada é publicado.

## Configuração

O arquivo `data/config.json` é:

```json
{
  "ssid": "<SSID_DA_REDE>",
  "password": "<SENHA_DA_REDE>"
}
```

O arquivo existente em `data/` contém valores que devem ser tratados como credenciais locais e substituídos antes de distribuir ou gravar o filesystem. O firmware atualmente apenas tenta carregar `/config.json` em um `StaticJsonDocument<200>` e não usa seus campos para conectar à rede. As credenciais efetivamente usadas são constantes em `src/main_globals.cpp`. O modo SoftAP correspondente também está comentado em `src/main.cpp`.

Antes de compartilhar o projeto, remova ou substitua credenciais reais já existentes no código e revise o histórico do repositório. Esta documentação intencionalmente não reproduz esses valores.

## Diagnóstico

- **Inicialização travada antes do Wi-Fi:** verifique os dois MPU6050 no I²C e o endereço do segundo dispositivo.
- **Não aparece endereço IP:** confirme rede, disponibilidade do ponto de acesso e credenciais configuradas no firmware. O código espera conexão sem limite de tempo.
- **Página não abre:** confirme que o celular está na mesma rede Wi-Fi, use o IP impresso na Serial (em vez de `twinesp32.local`) e verifique se os arquivos foram gravados com `uploadfs`. Redes com isolamento de clientes (comum em Wi-Fi de visitantes) bloqueiam o acesso entre aparelhos.
- **Página abre sem assets:** regrave o filesystem e verifique a Serial/LittleFS.
- **Indicador fica "Desconectado":** o WebSocket `/ws` não conectou. Confira a Serial (`WebSocket: cliente #N conectado`) e recarregue a página.
- **Carrinho para sozinho enquanto o botão está pressionado:** os comandos estão chegando com mais de 500 ms de intervalo (Wi-Fi fraco). Aproxime-se do roteador ou aumente `COMMAND_TIMEOUT_MS` em `src/WebSocketControl.cpp`.
- **Comandos dos motores não funcionam:** confira alimentação do driver, sentido dos motores, PWM e compatibilidade física das GPIOs selecionadas.
- **RPM permanece em zero:** esperado, porque o shield REV 4.0 não tem encoders.
- **Distância permanece em zero:** confira o HC-SR04 em TRIG 13 / ECHO 14, a alimentação Vin e o divisor R3/R4. Leituras fora de 0–400 cm são zeradas.
- **MQTT sem conexão:** confirme Wi-Fi, host/porta e configuração do broker. O controle por WebSocket funciona mesmo sem broker.
- **Leituras da bússola/yaw inconsistentes:** confira o módulo QMC5883L, a calibração fixa e a orientação física do sensor.

O resumo serial periódico é produzido por `src/Debug.cpp`.

## Limitações conhecidas

1. O shield REV 4.0 não tem encoders, então o RPM das rodas não é medido.
2. A movimentação só é aceita pelo WebSocket; MQTT e HTTP não movem o carrinho.
3. Com vários celulares conectados ao mesmo tempo, todos controlam o carrinho, e a desconexão de qualquer um deles para os motores.
4. O MPU2 informa taxa angular como yaw, não orientação acumulada.
5. Credenciais de Wi-Fi e MQTT estão definidas diretamente em arquivos de código; o JSON não as substitui no fluxo atual.
6. O WebSocket de controle não tem autenticação: qualquer aparelho na mesma rede Wi-Fi pode mover o carrinho.
7. O sentido de rotação de cada motor depende de como os fios estão no borne da ponte H; o firmware assume o motor 1 no canal A e o motor 2 no canal B.
8. Falhas ao encontrar os MPU6050 e ao conectar ao Wi-Fi bloqueiam a inicialização sem uma estratégia de recuperação.
9. Não há suíte de testes automatizados do firmware no diretório `test/`.

Esses pontos são descrições do estado atual para orientar manutenção; este documento não altera nem corrige o firmware.

## Documentação complementar

- [Matriz de rastreabilidade](TRACEABILITY_MATRIX.md): relação entre funcionalidades/requisitos, módulos e estado de implementação.
- [Configuração PlatformIO](platformio.ini): placa, framework e dependências.
