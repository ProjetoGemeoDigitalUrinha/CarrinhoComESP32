# Carrinho com ESP32 — URA / Gêmeo Digital

Firmware PlatformIO/Arduino para um carrinho robótico baseado em ESP32. O firmware controla dois motores, disponibiliza uma interface web na rede local, coleta dados de sensores e inclui integrações com MQTT, FTP e mDNS.

> **Estado da implementação:** este guia descreve o comportamento encontrado no código, não apenas as funções previstas. Algumas partes — como leitura do ultrassônico, cálculo de RPM e formulário de ações — estão parcialmente conectadas ou não são atualizadas pelo loop atual. Consulte [Limitações conhecidas](#limitações-conhecidas) e a [matriz de rastreabilidade](TRACEABILITY_MATRIX.md).

## Sumário

- [Arquitetura](#arquitetura)
- [Estrutura do repositório](#estrutura-do-repositório)
- [Hardware e conexões](#hardware-e-conexões)
- [Dependências](#dependências)
- [Compilação e gravação](#compilação-e-gravação)
- [Inicialização e operação](#inicialização-e-operação)
- [Interface HTTP](#interface-http)
- [MQTT](#mqtt)
- [Configuração](#configuração)
- [Acesso remoto com zrok](#acesso-remoto-com-zrok)
- [Diagnóstico](#diagnóstico)
- [Limitações conhecidas](#limitações-conhecidas)
- [Documentação complementar](#documentação-complementar)

## Arquitetura

O ESP32 é o controlador central. Os módulos de firmware compartilham leituras e configurações declaradas em `main_globals`, enquanto `main.cpp` inicializa os componentes e coordena as tarefas periódicas.

```text
Navegador ── HTTP :80 ───────────────┐
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
2. Inicializa os dois MPU6050, os encoders e os pinos do sensor ultrassônico.
3. Monta SPIFFS e tenta ler `/config.json`.
4. Conecta ao Wi-Fi usando valores compilados no firmware.
5. Inicia mDNS com hostname `twinesp32` e registra o serviço `_http`/`tcp` na porta configurada como 1080.
6. Aguarda 10 segundos; em seguida, registra as rotas HTTP e inicia o servidor na porta 80, o FTP e a configuração MQTT.

No `loop()`, chama o processamento do FTP e MQTT e processa os dois MPU6050. Também verifica a condição de publicação MQTT e imprime um resumo serial aproximadamente a cada 500 ms. As funções de leitura do ultrassônico e cálculo de RPM não são chamadas nesse loop.

### Fluxo dos comandos de movimento

- Os botões da página fazem requisições HTTP GET às rotas direcionais.
- Cada rota chama a classe `L9110H` com velocidade 255 e duração de 150 ms; depois, o método para os motores.
- O MQTT contém tratamento para comandos equivalentes, mas há divergência entre o tópico assinado e o tópico verificado no callback. Portanto, essa via não está funcional de ponta a ponta no estado atual.

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
| `src/Encoders.cpp` | Interrupções dos encoders e cálculo de RPM. |
| `src/Ultrasonic.cpp` | Inicialização e rotina de leitura do sensor ultrassônico. |
| `src/WebHandlers.cpp` | Arquivos estáticos e endpoints HTTP. |
| `src/MQTTHandler.cpp` | Conexão, publicação e callback MQTT. |
| `src/Network.cpp`, `src/Config.cpp` | Helpers de rede e leitura do JSON no SPIFFS. |
| `src/Debug.cpp` | Resumo periódico enviado à Serial. |
| `data/` | Arquivos da interface web e `config.json`, destinados ao filesystem do dispositivo. |
| `include/HCSR04/` | Biblioteca local HCSR04 e exemplos; não é usada pela rotina ativa do ultrassônico, que usa GPIO diretamente. |
| `test/` | Diretório reservado para testes PlatformIO; não há testes automatizados do projeto atualmente. |
| `zrok-digitaltwin.sh` | Script Linux para tentar criar/ativar um compartilhamento zrok para o dispositivo. |

## Hardware e conexões

As GPIOs abaixo são os valores encontrados no código e devem ser confirmadas no circuito real antes de energizar o robô.

| Componente | GPIO/configuração atual | Observações |
|---|---|---|
| Driver L9110H, motor 1 | 7 e 6 | O arquivo atual atribui esses pinos ao motor 1. Em placas ESP32 comuns, GPIO 6–11 são associadas à memória flash; valide a placa e a fiação, pois esses pinos podem não estar disponíveis para uso geral. |
| Driver L9110H, motor 2 | 14 e 23 | Atribuídos no método `motor()`. |
| Encoder direita | 26 | Contagem por interrupção na borda de subida. |
| Encoder esquerda | 34 | Contagem por interrupção na borda de subida; GPIO 34 é somente entrada no ESP32. |
| Ultrassônico TRIG | 5 | Configurado como saída. |
| Ultrassônico ECHO | 18 | Configurado como entrada. Verifique a compatibilidade dos níveis lógicos do ECHO com o ESP32. |
| MPU6050 #1 | I²C, endereço padrão da biblioteca | Inicializado por `mpu1.begin()`. |
| MPU6050 #2 | I²C, endereço `0x69` | Inicializado por `mpu2.begin(105)`; exige configuração de endereço correspondente no pino AD0. |
| Bússola QMC5883L | I²C | Inicializada durante o processamento do MPU1. |

Os dois MPU6050 e a bússola compartilham o barramento I²C. Confirme alimentação, terra comum, endereços e pull-ups segundo os módulos usados. O construtor de `L9110H` configura os pinos antes de `motor()` atribuir os números de GPIO; essa inicialização merece revisão junto com o mapeamento de pinos.

## Dependências

O projeto usa o ambiente `[env:esp32dev]` do PlatformIO, com framework Arduino e placa `esp32dev`. As bibliotecas declaradas em `platformio.ini` incluem:

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

# Gravar os arquivos da pasta data/ no filesystem SPIFFS
pio run -e esp32dev -t uploadfs

# Abrir o monitor serial a 115200 baud
pio device monitor -b 115200
```

O firmware serve arquivos a partir do SPIFFS, portanto a interface web precisa estar gravada no filesystem. A opção `SPIFFS.begin(true)` solicita formatação se a montagem falhar; uma falha de montagem pode apagar os arquivos existentes ao tentar recuperá-la.

## Inicialização e operação

1. Verifique a fiação, em particular os pinos do driver, os níveis do sensor ECHO e os endereços I²C.
2. Configure as credenciais de rede e do broker no firmware **sem publicá-las em repositórios ou documentação**.
3. Compile e grave o firmware; grave também o filesystem com `uploadfs`.
4. Abra o monitor serial, reinicie a placa e acompanhe inicialização dos sensores, conexão Wi-Fi e endereço IP.
5. Acesse `http://<IP-do-ESP32>/` na mesma rede.
6. Para parar o robô, use o botão de parada ou a rota `/stop`.

O firmware não inicia seu ponto de acesso Wi-Fi no estado atual: o trecho de `WiFi.softAP()` está comentado. O dispositivo depende de uma rede Wi-Fi existente. Se um MPU6050 não for encontrado, a inicialização entra em espera infinita; se o Wi-Fi não conectar, o `setup()` também permanece esperando.

## Interface HTTP

O servidor assíncrono escuta na porta **80**. As requisições de leitura são GET. A página web contém botões direcionais; as rotas estão registradas em `src/WebHandlers.cpp`.

### Controle

| Método e rota | Ação |
|---|---|
| `GET /frt` | Avança por 150 ms e para. |
| `GET /trs` | Recua por 150 ms e para. |
| `GET /esq` | Vira à esquerda por 150 ms e para. |
| `GET /drt` | Vira à direita por 150 ms e para. |
| `GET /stop` | Para os motores. |
| `GET /action?action=quadrado` | Executa uma sequência de quatro lados e giros; a rotina usa esperas bloqueantes. Não é acionada corretamente pelo formulário atual da página. |

Os parâmetros de duração passados aos métodos do motor são usados diretamente por `delay()`, que recebe milissegundos, apesar do nome `seconds` usado no código.

### Telemetria HTTP

| GET | Resposta |
|---|---|
| `/mpu1/acceleration`, `/mpu2/acceleration` | JSON com `x`, `y` e `z`. Os valores são serializados como strings. |
| `/mpu1/rotation`, `/mpu2/rotation` | JSON com `x`, `y` e `z`, serializados como strings. |
| `/mpu1/orientation`, `/mpu2/orientation` | JSON com `pitch`, `roll` e `yaw`, serializados como strings. No MPU2, `yaw` representa taxa angular. |
| `/mpu1/vibration_magnitude`, `/mpu2/vibration_magnitude` | Texto simples com magnitude calculada. |
| `/mpu1/vibration_frequency`, `/mpu2/vibration_frequency` | Texto simples com frequência dominante da última janela FFT. |
| `/mpu1/vibration_amplitude`, `/mpu2/vibration_amplitude` | Texto simples com amplitude dominante da última janela FFT. |
| `/distance` | Texto simples com a variável de distância; no loop atual ela não é atualizada pela rotina do sensor. |
| `/rpm_dir`, `/rpm_esq` | Texto simples com RPM; no loop atual o cálculo de RPM não é chamado. |

O HTML e CSS ficam em `data/index.html` e `data/style.css`. Os botões direcionais enviam GET usando `XMLHttpRequest`. Já o formulário usa jQuery sem carregar a biblioteca, procura um campo `name="action"` que não existe no HTML e envia POST com o campo `input`, enquanto o firmware espera GET com `action`. Consequentemente, o formulário não aciona a ação prevista.

## MQTT

O cliente usa PubSubClient sobre Wi-Fi e tenta se conectar ao broker configurado em `src/main_globals.cpp`. Os valores de autenticação são definidos no firmware e não são repetidos aqui.

| Operação | Tópico atual | Conteúdo/estado |
|---|---|---|
| Publicar telemetria | `R/IOT/CTRL` (valor de `topic_sensors`) | JSON com `gyroscope.x`, `.y`, `.z` do MPU1. |
| Publicar status | `home/status` | Texto `Online` após conectar. |
| Assinar comandos | `home/commands` | Assinatura efetuada após conexão MQTT. |
| Verificar comandos no callback | `R/IOT/CTRL` | O callback compara o tópico recebido com esse valor e reconhece `frt`, `trs`, `esq`, `drt` e `stop`. |

A assinatura (`home/commands`) não corresponde ao tópico que o callback aceita (`R/IOT/CTRL`); comandos MQTT, portanto, não completam o fluxo configurado. A variável `topic_command` existe, mas não é usada para assinar o tópico. A tentativa de reconexão MQTT é bloqueante e repete a conexão enquanto não tiver sucesso.

O loop verifica `millis() % 10 == 0` para publicar telemetria; isso não é um agendamento periódico preciso e pode disparar mais de uma publicação no mesmo intervalo de milissegundo.

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

## Acesso remoto com zrok

O script `zrok-digitaltwin.sh` tenta resolver `twinesp32.local` com `getent` e, como alternativa, `avahi-resolve`; depois executa comandos `zrok reserve public` e `zrok share reserved`. É necessário ter zrok instalado, habilitado e o dispositivo acessível na rede local por mDNS.

O servidor web escuta na porta 80, enquanto o firmware anuncia o serviço mDNS na porta 1080. Confirme a porta/target do compartilhamento e a sintaxe do CLI zrok para a versão instalada antes de depender do acesso remoto. Expor o servidor para fora da rede também expõe as rotas de movimento, que não têm autenticação implementada no firmware.

Consulte [ZROK_README.md](ZROK_README.md) para os passos e pré-requisitos descritos para o script. Esse guia também deve ser conferido quanto à compatibilidade com a versão atual do zrok.

## Diagnóstico

- **Inicialização travada antes do Wi-Fi:** verifique os dois MPU6050 no I²C e o endereço do segundo dispositivo.
- **Não aparece endereço IP:** confirme rede, disponibilidade do ponto de acesso e credenciais configuradas no firmware. O código espera conexão sem limite de tempo.
- **Página não abre:** confirme IP/mesma rede, servidor na porta 80 e gravação dos arquivos SPIFFS com `uploadfs`.
- **Página abre sem assets:** regrave o filesystem e verifique Serial/SPIFFS.
- **Comandos dos motores não funcionam:** confira alimentação do driver, sentido dos motores, PWM e compatibilidade física das GPIOs selecionadas.
- **RPM ou distância permanecem em zero:** as rotinas `calcularRPMRodas()` e `readUltrasonicDistance()` ainda precisam ser chamadas periodicamente.
- **MQTT sem conexão:** confirme Wi-Fi, host/porta e configuração do broker. As tentativas atuais são bloqueantes.
- **Comandos MQTT não acionam o robô:** verifique a divergência entre tópico assinado e tópico testado no callback.
- **Leituras da bússola/yaw inconsistentes:** confira o módulo QMC5883L, a calibração fixa e a orientação física do sensor.

O resumo serial periódico é produzido por `src/Debug.cpp`. Ele inclui RPM e distância, mas esses campos não são necessariamente atuais enquanto suas funções de aquisição não estiverem integradas ao loop.

## Limitações conhecidas

1. O cálculo de RPM e a leitura ultrassônica estão implementados, mas não são chamados pelo loop principal.
2. Os comandos MQTT têm divergência de tópico entre assinatura e callback.
3. O formulário de ação web não corresponde ao HTML, ao JavaScript carregado e ao método/parâmetro esperados pelo servidor.
4. O MPU2 informa taxa angular como yaw, não orientação acumulada.
5. Credenciais de Wi-Fi e MQTT estão definidas diretamente em arquivos de código; o JSON não as substitui no fluxo atual.
6. O servidor não implementa autenticação nas rotas HTTP de controle.
7. As GPIOs 6 e 7 do motor 1 são potencialmente incompatíveis com placas ESP32 comuns; valide contra o hardware efetivo.
8. O construtor do driver configura pinos antes da seleção feita pelo método `motor()`.
9. Falhas ao encontrar os MPU6050 e ao conectar ao Wi-Fi bloqueiam a inicialização sem uma estratégia de recuperação.
10. Não há suíte de testes automatizados do firmware no diretório `test/`.

Esses pontos são descrições do estado atual para orientar manutenção; este documento não altera nem corrige o firmware.

## Documentação complementar

- [Matriz de rastreabilidade](TRACEABILITY_MATRIX.md): relação entre funcionalidades/requisitos, módulos e estado de implementação.
- [Guia de acesso remoto zrok](ZROK_README.md): instruções específicas do script de compartilhamento.
- [Configuração PlatformIO](platformio.ini): placa, framework e dependências.
