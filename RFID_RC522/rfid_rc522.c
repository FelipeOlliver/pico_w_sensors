#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "mfrc522.h"

const uint BUZZER_PIN = 21;

void beep() {
    // Busca a "fatia" (slice) do hardware PWM correspondente ao nosso pino
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Liga a fatia PWM para iniciar o som
    pwm_set_enabled(slice_num, true);
    sleep_ms(250); // Duração do beep
    // Desliga a fatia PWM para parar o som
    pwm_set_enabled(slice_num, false);
}

int main() {
    // Inicializa a saída serial via USB
    stdio_init_all();
    sleep_ms(5000); // Pausa para o monitor serial se conectar
    printf("Inicializando leitor RFID RC522...\n");

    // --- NOVO: Configuração do pino do Buzzer para PWM ---
    // Informa ao pino 21 que ele será controlado pelo hardware PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    
    // Encontra a "fatia" PWM correspondente
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Configura o PWM para gerar uma frequência de ~1kHz
    // Clock do sistema (125MHz) / (Divisor * Wrap) = Frequência
    // 125,000,000 / (10 * 12500) = 1000 Hz = 1 kHz
    pwm_set_clkdiv(slice_num, 10.0f);      // Define o divisor do clock
    pwm_set_wrap(slice_num, 12500);       // Define o valor máximo do contador (período da onda)
    
    // Define o "duty cycle" (largura do pulso) para 50%
    // O som é gerado quando o pino está em nível alto. 50% é ideal para um tom claro.
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 6250); // GP21 é o canal B da fatia PWM 2
    // A fatia PWM começa desligada
    pwm_set_enabled(slice_num, false);
    // --- Fim da configuração do PWM ---

    // Inicialização do leitor RFID
    MFRC522Ptr_t rfid = MFRC522_Init();
    PCD_Init(rfid, spi0);
    
    printf("Leitor pronto! Aproxime um cartao ou tag.\n");

    while (1) {
        if (PICC_IsNewCardPresent(rfid)) {
            
            if (PICC_ReadCardSerial(rfid)) {
                
                // NOVO: Chama a função de beep duas vezes
                beep();
                sleep_ms(100); // Uma pequena pausa entre os bipes
                beep();

                // Imprime o UID do cartão no monitor serial
                printf("Cartao detectado! UID: ");
                for (uint8_t i = 0; i < rfid->uid.size; i++) {
                    printf("%02X", rfid->uid.uidByte[i]);
                    if (i < rfid->uid.size - 1) {
                        printf(":");
                    }
                }
                printf("\n");

                PICC_HaltA(rfid);
            }
        }
        sleep_ms(100);
    }
    return 0;
}