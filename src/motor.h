#pragma once
#include <Arduino.h>
#include "debug.h"
#include "relay.h"

class Motor {
public:
    enum MotorStates {
        OFF,
        UP,
        DOWN,
    };

private:
    uint8_t id;
    Relay relay_up;
    Relay relay_down;
    bool timer_running = false;
    uint32_t timer_time = 0;
    uint32_t timer_time_start = 0;
    MotorStates state = OFF;

public:
    Motor(){}

    void begin(uint8_t id, Relay relay_up, Relay relay_down) {
        this->relay_up = relay_up;
        this->relay_down = relay_down;
        this->id= id;
    }

    uint8_t getId() {return this->id;}
    enum MotorStates getState() {return this->state;}

    void off() {
        relay_down.off();
        relay_up.off();
        state = OFF;
        if (timer_running == true) {
            timer_cancel();
        }
        printd("Motor {%d} stopped", id);
    }

    void up() {
        relay_down.off();
        relay_up.on();
        state = UP;
        printd("Motor {%d} going up", id);
    }

    void down() {
        relay_up.off();
        relay_down.on();
        state = DOWN;
        printd("Motor {%d} going down", id);
    }

    void set(enum MotorStates direction) {
        switch (direction) {
            case Motor::MotorStates::UP:
                this->up();
                break;
            case Motor::MotorStates::DOWN:
                this->down();
                break;
            default: // OFF
                this->off();
                break;
        }
    }

    void toggle(enum MotorStates direction) {
        if (this->state == OFF) {
            set(direction);
        } else { // UP or DOWN
            off();
            timer_cancel();
        }
    }

    enum MotorStates get() {
        uint8_t state_up = relay_up.get();
        uint8_t state_down = relay_down.get();
        if (state_up == LOW && state_down == LOW) {
            return Motor::MotorStates::OFF;
        } else if (state_up == LOW && state_down == HIGH) {
            return Motor::MotorStates::DOWN;
        } else if (state_up == HIGH && state_down == LOW) {
            return Motor::MotorStates::UP;
        } else { // both HIGH
            printd("Error: Both relays on for motor with id %d", getId());
            return Motor::MotorStates::OFF;
        }
    }

    void timer_set(uint32_t ms) {
        timer_time = ms;
        timer_time_start = millis();
        timer_running = true;
        printd("Motor {%d} timer set to %lu ms", id, timer_time);
    }

    void timer_cancel() {
        timer_running = false;
        printd("Motor {%d} timer cancelled", id);
    }

    void timer_handle() {
        if (timer_running == false) {
            return;
        }

        uint32_t time_current = millis();
        if (time_current - timer_time_start >= timer_time) {
            printd("Motor {%d} timer elapsed", id);
            off();
        }
    }
};