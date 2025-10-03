// lib/tcs34725/tcs34725.c

#include "tcs34725.h"

// Endereço I2C e registradores do sensor
const uint8_t SENSOR_ADDR = 0x29;
const uint8_t REG_ENABLE  = 0x00;
const uint8_t REG_ATIME   = 0x01;
const uint8_t REG_CONTROL = 0x0F;
const uint8_t REG_ID      = 0x12;
const uint8_t REG_CDATA   = 0x14;
const uint8_t REG_RDATA   = 0x16;
const uint8_t REG_GDATA   = 0x18;
const uint8_t REG_BDATA   = 0x1A;

static i2c_inst_t *i2c_instance;

// Função interna para escrever em um registrador
static void tcs_write_reg(uint8_t reg, uint8_t value) {
    uint8_t data[] = {reg | 0x80, value};
    i2c_write_blocking(i2c_instance, SENSOR_ADDR, data, 2, false);
}

// Função interna para ler dados de um registrador
static void tcs_read_data(uint8_t reg, uint8_t* buffer, uint8_t len) {
    uint8_t command_reg = reg | 0x80;
    i2c_write_blocking(i2c_instance, SENSOR_ADDR, &command_reg, 1, true);
    i2c_read_blocking(i2c_instance, SENSOR_ADDR, buffer, len, false);
}

// Inicializa a comunicação I2C e configura o sensor
void tcs_init(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin) {
    i2c_instance = i2c;
    i2c_init(i2c_instance, 100 * 1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

// Verifica se o sensor está conectado e respondendo
bool tcs_is_connected() {
    uint8_t device_id;
    tcs_read_data(REG_ID, &device_id, 1);
    
    // O ID pode ser 0x44 (TCS34725) ou 0x4D (TCS34727)
    if (device_id != 0x44 && device_id != 0x4D) { 
        return false;
    }

    tcs_write_reg(REG_ATIME, 0xC0);   // Tempo de integração
    tcs_write_reg(REG_CONTROL, 0x00); // Ganho 1x
    tcs_write_reg(REG_ENABLE, 0x03);  // Liga o oscilador e o sensor (PON & AEN)
    sleep_ms(3);
    return true;
}

// Lê os 4 canais de cor (Red, Green, Blue, Clear)
void tcs_read_rgbc(tcs_rgbc_t *data) {
    uint8_t r_buf[2], g_buf[2], b_buf[2], c_buf[2];
    tcs_read_data(REG_RDATA, r_buf, 2);
    tcs_read_data(REG_GDATA, g_buf, 2);
    tcs_read_data(REG_BDATA, b_buf, 2);
    tcs_read_data(REG_CDATA, c_buf, 2);

    data->red   = (r_buf[1] << 8) | r_buf[0];
    data->green = (g_buf[1] << 8) | g_buf[0];
    data->blue  = (b_buf[1] << 8) | b_buf[0];
    data->clear = (c_buf[1] << 8) | c_buf[0];
}