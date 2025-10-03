#ifndef MAX30102_H
#define MAX30102_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define MAX30102_ADDR 0x57

// Limiar para detectar a presença do dedo (ajuste conforme seu sensor e dedo)
// Valores abaixo disso = sem dedo, acima = com dedo.
// Você pode ajustar este valor (e.g., para 5000, 10000, etc.)
#define FINGER_ON_THRESHOLD 5000 

typedef struct {
    uint32_t red;
    uint32_t ir;
} max30102_data_t;

bool max30102_init(i2c_inst_t *i2c_port, uint sda_pin, uint scl_pin);
void max30102_read_fifo(max30102_data_t *data);
void max30102_reset();
uint8_t max30102_read_register(uint8_t reg);

#endif