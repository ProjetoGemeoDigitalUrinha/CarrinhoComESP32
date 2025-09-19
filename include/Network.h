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
}

#endif // NETWORK_H
