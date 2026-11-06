#ifndef WEBSOCKET_CONTROL_H
#define WEBSOCKET_CONTROL_H

#include <ESPAsyncWebServer.h>

// Controle de movimento do carrinho via WebSocket em ws://<ip-do-esp>/ws.
//
// Protocolo (mensagens de texto):
//   "frt"      -> frente
//   "trs"      -> ré
//   "esq"      -> esquerda
//   "drt"      -> direita
//   "stop"     -> parar
//   "quadrado" -> executa a sequência do quadrado (qualquer outro comando a interrompe)
//
// Enquanto um botão de direção está pressionado, a página repete o comando a cada
// ~150 ms. Se nenhum comando chegar em 500 ms, o carrinho para sozinho (proteção
// contra queda do Wi-Fi ou celular travado com o botão pressionado).
namespace WebSocketControl {
    // Registra o endpoint /ws no servidor (chamar antes de server.begin())
    // e inicia a tarefa que aciona os motores.
    void setup(AsyncWebServer& server);

    // Chamar no loop(): libera clientes WebSocket desconectados.
    void loop();
}

#endif
