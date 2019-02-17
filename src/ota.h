#pragma once
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "debug.h"

class Upgrade {
private:
    bool is_initialized = false;
    unsigned int last_progress = 0;
public:
    void setup() {
        ArduinoOTA.setHostname("shutter");

        ArduinoOTA.onStart([]() {
            if (ArduinoOTA.getCommand() == U_FLASH) {
                printd("Start updating sketch");
            } else { // U_SPIFFS
                printd("Start updating SPIFFS");
            }
            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        });

        ArduinoOTA.onEnd([]() {
            printd("End");
        });

        ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
            unsigned int progress_percent = progress / (total / 100);
            if (progress_percent % 10 != 0) {
                return;
            }
            if (this->last_progress != progress_percent) {
                printd("Update progress: %u%%", progress_percent);
                this->last_progress = progress_percent;
            }
        });
        
        ArduinoOTA.onError([](ota_error_t error) {
            if (error == OTA_AUTH_ERROR) printd("Upgrade error: Auth Failed");
            else if (error == OTA_BEGIN_ERROR) printd("Upgrade error: Begin Failed");
            else if (error == OTA_CONNECT_ERROR) printd("Upgrade error: Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) printd("Upgrade error: Receive Failed");
            else if (error == OTA_END_ERROR) printd("Upgrade error: End Failed");
        });
    }

    void handle()
    {
        if (!WiFi.isConnected()) {
            return;
        }

        if (this->is_initialized) {
            ArduinoOTA.handle();
        } else {
            ArduinoOTA.begin();
            this->is_initialized = true;
            printd("Upgrade system ready");
        }
    }
};