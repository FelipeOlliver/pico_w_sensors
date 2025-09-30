/*
 * ina219.c
 *
 * Created on: 2 Nov 2020
 * Author: Tockn
 *
 * INA219-c is a library for the INA219 current sensor.
 * It is written in C and is platform-independent.
 *
 * MIT License
 *
 * Copyright (c) 2020 Tockn
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ina219.h"

static INA219_I2C_INSTANCE _i2c;
static uint8_t _addr;
static float _current_lsb;
static uint16_t _cal;
static uint16_t _config;

/*
 * @brief Write a 16-bit value to a register
 * @param reg The register to write to.
 * @param val The value to write.
 */
static void write_register(uint8_t reg, uint16_t val) {
    uint8_t data[3];
    data[0] = reg;
    data[1] = (val >> 8) & 0xFF;
    data[2] = val & 0xFF;

#ifdef PICO_SDK_VERSION_STRING
    i2c_write_blocking(_i2c, _addr, data, 3, false);
#else
    // Generic I2C write function placeholder
    // i2c_write(_i2c, _addr, data, 3);
#endif
}

/*
 * @brief Read a 16-bit value from a register
 * @param reg The register to read from.
 * @return The value read from the register.
 */
static uint16_t read_register(uint8_t reg) {
    uint8_t data[2];
    
#ifdef PICO_SDK_VERSION_STRING
    i2c_write_blocking(_i2c, _addr, &reg, 1, true); // true to keep master control
    i2c_read_blocking(_i2c, _addr, data, 2, false);
#else
    // Generic I2C read function placeholder
    // i2c_read(_i2c, _addr, &reg, 1, data, 2);
#endif

    return ((data[0] << 8) | data[1]);
}


bool ina219_init(INA219_I2C_INSTANCE i2c, uint8_t addr) {
    _i2c = i2c;
    _addr = addr;

    // A simple check to see if the device is responding
#ifdef PICO_SDK_VERSION_STRING
    uint8_t dummy_data;
    int result = i2c_read_blocking(_i2c, _addr, &dummy_data, 1, false);
    if (result < 0) {
        return false; // Device not found
    }
#endif
    
    ina219_reset();
    return true;
}

void ina219_configure(ina219_range_t range, ina219_gain_t gain,
                      ina219_bus_adc_t bus_adc, ina219_shunt_adc_t shunt_adc,
                      ina219_mode_t mode) {
    _config = (range << 13) | (gain << 11) | (bus_adc << 7) | (shunt_adc << 3) | mode;

    // For the standard module with a 0.1 Ohm shunt resistor, this calibration
    // allows for a maximum current of 3.2A.
    //_current_lsb is 100uA per bit (1/10000)
    //_cal is 4096
    ina219_calibrate(0.0001, 4096);
}

void ina219_calibrate(float current_lsb, uint16_t cal) {
    _current_lsb = current_lsb;
    _cal = cal;
    write_register(INA219_REG_CALIBRATION, _cal);
    write_register(INA219_REG_CONFIG, _config);
}


void ina219_reset() {
    write_register(INA219_REG_CONFIG, 0x8000);
}

float ina219_get_bus_voltage() {
    uint16_t val = read_register(INA219_REG_BUS_VOLTAGE);
    return (val >> 3) * 0.004; // LSB is 4mV
}

float ina219_get_shunt_voltage_mv() {
    int16_t val = (int16_t)read_register(INA219_REG_SHUNT_VOLTAGE);
    return val * 0.01; // LSB is 10uV, so result is in mV
}

float ina219_get_current_ma() {
    int16_t val = (int16_t)read_register(INA219_REG_CURRENT);
    return val * _current_lsb * 1000;
}

float ina219_get_power_mw() {
    uint16_t val = read_register(INA219_REG_POWER);
    return val * _current_lsb * 20 * 1000;
}