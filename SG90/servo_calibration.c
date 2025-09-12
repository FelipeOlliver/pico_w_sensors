#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Necessário para a função memset
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define SERVO_PIN 8

int main() {
    stdio_init_all();

    // Aguarda a conexão do monitor serial (opcional, mas bom para garantir)
    // sleep_ms(5000); 

    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config config = pwm_get_default_config();
    uint32_t clkdiv = (uint32_t)(clock_get_hz(clk_sys) / 1000000);
    pwm_config_set_clkdiv(&config, clkdiv);
    pwm_config_set_wrap(&config, 19999);
    pwm_init(slice_num, &config, true);

    // Começa na sua posição central conhecida
    uint16_t current_pulse = 1400; 
    pwm_set_gpio_level(SERVO_PIN, current_pulse);

    printf("\n--- Calibracao de Servo Iniciada ---\n");
    printf("Digite um valor de pulso (400 a 2600) e pressione Enter.\n");
    
    char buffer[20];
    int buffer_pos = 0;

    while (true) {
        printf("Pulso atual: %u. Digite novo valor -> ", current_pulse);

        // Zera o buffer antes de ler nova entrada
        memset(buffer, 0, sizeof(buffer));
        buffer_pos = 0;

        while(buffer_pos < (sizeof(buffer) - 1)) {
            int c = getchar_timeout_us(30 * 1000 * 1000); // Timeout de 30 segundos

            if (c == PICO_ERROR_TIMEOUT) {
                // Se não houver entrada, continua o loop
                break;
            }

            // Se o caractere for Enter (newline ou carriage return), termina a leitura
            if (c == '\n' || c == '\r') {
                break;
            }
            
            // Adiciona o caractere ao buffer
            buffer[buffer_pos++] = (char)c;
        }

        // Se algo foi lido, processa
        if (buffer_pos > 0) {
            // Converte a string lida para um número inteiro
            int new_pulse = atoi(buffer);
            
            printf("Lido: '%s' -> Convertido para: %d\n", buffer, new_pulse);

            // Verifica se o valor está dentro da faixa de segurança
            if (new_pulse >= 50 && new_pulse <= 3600) {
                current_pulse = (uint16_t)new_pulse;
                pwm_set_gpio_level(SERVO_PIN, current_pulse);
                printf("Servo movido para pulso: %u\n", current_pulse);
            } else {
                printf("ERRO: Valor '%d' fora da faixa segura (400-2600).\n", new_pulse);
            }
        }
    }

    return 0;
}