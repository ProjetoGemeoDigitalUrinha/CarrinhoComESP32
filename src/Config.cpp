#include "Config.h"
#include <LittleFS.h> 
#include <Arduino.h>
#include <ArduinoJson.h>

void loadConfig(StaticJsonDocument<200>& document) {
    // 1. Verifica se o arquivo existe na memória. Se não existir, cria um vazio/padrão
    if (!LittleFS.exists("/config.json")) {
        Serial.println("config.json nao encontrado. Criando arquivo padrao...");
        File configFile = LittleFS.open("/config.json", FILE_WRITE);
        if (configFile) {
            StaticJsonDocument<200> defaultDoc;
            defaultDoc["configurado"] = true; // Exemplo de chave JSON padrao
            serializeJson(defaultDoc, configFile);
            configFile.close();
            Serial.println("config.json padrao criado com sucesso!");
        } else {
            Serial.println("Erro critico: Nao foi possivel criar o config.json");
        }
    }

    // 2. Abre o arquivo com segurança para leitura
    File configFile = LittleFS.open("/config.json", FILE_READ);
    if (!configFile) {
        Serial.println("Can't read config.json, please restart and try again.");
        return;
    }
    
    DeserializationError error = deserializeJson(document, configFile);
    configFile.close();
    
    if (error) {
        Serial.print("Erro de deserializacao: ");
        Serial.println(error.f_str());
        return;
    }
    Serial.println("Configuracoes JSON lidas com sucesso via LittleFS!");
}
