#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <vector>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "button.h"
#include "debug.h"
#include "motor.h"
#include "mqtt.h"
#include "ota.h"
#include "pca9534.h"
#include "relay.h"

#define NUM_BUTTONS   20
#define NUM_RELAYS    16
#define NUM_MOTORS    8
#define PORT_INTERNAL nullptr
#define TIME_NORMAL   (33*1000)
#define TIME_THIN     (37*1000)
#define TIME_BIG      (73*1000)

static void mqtt_callback(char* topic, byte* payload, unsigned int length);

const std::vector<int> port_internal = {25, 26, 32, 33};
PCA9534 port_out(0x20);
PCA9534 port_mixed(0x22);
PCA9534 port_in_a(0x21);
PCA9534 port_in_b(0x24);

const struct {
    PCA9534 *up_port;
    uint8_t up_pin;
    PCA9534 *down_port;
    uint8_t down_pin;
    uint32_t timer;
} config_motor[] {
    [0] = {PORT_INTERNAL, 26, PORT_INTERNAL, 25, TIME_THIN},
    [1] = {PORT_INTERNAL, 33, PORT_INTERNAL, 32, TIME_THIN},
    [2] = {&port_mixed,    5, &port_mixed,    4, TIME_NORMAL},
    [3] = {&port_mixed,    7, &port_mixed,    6, TIME_NORMAL},
    [4] = {&port_out,      1, &port_out,      0, TIME_NORMAL},
    [5] = {&port_out,      3, &port_out,      2, TIME_NORMAL},
    [6] = {&port_out,      4, &port_out,      5, TIME_BIG},
    [7] = {&port_out,      6, &port_out,      7, TIME_THIN},
};

const struct {
    PCA9534 *port;
    uint8_t pin;
    uint8_t motor_id;
    enum Motor::MotorStates direction;
} config_button[] {
    {&port_in_b,  7, 3, Motor::MotorStates::UP},   // Nappali ablak fel
    {&port_in_b,  5, 3, Motor::MotorStates::DOWN}, // Nappali ablak le
    {&port_mixed, 1, 1, Motor::MotorStates::UP},   // Nappali 1 fel
    {&port_mixed, 3, 1, Motor::MotorStates::DOWN}, // Nappali 1 le
    {&port_in_a,  1, 0, Motor::MotorStates::UP},   // Nappali 2 fel
    {&port_in_a,  3, 0, Motor::MotorStates::DOWN}, // Nappali 2 le
    {&port_in_a,  6, 7, Motor::MotorStates::UP},   // Nappali 3 fel
    {&port_in_a,  4, 7, Motor::MotorStates::DOWN}, // Nappali 3 le
    {&port_in_b,  1, 6, Motor::MotorStates::UP},   // Nappali ajtó fel
    {&port_in_b,  3, 6, Motor::MotorStates::DOWN}, // Nappali ajtó le
    {&port_in_b,  6, 6, Motor::MotorStates::UP},   // Nappali közös fel
    {&port_in_b,  4, 6, Motor::MotorStates::DOWN}, // Nappali közös le
    {&port_in_a,  7, 2, Motor::MotorStates::UP},   // Konyha fel
    {&port_in_a,  5, 2, Motor::MotorStates::DOWN}, // Konyha le
    {&port_in_b,  2, 2, Motor::MotorStates::UP},   // Bejárat fel
    {&port_in_b,  0, 2, Motor::MotorStates::DOWN}, // Bejárat le
    {&port_mixed, 0, 4, Motor::MotorStates::UP},   // Dolgozó fel
    {&port_mixed, 2, 4, Motor::MotorStates::DOWN}, // Dolgozó le
    {&port_in_a,  0, 5, Motor::MotorStates::UP},   // Vendég fel 
    {&port_in_a,  2, 5, Motor::MotorStates::DOWN}, // Vendég le
};

#error "Please set the SSID and password"
const char* ssid = "";
const char* password = "";

Upgrade upgrader;
Mqtt mqtt;
bool wifi_is_connected = false;

Motor motors[NUM_MOTORS];
Button buttons[NUM_BUTTONS];

