#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"

// --- CONFIGURAÇÃO DOS PINOS ---
#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12
#define BUTTON_PIN 5      // CORREÇÃO: Pino correto do botão
#define I2C_SDA_PIN 2
#define I2C_SCL_PIN 3

// --- CÓDIGO DO SENSOR DE COR (TCS34727) ---
const uint8_t SENSOR_ADDR = 0x29;
const uint8_t REG_ENABLE = 0x00;
const uint8_t REG_ATIME = 0x01;
const uint8_t REG_CONTROL = 0x0F;
const uint8_t REG_ID = 0x12;
const uint8_t REG_CDATA = 0x14;
const uint8_t REG_RDATA = 0x16;
const uint8_t REG_GDATA = 0x18;
const uint8_t REG_BDATA = 0x1A;

void tcs_write_reg(uint8_t reg, uint8_t value) {
    uint8_t data[] = {reg | 0x80, value};
    i2c_write_blocking(i2c1, SENSOR_ADDR, data, 2, false);
}

void tcs_read_data(uint8_t reg, uint8_t* buffer, uint8_t len) {
    uint8_t command_reg = reg | 0x80;
    i2c_write_blocking(i2c1, SENSOR_ADDR, &command_reg, 1, true);
    i2c_read_blocking(i2c1, SENSOR_ADDR, buffer, len, false);
}

bool tcs_init() {
    uint8_t device_id;
    tcs_read_data(REG_ID, &device_id, 1);
    
    // CORREÇÃO: Verificando pelo ID 0x4D (TCS34727)
    if (device_id != 0x4D) { 
        printf("ERRO: Sensor de cor nao encontrado. ID lido: 0x%02X\n", device_id);
        return false;
    }

    tcs_write_reg(REG_ATIME, 0xC0);
    tcs_write_reg(REG_CONTROL, 0x00);
    tcs_write_reg(REG_ENABLE, 0x03);
    sleep_ms(3);
    return true;
}

// --- CÓDIGO DO LED E BOTÃO ---
void set_led_color_pwm(int state) {
    uint16_t r_val = 0, g_val = 0, b_val = 0;
    switch (state) {
        case 0: r_val = 65535; g_val = 65535; b_val = 65535; break; // Branco
        case 1: r_val = 65535; g_val = 0;     b_val = 0;     break; // Vermelho
        case 2: r_val = 0;     g_val = 0;     b_val = 65535; break; // Azul
        case 3: r_val = 0;     g_val = 65535; b_val = 0;     break; // Verde
    }
    pwm_set_gpio_level(LED_R_PIN, r_val);
    pwm_set_gpio_level(LED_G_PIN, g_val);
    pwm_set_gpio_level(LED_B_PIN, b_val);
}

// --- FUNÇÃO PRINCIPAL ---
int main() {
    stdio_init_all();
    sleep_ms(5000);

    // --- Inicializa LED RGB com PWM ---
    gpio_set_function(LED_R_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_G_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_B_PIN, GPIO_FUNC_PWM);
    uint slice_r = pwm_gpio_to_slice_num(LED_R_PIN);
    uint slice_g = pwm_gpio_to_slice_num(LED_G_PIN);
    uint slice_b = pwm_gpio_to_slice_num(LED_B_PIN);
    pwm_set_enabled(slice_r, true);
    pwm_set_enabled(slice_g, true);
    pwm_set_enabled(slice_b, true);

    // --- Inicializa Botão ---
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // --- Inicializa Sensor de Cor I2C ---
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    if (!tcs_init()) {
        // Se o sensor falhar, acende o LED vermelho como sinal de erro
        set_led_color_pwm(1);
        while (1); // Trava o programa
    }
    
    printf("Sistema Final Iniciado!\n");

    int led_state = 0; // Estado inicial: Branco
    set_led_color_pwm(led_state);

    // --- Loop Principal ---
    while (1) {
        // Tarefa 1: Verificar o botão
        if (gpio_get(BUTTON_PIN) == 0) {
            led_state = (led_state + 1) % 4;
            set_led_color_pwm(led_state);
            sleep_ms(250); // Debounce
        }

        // Tarefa 2: Ler e imprimir dados do sensor de cor
        uint8_t r_buf[2], g_buf[2], b_buf[2], c_buf[2];
        tcs_read_data(REG_RDATA, r_buf, 2);
        tcs_read_data(REG_GDATA, g_buf, 2);
        tcs_read_data(REG_BDATA, b_buf, 2);
        tcs_read_data(REG_CDATA, c_buf, 2);

        uint16_t red   = (r_buf[1] << 8) | r_buf[0];
        uint16_t green = (g_buf[1] << 8) | g_buf[0];
        uint16_t blue  = (b_buf[1] << 8) | b_buf[0];
        uint16_t clear = (c_buf[1] << 8) | c_buf[0];
        
        printf("R: %d, G: %d, B: %d, C: %d\n", red, green, blue, clear);

        // Controla a frequência geral do loop
        sleep_ms(300);
    }
    return 0;
}