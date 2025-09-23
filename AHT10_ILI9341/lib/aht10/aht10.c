#include "aht10.h"
#include "hardware/i2c.h"
#include <stdio.h>

// --- CONFIGURAÇÕES I2C ---
#define I2C_PORT i2c1
const uint I2C_SDA_PIN = 2;
const uint I2C_SCL_PIN = 3;
// -------------------------

const uint8_t AHT10_ADDR = 0x38;
const uint8_t AHT10_CMD_INIT[] = {0xE1, 0x08, 0x00};
const uint8_t AHT10_CMD_TRIGGER[] = {0xAC, 0x33, 0x00};

void aht10_init(void) {
    // Inicializa o I2C (como antes)
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Define um timeout de 10ms (10000µs) por byte
    uint timeout_us = 10000;

    // A chamada original para comparar:
    // i2c_write_blocking(I2C_PORT, AHT10_ADDR, AHT10_CMD_INIT, sizeof(AHT10_CMD_INIT), false);

    // NOVA CHAMADA com timeout e verificação de erro:
    int bytes_escritos = i2c_write_timeout_per_char_us(I2C_PORT, AHT10_ADDR, AHT10_CMD_INIT, sizeof(AHT10_CMD_INIT), false, timeout_us);

    // VERIFICAÇÃO DO RESULTADO
    if (bytes_escritos < 0) {
        // Se a função retornou um erro (ex: PICO_ERROR_TIMEOUT)
        printf("ERRO: Falha ao enviar comando de init para o AHT10. Codigo: %d\n", bytes_escritos);
    } else if (bytes_escritos < sizeof(AHT10_CMD_INIT)) {
        // Se a função não conseguiu escrever todos os bytes
        printf("AVISO: Nem todos os bytes de init foram escritos. Escritos: %d de %d\n", bytes_escritos, sizeof(AHT10_CMD_INIT));
    } else {
        // Sucesso
        printf("Comando de init para o AHT10 enviado com sucesso.\n");
    }
    
    sleep_ms(20);
}

void aht10_deinit(void) {
    // 1. Desliga o periférico I2C
    i2c_deinit(I2C_PORT);

    // 2. A maneira mais FORTE de resetar um pino para o estado padrão.
    //    Isso desativa os pull-ups e desconecta o pino de qualquer periférico.
    gpio_init(I2C_SDA_PIN);
    gpio_init(I2C_SCL_PIN);
}

bool aht10_read_data(aht10_reading *result) {
    uint8_t read_buffer[6];
    uint timeout_us = 10000; // 10ms

    // 1. Envia comando para iniciar a medição com timeout
    int bytes_escritos = i2c_write_timeout_per_char_us(I2C_PORT, AHT10_ADDR, AHT10_CMD_TRIGGER, sizeof(AHT10_CMD_TRIGGER), false, timeout_us);

    if (bytes_escritos < 0) {
        printf("ERRO: Falha ao enviar comando de trigger para o AHT10.\n");
        return false; // Falha na comunicação
    }

    sleep_ms(80); // Aguarda a conversão

    // (A função de leitura também pode ser trocada por uma com timeout, se necessário)
    int bytes_lidos = i2c_read_blocking(I2C_PORT, AHT10_ADDR, read_buffer, 6, false);

    if (bytes_lidos < 0) {
        printf("ERRO: Falha ao ler dados do AHT10.\n");
        return false; // Falha na comunicação
    }
    
    // O resto da lógica continua igual...
    if ((read_buffer[0] & 0x80) != 0) {
        return false; 
    }
    uint32_t raw_humidity = ((uint32_t)read_buffer[1] << 12) | ((uint32_t)read_buffer[2] << 4) | (read_buffer[3] >> 4);
    result->humidity = ((float)raw_humidity / 1048576.0) * 100.0;
    uint32_t raw_temp = (((uint32_t)read_buffer[3] & 0x0F) << 16) | ((uint32_t)read_buffer[4] << 8) | read_buffer[5];
    result->temperature = ((float)raw_temp / 1048576.0) * 200.0 - 50.0;

    return true; // Sucesso
}