#include "Network.h"
#include <time.h>
#include <sys/time.h>

// Namespace for network-related functionality
namespace Network { 
    
    // Connects to WiFi with given SSID and password, returns true if successful
    bool connectWiFi(const char* ssid, const char* password, uint32_t timeout_ms) {
        WiFi.begin(ssid, password);
        uint32_t startAttemptTime = millis();

        // Wait for connection or timeout
        while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < timeout_ms) {
            delay(100);
        }

        return WiFi.status() == WL_CONNECTED;
    }

    // Disconnects from WiFi
    void disconnectWiFi() {
        WiFi.disconnect();
    }

    // Returns true if WiFi is connected
    bool isWiFiConnected() {
        return WiFi.status() == WL_CONNECTED;
    }

    // Starts mDNS with given hostname, returns true if successful
    bool beginMDNS(const char* hostname) {
        if (!MDNS.begin(hostname)) {
            Serial.println("Error setting up MDNS responder!");
            return false;
        }
        Serial.println("MDNS responder started");
        return true;
    }
    // Adds a service to mDNS
    bool addMDNSService(const char* service, const char* proto, uint16_t port) {
        return MDNS.addService(service, proto, port);
    }
    // Stops mDNS
    void endMDNS() {
        MDNS.end();
    }

    // --- Relógio (NTP) ---
    // Qualquer instante depois de 01/01/2024 indica que o NTP já acertou o relógio.
    static const time_t MIN_VALID_EPOCH = 1704067200;

    void beginNTP(const char* tz, const char* server1, const char* server2) {
        configTzTime(tz, server1, server2);
    }

    bool isTimeSynced() {
        return time(nullptr) >= MIN_VALID_EPOCH;
    }

    uint64_t epochMillis() {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        if (tv.tv_sec < MIN_VALID_EPOCH) return 0;
        return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)(tv.tv_usec / 1000);
    }

    bool isoTimestamp(char* buf, size_t len) {
        if (len > 0) buf[0] = '\0';
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        if (tv.tv_sec < MIN_VALID_EPOCH || len < 30) return false;

        struct tm local;
        localtime_r(&tv.tv_sec, &local);
        char base[20];  // "YYYY-MM-DDTHH:MM:SS"
        strftime(base, sizeof(base), "%Y-%m-%dT%H:%M:%S", &local);
        char tzRaw[6];  // "-0300"
        strftime(tzRaw, sizeof(tzRaw), "%z", &local);
        // ISO 8601 pede o offset com dois-pontos: -0300 -> -03:00
        snprintf(buf, len, "%s.%03ld%c%c%c:%c%c", base, (long)(tv.tv_usec / 1000),
                 tzRaw[0], tzRaw[1], tzRaw[2], tzRaw[3], tzRaw[4]);
        return true;
    }

} // namespace Network