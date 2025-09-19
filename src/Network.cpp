#include "Network.h"

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

} // namespace Network