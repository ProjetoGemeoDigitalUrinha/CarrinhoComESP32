#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <ESPmDNS.h>

// Namespace for network-related functionality
namespace Network {

    // Connects to WiFi with given SSID and password, returns true if successful
    bool connectWiFi(const char* ssid, const char* password, uint32_t timeout_ms = 10000);

    // Disconnects from WiFi
    void disconnectWiFi();

    // Returns true if WiFi is connected
    bool isWiFiConnected();

    // SoftAP mode setup (if needed)
    bool setupSoftAP(const char* ssid, const char* password);

    // Adds a service to mDNS
    bool addMDNSService(const char* service, const char* proto, uint16_t port);


    // Starts mDNS with given hostname, returns true if successful
    bool beginMDNS(const char* hostname);

    // Stops mDNS
    void endMDNS();

    // Inicia a sincronização do relógio via NTP (não bloqueia; o SNTP roda em segundo plano
    // e ressincroniza sozinho). tz no formato POSIX, ex.: "<-03>3" para UTC-3 (Brasília).
    void beginNTP(const char* tz, const char* server1, const char* server2 = nullptr);

    // true quando o relógio já foi acertado pelo NTP (antes disso a hora começa em 1970).
    bool isTimeSynced();

    // Horário atual em milissegundos desde a época Unix (UTC). Retorna 0 se não sincronizado.
    uint64_t epochMillis();

    // Escreve o horário local em ISO 8601 com ms, ex.: "2026-11-18T14:05:00.123-03:00".
    // Retorna false (e buf vazio) se o relógio ainda não foi sincronizado.
    bool isoTimestamp(char* buf, size_t len);
}

#endif // NETWORK_H
