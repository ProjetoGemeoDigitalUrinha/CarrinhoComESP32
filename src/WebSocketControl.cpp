#include "WebSocketControl.h"
#include "main_globals.h"

namespace {

enum class Cmd : uint8_t { NONE, STOP, FORWARD, BACKWARD, LEFT, RIGHT, SQUARE };

const int MOTOR_SPEED = 255;
// Partida suave: o PWM sobe RAMP_STEP a cada ciclo da tarefa (20 ms), indo de 0
// a MOTOR_SPEED em ~300 ms. Ligar os dois motores direto em 100% gera um pico de
// corrente que derruba a tensão das baterias/MT3608 e reinicia o ESP (brownout).
const int RAMP_STEP = 17;
const uint32_t COMMAND_TIMEOUT_MS = 500;
const TickType_t MOTOR_TASK_PERIOD = pdMS_TO_TICKS(20);
const uint32_t CLEANUP_INTERVAL_MS = 1000;

AsyncWebSocket ws("/ws");

// Comando recebido pelo WebSocket (tarefa do AsyncTCP) e consumido pela tarefa
// dos motores. Protegido por cmdMux.
portMUX_TYPE cmdMux = portMUX_INITIALIZER_UNLOCKED;
Cmd pendingCmd = Cmd::NONE;
uint32_t lastCmdMs = 0;

// Mesma sequência da antiga rota /action?action=quadrado, agora sem bloquear:
// cada lado = anda 2 s, pausa 2 s, gira 300 ms, pausa 2 s.
struct Step { Cmd cmd; uint32_t durationMs; };
const Step SQUARE_SIDE[] = {
    {Cmd::FORWARD, 2000},
    {Cmd::STOP,    2000},
    {Cmd::RIGHT,    300},
    {Cmd::STOP,    2000},
};
const size_t SQUARE_SIDE_STEPS = sizeof(SQUARE_SIDE) / sizeof(SQUARE_SIDE[0]);
const size_t SQUARE_TOTAL_STEPS = SQUARE_SIDE_STEPS * 4;

void postCommand(Cmd cmd) {
    uint32_t now = millis();
    portENTER_CRITICAL(&cmdMux);
    pendingCmd = cmd;
    lastCmdMs = now;
    portEXIT_CRITICAL(&cmdMux);
}

Cmd parseCommand(const char* msg) {
    if (strcmp(msg, "frt") == 0) return Cmd::FORWARD;
    if (strcmp(msg, "trs") == 0) return Cmd::BACKWARD;
    if (strcmp(msg, "esq") == 0) return Cmd::LEFT;
    if (strcmp(msg, "drt") == 0) return Cmd::RIGHT;
    if (strcmp(msg, "stop") == 0) return Cmd::STOP;
    if (strcmp(msg, "quadrado") == 0) return Cmd::SQUARE;
    return Cmd::NONE;
}

// Liga os motores na velocidade dada e retorna imediatamente (sem delay/parada automática).
void applyMotion(Cmd cmd, int speed) {
    switch (cmd) {
        case Cmd::FORWARD:  robot.forward(speed, 0, false);  break;
        case Cmd::BACKWARD: robot.backward(speed, 0, false); break;
        case Cmd::LEFT:     robot.left(speed, 0, false);     break;
        case Cmd::RIGHT:    robot.right(speed, 0, false);    break;
        default:            robot.stop();                    break;
    }
}

// Única tarefa que mexe nos motores. Roda separada do loop() para que a parada
// por timeout funcione mesmo que o loop esteja ocupado (MQTT, sensores etc.).
void motorTask(void*) {
    Cmd current = Cmd::STOP;
    int speed = 0;
    bool squareRunning = false;
    size_t squareStep = 0;
    uint32_t stepStartMs = 0;

    // Troca de movimento: para os motores e recomeça a rampa do zero. Isso também
    // evita inverter o sentido de giro em velocidade máxima (outro pico de corrente).
    auto setMotion = [&](Cmd cmd) {
        current = cmd;
        speed = 0;
        robot.stop();
    };

    robot.stop();

    for (;;) {
        portENTER_CRITICAL(&cmdMux);
        Cmd cmd = pendingCmd;
        pendingCmd = Cmd::NONE;
        uint32_t lastMs = lastCmdMs;
        portEXIT_CRITICAL(&cmdMux);

        uint32_t now = millis();

        if (cmd == Cmd::SQUARE) {
            squareRunning = true;
            squareStep = 0;
            stepStartMs = now;
            setMotion(SQUARE_SIDE[0].cmd);
        } else if (cmd != Cmd::NONE) {
            squareRunning = false;
            if (cmd != current) {
                setMotion(cmd);
            }
        }

        if (squareRunning) {
            if (now - stepStartMs >= SQUARE_SIDE[squareStep % SQUARE_SIDE_STEPS].durationMs) {
                squareStep++;
                stepStartMs = now;
                squareRunning = (squareStep < SQUARE_TOTAL_STEPS);
                setMotion(squareRunning ? SQUARE_SIDE[squareStep % SQUARE_SIDE_STEPS].cmd
                                        : Cmd::STOP);
            }
        } else if (current != Cmd::STOP && now - lastMs > COMMAND_TIMEOUT_MS) {
            setMotion(Cmd::STOP);
        }

        // Rampa de aceleração.
        if (current != Cmd::STOP && speed < MOTOR_SPEED) {
            speed = min(speed + RAMP_STEP, MOTOR_SPEED);
            applyMotion(current, speed);
        }

        vTaskDelay(MOTOR_TASK_PERIOD);
    }
}

void onEvent(AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
             void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket: cliente #%u conectado (%s)\n",
                          client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket: cliente #%u desconectado\n", client->id());
            postCommand(Cmd::STOP);
            break;
        case WS_EVT_DATA: {
            AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);
            // Aceita só mensagens de texto curtas, em um único frame.
            if (!info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) {
                return;
            }
            char msg[16];
            if (len >= sizeof(msg)) {
                client->text("erro:comando muito longo");
                return;
            }
            memcpy(msg, data, len);
            msg[len] = '\0';

            Cmd cmd = parseCommand(msg);
            if (cmd == Cmd::NONE) {
                client->text(String("erro:comando desconhecido:") + msg);
                return;
            }
            postCommand(cmd);
            break;
        }
        default:
            break;
    }
}

} // namespace

namespace WebSocketControl {

void setup(AsyncWebServer& server) {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    // Prioridade acima do loop() (1) para não atrasar a parada por timeout.
    xTaskCreatePinnedToCore(motorTask, "motorTask", 4096, nullptr, 2, nullptr, ARDUINO_RUNNING_CORE);
}

void loop() {
    static uint32_t lastCleanupMs = 0;
    if (millis() - lastCleanupMs >= CLEANUP_INTERVAL_MS) {
        lastCleanupMs = millis();
        ws.cleanupClients();
    }
}

} // namespace WebSocketControl
