#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "rfm95_lora.h"

// =============================================================================
// --- DEFINIÇÕES E FUNÇÕES DO SENSOR VL53L0X ---
// =============================================================================

// Definições de I2C para o sensor
#define I2C_PORT_SENSOR i2c0
#define SDA_PIN 0
#define SCL_PIN 1
#define VL53L0X_ADDR 0x29

// Lista de endereços de registradores do VL53L0X
#define REG_IDENTIFICATION_MODEL_ID   0xC0
#define REG_SYSRANGE_START            0x00
#define REG_RESULT_RANGE_STATUS       0x14
#define REG_RESULT_RANGE_MM           0x1E
#define DISTANCE_OFFSET_MM            100 

/* Função de inicialização e configuração do I2C para o sensor */
void config_i2c_sensor() {
    i2c_init(I2C_PORT_SENSOR, 100 * 1000); // Comunicação I2C a 100 kHz
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}

/* Inicialização do sensor VL53L0X */
bool vl53l0x_init() {
    uint8_t reg = REG_IDENTIFICATION_MODEL_ID;
    uint8_t id;

    // Lê ID do sensor
    if (i2c_write_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, &reg, 1, true) != 1) return false;
    if (i2c_read_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, &id, 1, false) != 1) return false;
    
    if (id != 0xEE) { // Confirma se é o VL53L0X
        printf("ID do sensor inválido: 0x%02X (esperado: 0xEE)\n", id);
        return false;
    }
    return true;
}

/* Leitura da distância do sensor VL53L0X em milímetros */
int vl53l0x_read_distance_mm() {
    // Inicia medição única
    uint8_t cmd[2] = {REG_SYSRANGE_START, 0x01};
    if (i2c_write_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, cmd, 2, false) != 2) return -1;

    // Aguarda resultado (~50 ms máx)
    uint8_t status = 0;
    for (int i = 0; i < 100; i++) {
        uint8_t reg_status = REG_RESULT_RANGE_STATUS;
        if (i2c_write_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, &reg_status, 1, true) != 1) return -1;
        if (i2c_read_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, &status, 1, false) != 1) return -1;
        if (status & 0x01) break; // Medição pronta
        sleep_ms(5);
    }

    // Lê os 2 bytes da distância
    uint8_t reg_mm = REG_RESULT_RANGE_MM;
    uint8_t buffer[2];
    if (i2c_write_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, &reg_mm, 1, true) != 1) return -1;
    if (i2c_read_blocking(I2C_PORT_SENSOR, VL53L0X_ADDR, buffer, 2, false) != 2) return -1;

    return (buffer[0] << 8) | buffer[1];
}


// =============================================================================
// --- FUNÇÃO PRINCIPAL ---
// =============================================================================
int main() {
    stdio_init_all();
    sleep_ms(4000); // Tempo para o monitor serial conectar

    // --- Inicialização do Sensor de Distância ---
    printf("Inicializando I2C para o sensor VL53L0X...\n");
    config_i2c_sensor();
    if (!vl53l0x_init()) {
        printf("Falha ao inicializar o VL53L0X. Travando. ❌\n");
        while(1);
    }
    printf("Sensor VL53L0X OK! ✅\n\n");

    // --- Inicialização do Módulo LoRa ---
    printf("Inicializando Transmissor LoRa (TX)...\n");
    if (!lora_init()) {
        printf("Falha na comunicacao com o RFM95. Travando. ❌\n");
        while(1);
    }
    printf("Comunicacao com RFM95 OK! ✅\n\n");
    
    // Configura a potência do LoRa para o máximo
    lora_set_power(17);

    // --- Loop Principal ---
    while (1) {
        // 1. Faz a leitura "crua" do sensor em milímetros
        int distancia_raw = vl53l0x_read_distance_mm();
        int distancia = 0; // Variável para a distância final corrigida em mm

        // 2. Aplica a correção de offset
        if (distancia_raw >= 0) {
            distancia = distancia_raw - DISTANCE_OFFSET_MM;
            if (distancia < 0) { // Garante que a distância não seja negativa
                distancia = 0;
            }
        } else {
            distancia = -1; // Mantém o valor de erro se a leitura falhar
        }

        // 3. Processa e exibe o valor corrigido
        if (distancia < 0) {
            printf("Erro na leitura da distância.\n");
        } else {
            // Converte a distância corrigida (mm) para cm para exibição
            float distancia_cm = distancia / 10.0f;
            
            // Imprime a distância em cm com uma casa decimal
            printf("Distância: %.1f cm\n", distancia_cm);

            // 4. Verifica a condição de alerta com o valor corrigido em mm
            if (distancia < 100) { // Menor que 10 cm (100 mm)
                char message_buffer[50];
                // Monta a mensagem de alerta usando o valor em cm
                snprintf(message_buffer, sizeof(message_buffer), "ALERTA! Objeto a %.1f cm", distancia_cm);
                
                printf("--> Condição de alerta atingida! Enviando via LoRa: '%s'\n", message_buffer);
                lora_send_packet((uint8_t*)message_buffer, strlen(message_buffer));
            }
        }
        
        // Aguarda 2 segundos para a próxima leitura
        sleep_ms(2000);
    }

    return 0;
}