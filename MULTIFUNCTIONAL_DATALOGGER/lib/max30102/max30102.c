#include "max30102.h"
#include <stdio.h>

static i2c_inst_t *i2c_instance;

// Funções auxiliares (permanecem as mesmas)
static void max30102_write_register(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    i2c_write_blocking(i2c_instance, MAX30102_ADDR, data, 2, false);
}

uint8_t max30102_read_register(uint8_t reg) {
    uint8_t value;
    i2c_write_blocking(i2c_instance, MAX30102_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c_instance, MAX30102_ADDR, &value, 1, false);
    return value;
}

static void max30102_read_registers(uint8_t reg, uint8_t *buffer, uint16_t len) {
    i2c_write_blocking(i2c_instance, MAX30102_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c_instance, MAX30102_ADDR, buffer, len, false);
}


void max30102_reset() {
    max30102_write_register(0x09, 0x40); // REG_MODE_CONFIG
}

bool max30102_init(i2c_inst_t *i2c_port, uint sda_pin, uint scl_pin) {
    i2c_instance = i2c_port;

    i2c_init(i2c_instance, 400 * 1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    
    printf("Inicializando MAX30102...\n");

    // Diagnóstico para o MAX30102
    uint8_t part_id = max30102_read_register(0xFF);
    printf("  [DIAGNÓSTICO] Part ID: 0x%02x (esperado: 0x15)\n", part_id);
    if (part_id != 0x15) {
        printf("  [ERRO] Dispositivo não é um MAX30102.\n");
        return false;
    }
    
    max30102_reset();
    sleep_ms(200);

    // Limpa o registrador de interrupção
    max30102_read_register(0x00);

    // Zerar ponteiros do FIFO
    max30102_write_register(0x04, 0x00); // FIFO_WR_PTR
    max30102_write_register(0x05, 0x00); // OVF_COUNTER
    max30102_write_register(0x06, 0x00); // FIFO_RD_PTR

    // Configuração do FIFO
    // sample averaging=4, fifo rollover=false, fifo almost full=17
    max30102_write_register(0x08, 0x4F);
    
    // Configuração do modo
    // 0x03 = SpO2 mode
    max30102_write_register(0x09, 0x03);

    // Configuração SpO2
    // ADC Range=4096, Sample Rate=100Hz, Pulse Width=411us
    max30102_write_register(0x0A, 0x27);

    // Configuração da corrente dos LEDs
    max30102_write_register(0x0C, 0x24); // LED1 (Red) ~7.6mA
    max30102_write_register(0x0D, 0x24); // LED2 (IR)  ~7.6mA
    
    printf("MAX30102 inicializado com sucesso.\n");
    return true;
}

// Leitura do FIFO para o MAX30102 (6 bytes por amostra)
void max30102_read_fifo(max30102_data_t *data) {
    uint8_t buffer[6];
    max30102_read_registers(0x07, buffer, 6);

    // Os dados são de 18 bits, mas usamos os 16 bits mais significativos
    data->red = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    data->ir = (buffer[3] << 16) | (buffer[4] << 8) | buffer[5];
    
    // Descartar os 2 bits menos significativos para simplificar
    data->red >>= 2;
    data->ir >>= 2;
}