void setup() {
    // Configure serial
    Serial.begin(115200);

    // Configure I2C
    Wire.begin();

    // Configure WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // Configure MQTT
    mqtt.setCallback(mqtt_callback);

    // Setup OTA
    upgrader.setup();

    // Wait for peripherals
    sleep(1);

    // Configure pins
    for (const int& i : port_internal) {
        pinMode(i, OUTPUT);
        digitalWrite(i, LOW);
    }
    
    port_out.begin();
    port_out.configure(0x00);

    port_mixed.begin();
    port_mixed.configure(0x0f);

    port_in_a.begin();
    port_in_a.configure(0xff);
    
    port_in_b.begin();
    port_in_b.configure(0xff);

    // Configure motors
    for (uint8_t i = 0; i < NUM_MOTORS; ++i) {
        motors[i].begin(
            i,
            Relay(config_motor[i].up_port, config_motor[i].up_pin),
            Relay(config_motor[i].down_port, config_motor[i].down_pin)
        );
    }

    // Configure buttons
    for (uint8_t i = 0; i < NUM_BUTTONS; ++i) {
        buttons[i].begin(i);

        uint8_t motor_id = config_button[i].motor_id;
        Motor::MotorStates direction = config_button[i].direction;
        uint32_t motor_timer = config_motor[motor_id].timer;

        buttons[i].onPress([=](uint8_t button){
            motors[motor_id].toggle(direction);
        });

        buttons[i].onShort([=](uint8_t button){
            if (motors[motor_id].getState() != Motor::MotorStates::OFF) {
                motors[motor_id].timer_set(motor_timer);
            }
        });

        buttons[i].onLong([=](uint8_t button){
            motors[motor_id].off();
        });
    }

    // Configure special buttons
    for (uint8_t i = 10; i < 12; ++i) {
        Motor::MotorStates direction = config_button[i].direction;

        buttons[i].onPress([=](uint8_t button){
            for (const int& m : {0, 1, 3, 6, 7}) {
                motors[m].toggle(direction);
            }
        });

        buttons[i].onShort([=](uint8_t button){
            for (const int& m : {0, 1, 3, 6, 7}) {
                if (motors[m].getState() != Motor::MotorStates::OFF) {
                    motors[m].timer_set(config_motor[m].timer);
                }
            }
        });

        buttons[i].onLong([=](uint8_t button){
            for (const int& m : {0, 1, 3, 6, 7}) {
                motors[m].off();
            }
        });
    }

    for (uint8_t i = 14; i < 15; ++i) {
        Motor::MotorStates direction = config_button[i].direction;

        buttons[i].onPress([=](uint8_t button){
            for (uint8_t m = 0; m < NUM_MOTORS; ++m) {
                motors[m].toggle(direction);
            }
        });

        buttons[i].onShort([=](uint8_t button){
            for (uint8_t m = 0; m < NUM_MOTORS; ++m) {
                if (motors[m].getState() != Motor::MotorStates::OFF) {
                    motors[m].timer_set(config_motor[m].timer);
                }
            }
        });

        buttons[i].onLong([=](uint8_t button){
            for (uint8_t m = 0; m < NUM_MOTORS; ++m) {
                motors[m].off();
            }
        });
    }

    // Ready
    Serial.println("Setup done");
}

void loop() {
    // Handle WiFi
    if (!wifi_is_connected && WiFi.isConnected()) {
        wifi_is_connected = true;
        Serial.print("WiFi connected, IP = ");
        Serial.println(WiFi.localIP());
        debug_setup();
    }

    if (wifi_is_connected) {
        upgrader.handle(); // Handle OTA
        mqtt.handle();     // Handle MQTT
        debug_handle();    // Handle telnet debug
    }

    // Handle buttons
    for (uint8_t i = 0; i < NUM_BUTTONS; ++i) {
        uint8_t value = config_button[i].port->digitalRead(config_button[i].pin);
        buttons[i].new_value(value);
    }

    // Handle motors
    for (uint8_t i = 0; i < NUM_MOTORS; ++i) {
        motors[i].timer_handle();
    }

    // Handle port expanders
    port_mixed.handle();
    port_in_a.handle();
    port_in_b.handle();
    port_out.handle();
}


void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
    // Convert payload
    char buf[512];
    int i;
    for (i = 0; i < length; i++) {
        buf[i] = payload[i];
    }
    buf[i] = '\0';

    long int channel = strtol(buf, nullptr, 10);

    // Convert topic to direction
    Motor::MotorStates direction;
    if (strncmp(topic, "cmnd/shutter/up", 15) == 0) {
        direction = Motor::MotorStates::UP;
    } else if (strncmp(topic, "cmnd/shutter/down", 15) == 0) {
        direction = Motor::MotorStates::DOWN;
    } else {
        direction = Motor::MotorStates::OFF;
    }

    // Print received message
    String recv = "MQTT received: Motor {";
    recv += channel;
    recv += "} command ";
    switch (direction) {
        case Motor::MotorStates::UP:
            recv += "UP";
            break;
        case Motor::MotorStates::DOWN:
            recv += "DOWN";
            break;
        default: // OFF
            printd("MQTT received command OFF");
            for (uint8_t i = 0; i < NUM_MOTORS; ++i) {
                motors[i].off();
            }
            mqtt.publish("stat/shutter/state", "All off");
            return;
            break;
    }
    printd(recv.c_str());

    // Sanity check
    if (channel >= NUM_MOTORS) {
        printd("MQTT command for motor {%d} out of range", channel);
        return;
    }

    // Execute command
    uint32_t motor_timer = config_motor[channel].timer;
    motors[channel].toggle(direction);
    if (motors[channel].getState() != Motor::MotorStates::OFF) {
        motors[channel].timer_set(motor_timer);
    }

    // Respond
    String resp = "{";
    resp += channel;
    resp += "}";
    switch (motors[channel].getState()) {
        case Motor::MotorStates::UP:
            resp += "↑";
            break;
        case Motor::MotorStates::DOWN:
            resp += "↓";
            break;
        default:
            resp += "x";
            break;
    }
    mqtt.publish("stat/shutter/state", resp.c_str());
}