#pragma once
#include "pca9534.h"

class Relay {
private:
    PCA9534 *port = nullptr;
    uint8_t pin = 0xff;
public:
    Relay(){}

    Relay(PCA9534 *port, uint8_t pin) {
        this->port = port;
        this->pin = pin;
    }

    void set(uint8_t value) {
        if (port != nullptr) {
            port->digitalWrite(pin, value);
        } else { // Internal port
            digitalWrite(pin, value);
        }
    }

    uint8_t get() {
        if (port != nullptr) {
            return port->digitalRead(pin);
        } else { // Internal port
            return digitalRead(pin);
        }
    }

    void on() {set(HIGH);}
    void off() {set(LOW);}
};