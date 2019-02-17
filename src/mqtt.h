#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include "debug.h"

class Mqtt {
private:
    String clientId = "shutter";
    String broker = "192.168.0.1";
    int broker_port = 1883;
    WiFiClient wifi_client;
    PubSubClient client{wifi_client};
    uint32_t last_connection_trial = 0;
    char buffer[512];

    void reconnect_if_needed() {
        if (!client.connected()) {
            uint32_t current_time = millis();
            if (current_time - last_connection_trial < 5000) {
                return;
            }

            if (client.connect(clientId.c_str())) {
                printd("MQTT connected");
                client.subscribe("cmnd/shutter/up");
                client.subscribe("cmnd/shutter/down");
                client.subscribe("cmnd/shutter/off");
                snprintf(buffer, 50, "alive");
                client.publish("stat/shutter/state", buffer);
                last_connection_trial = 0;
            } else {
                printd("MQTT connection failed");
                last_connection_trial = current_time;
            }
        }
    }

public:
    Mqtt() {
        client.setServer(broker.c_str(), broker_port);
    }

    void handle() {
        reconnect_if_needed();
        client.loop();
    }

    void setCallback(MQTT_CALLBACK_SIGNATURE) {
        client.setCallback(callback);
    }

    void publish(const char* topic, String payload) {
        client.publish(topic, payload.c_str());
    }
};
