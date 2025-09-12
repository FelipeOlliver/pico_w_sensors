#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// --- CONFIGURAÇÕES DO SENSOR BH1750 ---
#define I2C_PORT i2c1
#define I2C_SDA_PIN 2
#define I2C_SCL_PIN 3
const uint8_t BH1750_ADDR = 0x23;
const uint8_t BH1750_POWER_ON = 0x01;
const uint8_t BH1750_CONT_H_RES_MODE = 0x10;
const float LUX_CONVERSION_FACTOR = 1.2;
uint8_t current_mtreg = 69;

// --- CONFIGURAÇÕES DO SERVO MOTOR ---
#define SERVO_PIN 8
#define PULSE_MIN 500  // Pulso para -90 graus
#define PULSE_MAX 2500 // Pulso para +90 graus

// --- AJUSTE PRINCIPAL DA LÓGICA ---
// Define qual valor de lux corresponde ao ângulo máximo do servo (+90).
// Ajuste este valor de acordo com a iluminação do seu ambiente.
#define MAX_LUX_PARA_MAPEAMENTO 2000.0f 

// --- FUNÇÕES AUXILIARES (SENSOR E SERVO) ---

// Função de mapeamento genérica
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Envia comando para o sensor BH1750
void bh1750_send_command(uint8_t command) {
    i2c_write_blocking(I2C_PORT, BH1750_ADDR, &command, 1, false);
}

// Inicializa o sensor BH1750
void bh1750_init() {
    bh1750_send_command(BH1750_POWER_ON);
    sleep_ms(10);
    bh1750_send_command(BH1750_CONT_H_RES_MODE);
    sleep_ms(120);
}

// Lê o valor de luminosidade do sensor
float bh1750_read_lux() {
    uint8_t buffer[2];
    i2c_read_blocking(I2C_PORT, BH1750_ADDR, buffer, 2, false);
    uint16_t raw_value = (buffer[0] << 8) | buffer[1];
    return (raw_value / LUX_CONVERSION_FACTOR);
}

// Converte um ângulo em largura de pulso para o servo
uint16_t angle_to_pulse(float angle) {
    if (angle < -90) angle = -90;
    if (angle > 90) angle = 90;
    return (uint16_t)map(angle, -90, 90, PULSE_MIN, PULSE_MAX);
}

// --- FUNÇÃO PRINCIPAL ---
int main() {
    stdio_init_all();
    sleep_ms(2000); // Delay para conectar o monitor serial

    // --- Inicialização do I2C para o sensor BH1750 ---
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    bh1750_init();
    printf("Sensor BH1750 inicializado.\n");

    // --- Inicialização do PWM para o Servo Motor ---
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config config = pwm_get_default_config();
    // Frequência do PWM em 50Hz (período de 20ms)
    uint32_t clkdiv = (uint32_t)(clock_get_hz(clk_sys) / (50 * 20000));
    pwm_config_set_clkdiv(&config, clkdiv);
    pwm_config_set_wrap(&config, 19999);
    pwm_init(slice_num, &config, true);
    printf("Servo Motor inicializado no pino %d.\n", SERVO_PIN);

    // --- Loop Principal de Controle ---
    while (1) {
        // 1. Lê a luminosidade do sensor
        float lux = bh1750_read_lux();

        // 2. Mapeia o valor de lux para um ângulo de servo (-90 a +90)
        // A faixa de entrada do mapeamento é de 0 até o valor que você definiu
        float angle = map(lux, 0, MAX_LUX_PARA_MAPEAMENTO, -90, 90);

        // 3. Garante que o ângulo não ultrapasse os limites do servo
        if (angle < -90) angle = -90;
        if (angle > 90) angle = 90;

        // 4. Converte o ângulo para pulso e comanda o servo
        uint16_t pulse = angle_to_pulse(angle);
        pwm_set_gpio_level(SERVO_PIN, pulse);

        // Imprime os valores para depuração
        printf("Luminosidade: %.2f lx -> Ângulo: %.2f graus (Pulso: %d)\n", lux, angle, pulse);
        
        // Pequeno delay para estabilidade
        sleep_ms(100);
    }

    return 0;
}