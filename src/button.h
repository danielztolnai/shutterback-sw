#pragma once
#include <Arduino.h>
#include <functional>
#include "debug.h"

class Button {
private:
    enum ButtonStates {
        IDLE = 0,
        PRESSED,
        SHORT_PRESS,
        LONG_PRESS,
    };

    uint8_t id = 0xff;
    enum ButtonStates state = IDLE;
    uint32_t time_short_press = 100;
    uint32_t time_long_press = 750;

    uint32_t last_time = 0;
    std::function<void(uint8_t button)> _action_press = nullptr;
    std::function<void(uint8_t button)> _action_short = nullptr;
    std::function<void(uint8_t button)> _action_long = nullptr;

    void state_button_idle(uint8_t value) {
        if (value == HIGH) {
            state = PRESSED;
            printd("Button pressed: %d", id);
            last_time = millis();
        }
    }

    void state_button_pressed(uint8_t value) {
        if (value == HIGH) {
            uint32_t time_elapsed = millis() - last_time;
            if (time_elapsed > time_short_press) {
                state = SHORT_PRESS;
                printd("Button short: %d", id);
                // Run handler
                if (_action_press != nullptr) {
                    _action_press(id);
                }
            }
        } else { // LOW
            state = IDLE;
            printd("Button released: %d", id);
        }
    }

    void state_button_short(uint8_t value) {
        if (value == HIGH) {
            uint32_t time_elapsed = millis() - last_time;
            if (time_elapsed > time_long_press) {
                printd("Button long: %d", id);
                state = LONG_PRESS;
            }
        } else { // LOW
            state = IDLE;
            printd("Button short released: %d", id);
            // Run handler
            if (_action_short != nullptr) {
                _action_short(id);
            }
        }
    }

    void state_button_long(uint8_t value) {
        if (value == LOW) {
            state = IDLE;
            printd("Button long released: %d", id);
            // Run handler
            if (_action_long != nullptr) {
                _action_long(id);
            }
        }
    }

public:
    void begin(uint8_t id) {
        this->id = id;
    }

    void new_value(uint8_t value) {
        switch (state) {
            case IDLE:
                state_button_idle(value);
                break;

            case PRESSED:
                state_button_pressed(value);
                break;

            case SHORT_PRESS:
                state_button_short(value);
                break;

            case LONG_PRESS:
                state_button_long(value);
                break;
        }
    }

    void onPress(std::function<void(uint8_t button)> fn) {
        _action_press = fn;
    }

    void onShort(std::function<void(uint8_t button)> fn) {
        _action_short = fn;
    }

    void onLong(std::function<void(uint8_t button)> fn) {
        _action_long = fn;
    }

    void set_time_short_press(uint32_t value) {time_short_press = value;}
    void set_time_long_press(uint32_t value) {time_long_press = value;}
};