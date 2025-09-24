#include <stdio.h>
#include <math.h> // Necessário para atan2 e sqrt
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/binary_info.h"

// --- Definições dos Pinos ---
#define SERVO_PIN 8
// Definições dos pinos I2C personalizados
#define I2C_PORT i2c0       // Usa a porta I2C0
#define I2C_SDA 0           // Pino GPIO 0 como SDA
#define I2C_SCL 1           // Pino GPIO 1 como SCL

// --- Definições do Servo Motor ---
#define PULSE_MIN 500  // Valor de pulso para -90 graus
#define PULSE_MAX 2500 // Valor de pulso para +90 graus

// --- Definições do MPU6050 ---
static int MPU6050_ADDR = 0x68;
// Fator de sensibilidade para a escala padrão de +/- 2g
const float ACCEL_SENSITIVITY = 16384.0f;

// Função de mapeamento para o servo
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Converte ângulo para largura de pulso PWM
uint16_t angle_to_pulse(float angle) {
    if (angle < -90) angle = -90;
    if (angle > 90) angle = 90;
    return (uint16_t)map(angle, -90, 90, PULSE_MIN, PULSE_MAX);
}

// --- Funções do MPU6050 ---
static void mpu6050_reset() {
    uint8_t buf[] = {0x6B, 0x00}; // Acorda o MPU6050
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);
}

static void mpu6050_read_raw(int16_t accel[3]) {
    uint8_t buffer[6];
    uint8_t reg = 0x3B; // Registro inicial dos dados do acelerômetro
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, buffer, 6, false);

    for (int i = 0; i < 3; i++) {
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }
}

int main() {
    stdio_init_all();
    
    // Pequeno atraso para o monitor serial iniciar, se necessário
    sleep_ms(2000);

    // --- Inicialização do I2C para o MPU6050 nos pinos personalizados ---
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Usa o pino definido
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Usa o pino definido
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // Atualiza a informação de debug para os novos pinos
    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));

    mpu6050_reset();

    // --- Inicialização do PWM para o Servo Motor ---
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 62.5f);
    pwm_config_set_wrap(&config, 20000); // Para 50Hz
    
    pwm_init(slice_num, &config, true);

    int16_t acceleration_raw[3];

    printf("Iniciando controle de servo com MPU6050...\n");
    printf("Usando I2C em SDA: GP%d, SCL: GP%d\n", I2C_SDA, I2C_SCL);

    while (1) {
        // 1. Ler os dados brutos do acelerômetro
        mpu6050_read_raw(acceleration_raw);

        // 2. Converter os dados brutos para 'g' (unidades de gravidade)
        float acc_x = acceleration_raw[0] / ACCEL_SENSITIVITY;
        float acc_y = acceleration_raw[1] / ACCEL_SENSITIVITY;
        float acc_z = acceleration_raw[2] / ACCEL_SENSITIVITY;

        // 3. Calcular o ângulo de inclinação lateral (roll)
        float roll_angle = atan2(acc_y, acc_z) * 180.0 / M_PI;

        // 4. Imprimir os dados formatados no monitor serial
        printf("Acc X: %.2f | Y: %.2f | Z: %.2f | Angulo (Roll): %.0f°\n",
               acc_x, acc_y, acc_z, roll_angle);

        // 5. Mover o servo para o ângulo correspondente
        pwm_set_gpio_level(SERVO_PIN, angle_to_pulse(roll_angle));
        
        // Pequeno atraso para estabilidade
        sleep_ms(50);
    }

    return 0;
}