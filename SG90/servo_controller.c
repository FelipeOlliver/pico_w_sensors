#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define SERVO_PIN 8

// --- VALORES AJUSTADOS DE ACORDO COM SUA CALIBRAÇÃO ---
#define PULSE_MIN 500   // Valor para -90 graus
#define PULSE_MAX 2500  // Valor para +90 graus

// Função de mapeamento que converte um valor de uma faixa para outra
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t angle_to_pulse(float angle) {
    if (angle < -90) angle = -90;
    if (angle > 90) angle = 90;

    // Mapeia o ângulo (-90 a 90) para a faixa de pulso calibrada (PULSE_MIN a PULSE_MAX)
    return (uint16_t)map(angle, -90, 90, PULSE_MIN, PULSE_MAX);
}

int main() {
    stdio_init_all();
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config config = pwm_get_default_config();
    uint32_t clkdiv = (uint32_t)(clock_get_hz(clk_sys) / 1000000); // 1MHz
    pwm_config_set_clkdiv(&config, clkdiv);
    pwm_config_set_wrap(&config, 19999);
    pwm_init(slice_num, &config, true);

    // Loop principal para mover o servo
    while (1) {
        printf("Movendo para -90 graus (Pulso: %d)\n", angle_to_pulse(-90));
        pwm_set_gpio_level(SERVO_PIN, angle_to_pulse(-90));
        sleep_ms(2000);

        printf("Movendo para 0 graus (Pulso: %d)\n", angle_to_pulse(0));
        pwm_set_gpio_level(SERVO_PIN, angle_to_pulse(0));
        sleep_ms(2000);
        
        printf("Movendo para 45 graus (Pulso: %d)\n", angle_to_pulse(45));
        pwm_set_gpio_level(SERVO_PIN, angle_to_pulse(45));
        sleep_ms(2000);

        printf("Movendo para 90 graus (Pulso: %d)\n", angle_to_pulse(90));
        pwm_set_gpio_level(SERVO_PIN, angle_to_pulse(90));
        sleep_ms(2000);
    }

    return 0;
}