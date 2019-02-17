#pragma once
#include <Arduino.h>
#include <Wire.h>

class PCA9534 {
private:
    enum registers {
        REG_INPUT_PORT = 0x00,
        REG_OUTPUT_PORT = 0x01,
        REG_POLARITY_INVERSION = 0x02,
        REG_CONFIGURATION = 0x03,
    };

    uint8_t _address; // I2C address of the device
    uint8_t _configuration = 0xff; // All pins are inputs initially
    uint8_t _reg_output = 0x00;
    uint8_t _reg_input = 0x00;

    void writeRegister(enum registers reg, uint8_t value) {
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.write(value);
        Wire.endTransmission();
    }

    uint8_t readRegister(enum registers reg) {
        uint8_t value = 0;
        // Send register to read from
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.endTransmission();
        // Read a single byte
        Wire.requestFrom(_address, 1u);
        if (1 <= Wire.available()) {
            value = Wire.read();
        }
        return value;
    }

public:
    PCA9534(uint8_t address) {
        _address = address;
    }

    void begin() {
        writeRegister(REG_CONFIGURATION, _configuration);
        writeRegister(REG_OUTPUT_PORT, _reg_output);
        _reg_input = readRegister(REG_INPUT_PORT);
    }

    void handle() {
        _reg_input = readRegister(REG_INPUT_PORT);
        writeRegister(REG_OUTPUT_PORT, _reg_output);
        writeRegister(REG_CONFIGURATION, _configuration);
    }

    void configure(uint8_t config) {
        _configuration = config;
        writeRegister(REG_CONFIGURATION, _configuration);
    }

    void pinMode(uint8_t pin, uint8_t mode) {
        uint8_t value = (mode == OUTPUT) ? 0 : 1;
        bitWrite(_configuration, pin, value);
        writeRegister(REG_CONFIGURATION, _configuration);
    }

    void digitalWrite(uint8_t pin, uint8_t value) {
        bitWrite(_reg_output, pin, value);
        writeRegister(REG_OUTPUT_PORT, _reg_output);
    }

    uint8_t digitalRead(uint8_t pin) {
        return bitRead(_reg_input, pin);
    }
};