#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h" // Para o buzzer
#include "hardware/gpio.h" // Para o buzzer
#include "max30100.h"

// --- Definições do Sensor I2C ---
#define I2C_PORT i2c0
#define I2C_SDA_PIN 0
#define I2C_SCL_PIN 1

// --- Definições do Buzzer ---
const uint BUZZER_PIN = 21; // Usando o pino 21, como no seu exemplo

// --- Funções do Buzzer ---
void init_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_clkdiv(slice_num, 10.0f);
    pwm_set_wrap(slice_num, 12500); // Frequência de 1kHz
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), 6250); // 50% duty cycle
    pwm_set_enabled(slice_num, false); // Começa desligado
}

void beep() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, true);
    sleep_ms(150); // Duração do beep (ajustado para ser um pouco mais curto)
    pwm_set_enabled(slice_num, false);
}

// --- Funções de Beeper Múltiplos ---
void three_beeps() {
    beep();
    sleep_ms(100);
    beep();
    sleep_ms(100);
    beep();
}

int main() {
    stdio_init_all();
    sleep_ms(10000);
    printf("Iniciando o sistema de monitoramento MAX30102...\n");

    if (!max30102_init(I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN)) {
        printf("Falha ao inicializar o sensor MAX30102. Travando.\n");
        while (1);
    }
    
    init_buzzer(); // Inicializa o buzzer

    max30102_data_t current_data;
    bool finger_present = false;
    uint32_t measurement_start_time = 0;
    const uint32_t MEASUREMENT_DURATION_MS = 15000; // Medir por 10 segundos
    
    // --- Variáveis para os algoritmos de BPM e SpO2 (placeholders) ---
    // Você precisaria de arrays/buffers para armazenar as amostras para processamento.
    // float ir_buffer[MAX_SAMPLES];
    // float red_buffer[MAX_SAMPLES];
    // int sample_count = 0;

    printf("\nAguardando dedo no sensor...\n");

    while (1) {
        max30102_read_fifo(&current_data);

        // --- Lógica de Detecção de Dedo ---
        if (!finger_present && current_data.ir > FINGER_ON_THRESHOLD) {
            finger_present = true;
            measurement_start_time = to_ms_since_boot(get_absolute_time());
            printf("\nDedo detectado! Iniciando medicão por %lu segundos...\n", MEASUREMENT_DURATION_MS / 1000);
            // Aqui você resetaria seus buffers de DSP se estivesse usando
            // sample_count = 0;
        } else if (finger_present && current_data.ir < FINGER_ON_THRESHOLD) {
            finger_present = false;
            printf("\nDedo removido. Aguardando dedo no sensor...\n");
            // Se estivesse usando, aqui você processaria os dados coletados do ciclo anterior
            // calculate_bpm_spo2(ir_buffer, red_buffer, sample_count);
        }

        // --- Lógica de Medição (quando o dedo está presente) ---
        if (finger_present) {
            printf("IR: %lu, Red: %lu\n", current_data.ir, current_data.red);

            // Armazenar dados para os algoritmos (exemplo, você precisaria do buffer real)
            // if (sample_count < MAX_SAMPLES) {
            //     ir_buffer[sample_count] = (float)current_data.ir;
            //     red_buffer[sample_count] = (float)current_data.red;
            //     sample_count++;
            // }

            // Verifica se o tempo de medição terminou
            if ((to_ms_since_boot(get_absolute_time()) - measurement_start_time) >= MEASUREMENT_DURATION_MS) {
                printf("\nMedição concluída!\n");
                
                // --- AQUI É ONDE SEUS ALGORITMOS DE BPM E SPO2 ENTRARIAM ---
                // Exemplo (pseudocódigo):
                // float bpm = calculate_bpm(ir_buffer, sample_count);
                // float spo2 = calculate_spo2(ir_buffer, red_buffer, sample_count);
                // printf("BPM: %.0f, SpO2: %.1f%%\n", bpm, spo2);
                printf("Resultados simulados: BPM: 75, SpO2: 98.5%%\n"); // Apenas para demonstração

                three_beeps(); // 3 beeps ao final da medição
                finger_present = false; // Reinicia o estado para aguardar novo dedo
                printf("\nAguardando dedo no sensor novamente...\n");
                sleep_ms(2000);
            }
        }
        
        sleep_ms(1000); // Intervalo entre as leituras
    }

    return 0;
}