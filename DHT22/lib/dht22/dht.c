// lib/dht22/dht.c
#include "dht.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/time.h"
#include <stdio.h>

static uint dht_pin;
static absolute_time_t last_read_time;

void dht_init(uint pin) {
    dht_pin = pin;
    last_read_time = get_absolute_time() - 2000 * 1000;
    gpio_init(dht_pin);
}

bool dht_read(dht_reading *result) {
    printf("  [DHT] Entrando em dht_read()...\n");

    if (absolute_time_diff_us(last_read_time, get_absolute_time()) < 2000 * 1000) {
        printf("  [DHT] Leitura muito recente, aguardando.\n");
        return false;
    }
    last_read_time = get_absolute_time();

    uint8_t data[5] = {0, 0, 0, 0, 0};
    
    // 1. Enviar sinal de início
    gpio_set_dir(dht_pin, GPIO_OUT);
    gpio_put(dht_pin, 0);
    sleep_ms(18);
    printf("  [DHT] Sinal de inicio enviado.\n");

    // 2. Esperar pela resposta do sensor
    gpio_set_dir(dht_pin, GPIO_IN);
    gpio_disable_pulls(dht_pin);
    sleep_us(40); // Pequena espera para o pino estabilizar

    if (gpio_get(dht_pin)) {
        printf("  [DHT] ERRO: Sensor nao respondeu ao sinal de inicio (nao puxou pino para baixo).\n");
        return false;
    }
    sleep_us(80);
    if (!gpio_get(dht_pin)) {
        printf("  [DHT] ERRO: Sensor nao finalizou a resposta (nao puxou pino para cima).\n");
        return false;
    }
    printf("  [DHT] Resposta do sensor detectada.\n");

    // 3. Ler os 40 bits de dados
    printf("  [DHT] Lendo os 40 bits de dados...\n");
    for (int i = 0; i < 40; i++) {
        // Espera o pulso de 50us que precede cada bit
        uint32_t timeout = 0;
        while(!gpio_get(dht_pin)) {
            if(timeout++ > 1000) { printf("  [DHT] TIMEOUT bit %d low\n", i); return false; }
            sleep_us(1);
        }

        // Mede a duração do pulso de alta
        timeout = 0;
        while(gpio_get(dht_pin)) {
            if(timeout++ > 1000) { printf("  [DHT] TIMEOUT bit %d high\n", i); return false; }
            sleep_us(1);
        }

        // Se o pulso de alta foi maior que ~40us, é um '1'
        if (timeout > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }
    printf("  [DHT] Leitura dos bits finalizada.\n");
    
    // 4. Verificar o checksum
    if ((data[0] + data[1] + data[2] + data[3]) & 0xFF != data[4]) {
        printf("  [DHT] ERRO de Checksum!\n");
        return false;
    }
    printf("  [DHT] Checksum OK.\n");

    // 5. Calcular valores
    result->humidity = (float)((data[0] << 8) + data[1]) / 10.0;
    result->temp_celsius = (float)(((data[2] & 0x7F) << 8) + data[3]) / 10.0;
    if (data[2] & 0x80) {
        result->temp_celsius *= -1;
    }
    
    return true;
}