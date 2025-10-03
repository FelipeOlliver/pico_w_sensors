// dht.h
#ifndef DHT_H
#define DHT_H

#include "pico/stdlib.h"

typedef struct {
    float humidity;
    float temp_celsius;
} dht_reading;

void dht_init(uint pin);
bool dht_read(dht_reading *result);

#endif