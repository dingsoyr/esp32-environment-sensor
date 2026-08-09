#include <WiFi.h>

#include "logging.h"
#include "wifi_utils.h"
#include "secrets.h"

constexpr uint32_t WIFI_TIMEOUT_MS = 10000;

bool isWifiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool connectWifi() {
    if (isWifiConnected()) {
        LOG_PRINTLN("Wi-Fi already connected.");
        return true;
    }

    LOG_PRINTF("Connecting to Wi-Fi: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);

    const unsigned long start = millis();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= WIFI_TIMEOUT_MS) {
            LOG_PRINTLN("Wi-Fi connection timed out.");
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            return false;
        }

        delay(100);
    }

    const unsigned long duration = millis() - start;

    LOG_PRINTLN("Wi-Fi connected.");
    LOG_PRINTF("IP address: %s\n", WiFi.localIP().toString().c_str());
    LOG_PRINTF("RSSI: %d dBm\n", WiFi.RSSI());
    LOG_PRINTF("Connection time: %lu ms\n", duration);

    return true;
}

void disconnectWifi() {
    if (!isWifiConnected()) {
        WiFi.mode(WIFI_OFF);
        LOG_PRINTLN("Wi-Fi disconnected.");
        return;
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    LOG_PRINTLN("Wi-Fi disconnected.");
}