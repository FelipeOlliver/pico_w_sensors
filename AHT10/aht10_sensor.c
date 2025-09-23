#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// --- SUAS CONFIGURAÇÕES DE PINO ---
#define I2C_PORT i2c1 // Usaremos o barramento I2C1
const uint I2C_SDA_PIN = 2;
const uint I2C_SCL_PIN = 3;
// ------------------------------------

// Endereço I2C padrão do sensor AHT10
const uint8_t AHT10_ADDR = 0x38;

// Comandos do AHT10 (conforme datasheet)
const uint8_t AHT10_CMD_INIT[] = {0xE1, 0x08, 0x00};
const uint8_t AHT10_CMD_TRIGGER[] = {0xAC, 0x33, 0x00};

// Função para inicializar o sensor
void aht10_init() {
    i2c_write_blocking(I2C_PORT, AHT10_ADDR, AHT10_CMD_INIT, sizeof(AHT10_CMD_INIT), false);
    sleep_ms(20);
}

// Função para ler os dados brutos do sensor
void aht10_read_raw(uint8_t* buffer) {
    // 1. Envia o comando para iniciar a medição
    i2c_write_blocking(I2C_PORT, AHT10_ADDR, AHT10_CMD_TRIGGER, sizeof(AHT10_CMD_TRIGGER), false);
    
    // 2. Aguarda o tempo de conversão (>75ms)
    sleep_ms(80);

    // 3. Lê os 6 bytes de dados
    i2c_read_blocking(I2C_PORT, AHT10_ADDR, buffer, 6, false);
}

int main() {
    // Inicializa a saída serial (USB)
    stdio_init_all();
    sleep_ms(2000);

    printf("Iniciando leitor do sensor AHT10 nos pinos GP2/GP3 (i2c1)...\n");

    // Inicializa o barramento I2C1 com uma frequência de 100kHz
    i2c_init(I2C_PORT, 100 * 1000);
    
    // Configura os pinos GP2 (SDA) e GP3 (SCL) para a função I2C
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    
    // Habilita os resistores de pull-up internos do Pico
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializa o sensor AHT10
    aht10_init();

    uint8_t read_buffer[6];
    
    while (1) {
        aht10_read_raw(read_buffer);

        if ((read_buffer[0] & 0x80) == 0) { // Se o sensor não estiver ocupado
            // --- CÁLCULO DA UMIDADE ---
            uint32_t raw_humidity = ((uint32_t)read_buffer[1] << 12) | ((uint32_t)read_buffer[2] << 4) | (read_buffer[3] >> 4);
            float humidity = ((float)raw_humidity / 1048576.0) * 100.0;

            // --- CÁLCULO DA TEMPERATURA ---
            uint32_t raw_temp = (((uint32_t)read_buffer[3] & 0x0F) << 16) | ((uint32_t)read_buffer[4] << 8) | read_buffer[5];
            float temperature = ((float)raw_temp / 1048576.0) * 200.0 - 50.0;
            
            printf("Umidade: %.2f %%RH, Temperatura: %.2f C\n", humidity, temperature);

        } else {
            printf("Sensor ocupado, tentando novamente...\n");
        }

        sleep_ms(2000);
    }

    return 0;
}