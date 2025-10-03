// lib/tcs34725/tcs34725.h

#ifndef TCS34725_H
#define TCS34725_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Estrutura para armazenar a leitura do sensor
typedef struct {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t clear;
} tcs_rgbc_t;

// Protótipos das funções
void tcs_init(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin);
bool tcs_is_connected();
void tcs_read_rgbc(tcs_rgbc_t *data);

#endif