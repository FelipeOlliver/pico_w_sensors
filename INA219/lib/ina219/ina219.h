/*
 * ina219.h
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
 * The above copyright notice and this permission notice shall be included in all
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

#ifndef INA219_H_
#define INA219_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef PICO_SDK_VERSION_STRING
#include "hardware/i2c.h"
#define INA219_I2C_INSTANCE i2c_inst_t*
#else
// Define a generic type if not using Pico SDK for broader compatibility
#define INA219_I2C_INSTANCE void*
#endif


/**
 * @brief INA219 register addresses
 */
#define INA219_REG_CONFIG 0x00
#define INA219_REG_SHUNT_VOLTAGE 0x01
#define INA219_REG_BUS_VOLTAGE 0x02
#define INA219_REG_POWER 0x03
#define INA219_REG_CURRENT 0x04
#define INA219_REG_CALIBRATION 0x05

/**
 * @brief INA219 configuration constants
 */
typedef enum {
    INA219_RANGE_16V = 0, /* 0-16V */
    INA219_RANGE_32V = 1  /* 0-32V */
} ina219_range_t;

typedef enum {
    INA219_GAIN_40MV = 0,  /* Gain 1, Range 0-40mV */
    INA219_GAIN_80MV = 1,  /* Gain 2, Range 0-80mV */
    INA219_GAIN_160MV = 2, /* Gain 4, Range 0-160mV */
    INA219_GAIN_320MV = 3  /* Gain 8, Range 0-320mV */
} ina219_gain_t;

typedef enum {
    INA219_BUS_ADC_9BIT = 0,   /* 9-bit bus ADC */
    INA219_BUS_ADC_10BIT = 1,  /* 10-bit bus ADC */
    INA219_BUS_ADC_11BIT = 2,  /* 11-bit bus ADC */
    INA219_BUS_ADC_12BIT = 3,  /* 12-bit bus ADC */
} ina219_bus_adc_t;

typedef enum {
    INA219_SHUNT_ADC_9BIT = 0,    /* 9-bit shunt ADC */
    INA219_SHUNT_ADC_10BIT = 1,   /* 10-bit shunt ADC */
    INA219_SHUNT_ADC_11BIT = 2,   /* 11-bit shunt ADC */
    INA219_SHUNT_ADC_12BIT = 3,   /* 12-bit shunt ADC */
    INA219_SHUNT_ADC_2S = 9,      /* 2 samples at 12-bit */
    INA219_SHUNT_ADC_4S = 10,     /* 4 samples at 12-bit */
    INA219_SHUNT_ADC_8S = 11,     /* 8 samples at 12-bit */
    INA219_SHUNT_ADC_16S = 12,    /* 16 samples at 12-bit */
    INA219_SHUNT_ADC_32S = 13,    /* 32 samples at 12-bit */
    INA219_SHUNT_ADC_64S = 14,    /* 64 samples at 12-bit */
    INA219_SHUNT_ADC_128S = 15,   /* 128 samples at 12-bit */
} ina219_shunt_adc_t;

typedef enum {
    INA219_MODE_POWER_DOWN = 0,
    INA219_MODE_SHUNT_TRIG = 1,
    INA219_MODE_BUS_TRIG = 2,
    INA219_MODE_SHUNT_BUS_TRIG = 3,
    INA219_MODE_ADC_OFF = 4,
    INA219_MODE_SHUNT_CONTINUOUS = 5,
    INA219_MODE_BUS_CONTINUOUS = 6,
    INA219_MODE_SHUNT_BUS_CONTINUOUS = 7,
} ina219_mode_t;


/**
 * @brief Initialize the INA219
 * @param i2c The I2C instance to use. For Pico, this is i2c0 or i2c1.
 * @param addr The I2C address of the INA219.
 * @return True if initialization was successful, false otherwise.
 */
bool ina219_init(INA219_I2C_INSTANCE i2c, uint8_t addr);

/**
 * @brief Configure the INA219
 * @param range The bus voltage range.
 * @param gain The shunt voltage gain.
 * @param bus_adc The bus ADC resolution/averaging.
 * @param shunt_adc The shunt ADC resolution/averaging.
 * @param mode The operating mode.
 */
void ina219_configure(ina219_range_t range, ina219_gain_t gain,
                      ina219_bus_adc_t bus_adc, ina219_shunt_adc_t shunt_adc,
                      ina219_mode_t mode);

/**
 * @brief Calibrate the INA219
 * @param current_lsb The LSB of the current register.
 * @param cal The calibration value.
 */
void ina219_calibrate(float current_lsb, uint16_t cal);

/**
 * @brief Reset the INA219
 */
void ina219_reset();

/**
 * @brief Get the bus voltage in volts
 * @return The bus voltage in volts.
 */
float ina219_get_bus_voltage();

/**
 * @brief Get the shunt voltage in millivolts
 * @return The shunt voltage in millivolts.
 */
float ina219_get_shunt_voltage_mv();

/**
 * @brief Get the current in milliamps
 * @return The current in milliamps.
 */
float ina219_get_current_ma();

/**
 * @brief Get the power in milliwatts
 * @return The power in milliwatts.
 */
float ina219_get_power_mw();


#ifdef __cplusplus
}
#endif

#endif /* INA219_H_ */