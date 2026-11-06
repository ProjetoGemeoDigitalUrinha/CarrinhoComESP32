// Controle do carrinho via WebSocket (ws://<ip-do-esp>/ws).
// Segure um botão de direção para andar; solte para parar.
// Enquanto o botão está pressionado, o comando é repetido a cada REPEAT_MS:
// se as mensagens pararem de chegar (Wi-Fi caiu, tela bloqueou...), o ESP32
// para o carrinho sozinho depois de ~500 ms.

const REPEAT_MS = 150;
const RECONNECT_MS = 1000;

const statusEl = document.getElementById("ws-status");
let ws = null;
let repeatTimer = null;
let activeCmd = null;

function setStatus(text, ok) {
  statusEl.textContent = text;
  statusEl.className = "ws-status " + (ok ? "ws-on" : "ws-off");
}

function connect() {
  ws = new WebSocket("ws://" + location.host + "/ws");

  ws.onopen = function () {
    setStatus("Conectado", true);
  };

  ws.onclose = function () {
    setStatus("Desconectado — reconectando...", false);
    stopMoving();
    setTimeout(connect, RECONNECT_MS);
  };

  ws.onerror = function () {
    ws.close();
  };

  ws.onmessage = function (event) {
    if (event.data.startsWith("erro:")) {
      console.warn("ESP32:", event.data);
      alert(event.data.substring(5));
    }
  };
}

function send(cmd) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(cmd);
  }
}

function startMoving(cmd) {
  if (activeCmd === cmd) return;
  clearInterval(repeatTimer);
  activeCmd = cmd;
  send(cmd);
  repeatTimer = setInterval(function () {
    send(cmd);
  }, REPEAT_MS);
}

function stopMoving() {
  clearInterval(repeatTimer);
  repeatTimer = null;
  if (activeCmd !== null) {
    activeCmd = null;
    send("stop");
  }
}

// --- Botões (toque no celular e mouse no PC) ---
document.querySelectorAll("button[data-cmd]").forEach(function (btn) {
  const cmd = btn.dataset.cmd;

  if (cmd === "stop") {
    btn.addEventListener("pointerdown", function (e) {
      e.preventDefault();
      stopMoving();
      send("stop");
    });
    return;
  }

  btn.addEventListener("pointerdown", function (e) {
    e.preventDefault();
    btn.setPointerCapture(e.pointerId);
    startMoving(cmd);
  });
  btn.addEventListener("pointerup", stopMoving);
  btn.addEventListener("pointercancel", stopMoving);
  btn.addEventListener("lostpointercapture", stopMoving);
  btn.addEventListener("contextmenu", function (e) {
    e.preventDefault();
  });
});

// --- Setas do teclado (PC) ---
const KEY_CMDS = { ArrowUp: "frt", ArrowDown: "trs", ArrowLeft: "esq", ArrowRight: "drt" };

document.addEventListener("keydown", function (e) {
  if (e.target.tagName === "INPUT") return;
  if (KEY_CMDS[e.key]) {
    e.preventDefault();
    startMoving(KEY_CMDS[e.key]);
  } else if (e.key === " " || e.key === "Escape") {
    e.preventDefault();
    stopMoving();
    send("stop");
  }
});

document.addEventListener("keyup", function (e) {
  if (KEY_CMDS[e.key] === activeCmd) stopMoving();
});

// Para o carrinho se a página perder o foco ou for para segundo plano.
window.addEventListener("blur", stopMoving);
document.addEventListener("visibilitychange", function () {
  if (document.hidden) stopMoving();
});

// --- Campo de comando (ex.: "quadrado") ---
document.getElementById("action").addEventListener("submit", function (e) {
  e.preventDefault();
  const input = this.elements.action;
  const cmd = input.value.trim().toLowerCase();
  if (!cmd) return;
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    alert("Sem conexão com o carrinho.");
    return;
  }
  send(cmd);
  input.value = "";
});

connect();
