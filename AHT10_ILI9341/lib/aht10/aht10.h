#ifndef AHT10_H
#define AHT10_H

#include "pico/stdlib.h"

// Estrutura para guardar os dados lidos
typedef struct {
    float temperature;
    float humidity;
} aht10_reading;

// Funções públicas da biblioteca
void aht10_init(void);
void aht10_deinit(void);
bool aht10_read_data(aht10_reading *result);

#endif