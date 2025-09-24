#include <stdio.h>
#include <math.h> // Necessário para atan2 e sqrt
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "pico/binary_info.h"
#include "ili9341.h"
#include "gfx.h"

#define ANGULO_ALERTA 45.0f

// Definições dos pinos I2C personalizados
#define I2C_PORT i2c0       // Usa a porta I2C0
#define I2C_SDA 0           // Pino GPIO 0 como SDA
#define I2C_SCL 1           // Pino GPIO 1 como SCL

// --- Definições do MPU6050 ---
static int MPU6050_ADDR = 0x68;
// Fator de sensibilidade para a escala padrão de +/- 2g
const float ACCEL_SENSITIVITY = 16384.0f;

// Função de mapeamento para o servo
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
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

    // --- Inicialização do I2C para o MPU6050 ---
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    mpu6050_reset();

    // --- Inicialização do Display ILI9341 ---
    LCD_initDisplay();
    LCD_setRotation(1);
    GFX_createFramebuf();

    int16_t acceleration_raw[3];

    while (true) {
        // 1. Ler e converter dados do acelerômetro
        mpu6050_read_raw(acceleration_raw);
        float acc_x = acceleration_raw[0] / ACCEL_SENSITIVITY;
        float acc_y = acceleration_raw[1] / ACCEL_SENSITIVITY;
        float acc_z = acceleration_raw[2] / ACCEL_SENSITIVITY;

        // 2. Calcular o ângulo de inclinação lateral (roll)
        float roll_angle = atan2(acc_y, acc_z) * 180.0 / M_PI;

        // 3. Lógica de Alerta Visual
        // A função fabs() calcula o valor absoluto (ignora se o ângulo é positivo ou negativo)
        if (fabs(roll_angle) > ANGULO_ALERTA) {
            // --- ESTADO DE ALERTA ---
            GFX_setClearColor(ILI9341_RED);
            GFX_clearScreen();
            GFX_setCursor(20, 50);
            GFX_setTextSize(3);
            GFX_setTextColor(ILI9341_WHITE);
            GFX_printf("ALERTA DE\nINCLINACAO!\n\n");
            GFX_setTextSize(2);
            GFX_printf("  Angulo: %.1f%c", roll_angle, 247); // 247 é o caractere de grau (°)
        } else {
            // --- ESTADO NORMAL ---
            GFX_setClearColor(ILI9341_BLACK);
            GFX_clearScreen();
            GFX_setCursor(0, 20);
            GFX_setTextSize(3);
            GFX_setTextColor(ILI9341_WHITE);
            GFX_printf("Acc X: %.2f g\n", acc_x);
            GFX_printf("Acc Y: %.2f g\n", acc_y);
            GFX_printf("Acc Z: %.2f g\n", acc_z);
            GFX_printf("\nAngulo: %.1f%c", roll_angle, 247);
        }

        // 4. Atualizar o display com o conteúdo do buffer
        GFX_flush();

        sleep_ms(100);
    }
    return 0;
}