#pragma once
#include <WiFi.h>

class Wireless {
private:
    const char* ssid = "";
    const char* password = "";
public:
    void setup() {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
    }

    bool isConnected() {
        return WiFi.isConnected();
    }
};