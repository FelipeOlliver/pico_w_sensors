#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// --- Constantes do Sensor BH1750 ---
const uint8_t BH1750_ADDR = 0x23;
const uint8_t BH1750_POWER_ON = 0x01;
const uint8_t BH1750_CONT_H_RES_MODE = 0x10;
const float LUX_CONVERSION_FACTOR = 1.2;

// --- Configurações do I2C do Pico ---
#define I2C_PORT i2c1
const uint I2C_SDA_PIN = 2;
const uint I2C_SCL_PIN = 3;

// Variável global para armazenar o valor do MTreg ---
uint8_t current_mtreg = 69; 

// Função para enviar um byte de comando
void bh1750_send_command(uint8_t command) {
    i2c_write_blocking(I2C_PORT, BH1750_ADDR, &command, 1, false);
}

// O valor de mtreg deve estar entre 31 e 254
bool bh1750_set_sensitivity(uint8_t mtreg) {
    if (mtreg < 31 || mtreg > 254) {
        printf("Erro: Valor de MTreg fora da faixa (31-254)\n");
        return false;
    }

    // O datasheet especifica dois comandos para alterar o MTreg
    // 1. Comando para os 3 bits mais significativos (High bit)
    uint8_t high_bits_cmd = 0b01000000 | (mtreg >> 5);
    // 2. Comando para os 5 bits menos significativos (Low bit)
    uint8_t low_bits_cmd = 0b01100000 | (mtreg & 0b00011111);

    bh1750_send_command(high_bits_cmd);
    bh1750_send_command(low_bits_cmd);

    // Armazena o novo valor para ser usado no cálculo do lux
    current_mtreg = mtreg;
    
    // O tempo de medição muda com a sensibilidade.
    // Tempo ~= 120ms * (novo_mtreg / 69)
    sleep_ms(120 * (float)current_mtreg / 69.0f);

    return true;
}

// Função de inicialização
void bh1750_init(uint8_t mtreg) {
    bh1750_send_command(BH1750_POWER_ON);
    sleep_ms(10);

    // Define a sensibilidade desejada
    bh1750_set_sensitivity(mtreg);

    // Configura o modo de medição contínua
    bh1750_send_command(BH1750_CONT_H_RES_MODE);
    sleep_ms(120 * (float)current_mtreg / 69.0f);
}

// Função de leitura
float bh1750_read_lux() {
    uint8_t buffer[2];
    int bytes_read = i2c_read_blocking(I2C_PORT, BH1750_ADDR, buffer, 2, false);
    
    if (bytes_read < 2) {
        printf("Erro ao ler do sensor I2C.\n");
        return -1.0;
    }
    
    uint16_t raw_value = (buffer[0] << 8) | buffer[1];
    
    // Fórmula de conversão ajustada pela sensibilidade 
    float lux = (raw_value / LUX_CONVERSION_FACTOR) * (69.0f / current_mtreg);
    
    return lux;
}

int main() {
    stdio_init_all();
    sleep_ms(2000); 
    
    uint8_t mtreg_value = 31; // Sensibilidade 
    
    printf("Iniciando leitor BH1750 com MTreg = %d\n", mtreg_value);

    i2c_init(I2C_PORT, 100 * 1000); 
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Inicializa o sensor com o valor de MTreg escolhido
    bh1750_init(mtreg_value);

    while (true) {
        float lux = bh1750_read_lux();
        
        if (lux >= 0) {

            printf("Luminosidade: %d lx\n", (int)lux);
        }
        
        sleep_ms(5000);
    }

    return 0;
}