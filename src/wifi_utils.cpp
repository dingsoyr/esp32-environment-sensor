#include <WiFi.h>

#include "wifi_utils.h"
#include "secrets.h"

constexpr uint32_t WIFI_TIMEOUT_MS = 10000;

bool isWifiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool connectWifi() {
    if (isWifiConnected()) {
        Serial.println("Wi-Fi already connected.");
        return true;
    }

    Serial.printf("Connecting to Wi-Fi: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);

    const unsigned long start = millis();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= WIFI_TIMEOUT_MS) {
            Serial.println("Wi-Fi connection timed out.");
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            return false;
        }

        delay(100);
    }

    const unsigned long duration = millis() - start;

    Serial.println("Wi-Fi connected.");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("Connection time: %lu ms\n", duration);

    return true;
}

void disconnectWifi() {
    if (!isWifiConnected()) {
        WiFi.mode(WIFI_OFF);
        Serial.println("Wi-Fi disconnected.");
        return;
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    Serial.println("Wi-Fi disconnected.");
}