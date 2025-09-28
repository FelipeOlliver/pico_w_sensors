#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "ff.h"
#include "diskio.h"

// --- Definições do SD Card (SPI) ---
#define SPI_PORT_SD     spi0
#define PIN_MISO_SD     16
#define PIN_MOSI_SD     19
#define PIN_SCK_SD      18
#define PIN_CS_SD       17

// --- Definições do Sensor VL53L0X (I2C) ---
#define I2C_PORT_SENSOR i2c0
#define SDA_PIN         0
#define SCL_PIN         1
#define VL53L0X_ADDR    0x29

// Registradores do VL53L0X
#define REG_IDENTIFICATION_MODEL_ID   0xC0
#define REG_SYSRANGE_START            0x00
#define REG_RESULT_RANGE_STATUS       0x14
#define REG_RESULT_RANGE_MM           0x1E

// Constante de calibração do sensor de distância (ajuste se necessário)
#define DISTANCE_OFFSET_MM 100 

// Variáveis globais para o sistema de arquivos
FATFS fs;
FIL file;

void config_i2c_sensor() {
    i2c_init(I2C_PORT_SENSOR, 100 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}

bool vl53l0x_init() {
    uint8_t reg = REG_IDENTIFICATION_MODEL_ID;
    uint8_t id;
    if (i2c_write_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, &reg, 1, true) != 1) return false;
    if (i2c_read_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, &id, 1, false) != 1) return false;
    if (id != 0xEE) {
        printf("ID do sensor inválido: 0x%02X (esperado: 0xEE)\n", id);
        return false;
    }
    return true;
}

int vl53l0x_read_distance_mm() {
    uint8_t cmd[2] = {REG_SYSRANGE_START, 0x01};
    if (i2c_write_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, cmd, 2, false) != 2) return -1;

    uint8_t status = 0;
    for (int i = 0; i < 100; i++) {
        uint8_t reg_status = REG_RESULT_RANGE_STATUS;
        if (i2c_write_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, &reg_status, 1, true) != 1) return -1;
        if (i2c_read_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, &status, 1, false) != 1) return -1;
        if (status & 0x01) break;
        sleep_ms(5);
    }

    uint8_t reg_mm = REG_RESULT_RANGE_MM;
    uint8_t buffer[2];
    if (i2c_write_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, &reg_mm, 1, true) != 1) return -1;
    if (i2c_read_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, buffer, 2, false) != 2) return -1;

    return (buffer[0] << 8) | buffer[1];
}

void inicializar_sd() {
    spi_init(SPI_PORT_SD, 1000 * 1000); // 1 MHz para inicialização
    gpio_set_function(PIN_MISO_SD, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI_SD, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK_SD, GPIO_FUNC_SPI);
    gpio_init(PIN_CS_SD);
    gpio_set_dir(PIN_CS_SD, GPIO_OUT);
    gpio_put(PIN_CS_SD, 1);

    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("Erro ao montar SD Card. Código: %d\n", fr);
        return; // Sai da função se falhar
    }
    printf("Cartão SD montado com sucesso.\n");

    fr = f_open(&file, "alertas_distancia.txt", FA_WRITE | FA_OPEN_APPEND);
    if (fr != FR_OK) {
        printf("Erro ao abrir/criar arquivo de log. Código: %d\n", fr);
        return; // Sai da função se falhar
    }
    
    // Escreve o cabeçalho apenas se o arquivo for novo (tamanho 0)
    if (f_size(&file) == 0) {
        f_puts("Alerta,Distancia,Timestamp_ms\n", &file);
        f_sync(&file); // Garante que o cabeçalho seja escrito
    }
    printf("Arquivo de log 'alertas_distancia.txt' pronto.\n");
}

void registrar_evento(const char* alerta_msg, float dist_cm) {
    char linha[100];
    // Formata a linha do log com o alerta, a distância e o timestamp
    snprintf(linha, sizeof(linha), "%s,%.1f cm,%lu\n", alerta_msg, dist_cm, to_ms_since_boot(get_absolute_time()));
    
    UINT bytes_escritos;
    FRESULT fr = f_write(&file, linha, strlen(linha), &bytes_escritos);
    
    if (fr != FR_OK) {
        printf("Erro ao escrever no arquivo: %d\n", fr);
    } else {
        f_sync(&file); // Força a escrita dos dados no cartão
        printf("--> ALERTA REGISTRADO NO SD: %s", linha);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(4000); // Aguarda a conexão do monitor serial

    // --- Inicializa os periféricos ---
    printf("Inicializando sensor VL53L0X...\n");
    config_i2c_sensor();
    if (!vl53l0x_init()) {
        printf("Falha ao inicializar o VL53L0X. Travando.\n");
        while(1);
    }
    printf("Sensor OK!\n\n");

    printf("Inicializando Cartão SD...\n");
    inicializar_sd();
    printf("Setup completo. Iniciando monitoramento.\n\n");

    // --- Loop Principal ---
    while (true) {
        int distancia_raw = vl53l0x_read_distance_mm();
        int distancia_corrigida = -1;

        if (distancia_raw >= 0) {
            distancia_corrigida = distancia_raw - DISTANCE_OFFSET_MM;
            if (distancia_corrigida < 0) distancia_corrigida = 0;
        }

        if (distancia_corrigida < 0) {
            printf("Erro na leitura da distância.\n");
        } else {
            float distancia_cm = distancia_corrigida / 10.0f;
            printf("Distância: %.1f cm\n", distancia_cm);

            // Se a distância for menor que 10 cm, registra o evento
            if (distancia_corrigida < 100) {
                registrar_evento("Objeto proximo detectado", distancia_cm);
            }
        }
        
        sleep_ms(2000); // Espera 2 segundos para a próxima leitura
    }

    return 0;
}