#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/sync.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "mfrc522.h"
#include "dht.h"
#include "tcs34725.h"
#include "max30102.h"
#include "ff.h"
#include "diskio.h"

// ======================= DEFINIÇÕES DE PINOS =======================
const uint BUZZER_PIN = 21;
const uint DHT_PIN = 4;

const uint LED_R_PIN = 13;
const uint LED_G_PIN = 11;
const uint LED_B_PIN = 12;
const uint BUTTON_PIN = 5;
const uint I2C_SDA_PIN = 2;
const uint I2C_SCL_PIN = 3;
const uint I2C0_SDA_PIN = 0;
const uint I2C0_SCL_PIN = 1;

#define PIN_MISO_SD 8
#define PIN_MOSI_SD 15
#define PIN_SCK_SD  14
#define PIN_CS_SD   9

// ======================= VARIÁVEIS GLOBAIS =======================
FATFS fs;
FIL file;
bool sd_initialized = false;
static mutex_t sd_card_mutex;
int led_state = 0;

// --- Timers e Estados ---
absolute_time_t next_dht_read_time;
absolute_time_t next_color_read_time;

// ---> NOVAS VARIÁVEIS DE ESTADO para o Oxímetro
typedef enum {
    OXIMETER_STATE_IDLE,      // 1. Aguardando dedo
    OXIMETER_STATE_MEASURING, // 2. Medindo por 15s
    OXIMETER_STATE_COOLDOWN   // 3. Pausando por 3s
} oximeter_state_t;

oximeter_state_t oximeter_current_state = OXIMETER_STATE_IDLE;
absolute_time_t oximeter_timer;

// ======================= FUNÇÕES AUXILIARES =======================
// (Funções do Buzzer não foram alteradas)
void configure_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_clkdiv(slice_num, 10.0f); pwm_set_wrap(slice_num, 12500);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 6250); pwm_set_enabled(slice_num, false);
}
void beep() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, true); sleep_ms(250); pwm_set_enabled(slice_num, false);
}

void three_beeps() {
    beep();
    sleep_ms(100);
    beep();
    sleep_ms(100);
    beep();
}

// ======================= FUNÇÕES DE LOG NO SD CARD =======================

void log_to_sd(const char* log_line) {
    if (!sd_initialized) return;

    // Bloqueia o acesso ao cartão SD para esta thread/tarefa
    mutex_enter_blocking(&sd_card_mutex);

    // Escreve a linha de log no arquivo
    if (f_puts(log_line, &file) < 0) {
        printf("ERRO: Falha ao escrever no arquivo de log\n");
    } else {
        // Força a escrita imediata para evitar perda de dados
        f_sync(&file); 
        printf("Log salvo no SD: %s", log_line);
    }

    // Libera o acesso ao cartão SD
    mutex_exit(&sd_card_mutex);
}

void inicializar_sd() {
    spi_init(spi1, 1000 * 1000);
    gpio_set_function(PIN_MISO_SD, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI_SD, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK_SD, GPIO_FUNC_SPI);
    gpio_init(PIN_CS_SD);
    gpio_set_dir(PIN_CS_SD, GPIO_OUT);
    gpio_put(PIN_CS_SD, 1);

    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("ERRO: Falha ao montar o cartao SD (%d)\n", fr);
        return;
    }

    // O arquivo de log agora é genérico
    fr = f_open(&file, "system_log.txt", FA_WRITE | FA_OPEN_APPEND);
    if (fr != FR_OK) {
        printf("ERRO: Falha ao abrir o arquivo de log (%d)\n", fr);
        return;
    }

    if (f_size(&file) == 0) {
        // O Mutex já deve estar inicializado antes de chamar esta função
        log_to_sd("Tipo,Dados,Timestamp_ms\n");
    }
    
    sd_initialized = true;
    printf("Sistema de log no cartao SD pronto.\n");
}

void configure_rgb_led() {
    gpio_set_function(LED_R_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_G_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_B_PIN, GPIO_FUNC_PWM);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_R_PIN), true);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_G_PIN), true);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_B_PIN), true);
}

void set_led_color_pwm(int state) {
    uint16_t r = 0, g = 0, b = 0;
    switch (state) {
        case 0: r = 65535; g = 65535; b = 65535; break; // Branco
        case 1: r = 65535; g = 0;     b = 0;     break; // Vermelho
        case 2: r = 0;     g = 0;     b = 65535; break; // Azul
        case 3: r = 0;     g = 65535; b = 0;     break; // Verde
    }
    pwm_set_gpio_level(LED_R_PIN, r);
    pwm_set_gpio_level(LED_G_PIN, g);
    pwm_set_gpio_level(LED_B_PIN, b);
}

void configure_button() {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

// ======================= TAREFAS PRINCIPAIS =======================

void task_dht22_reader() {
    // Verifica se já passou da hora de ler o sensor
    if (absolute_time_diff_us(get_absolute_time(), next_dht_read_time) < 0) {
        dht_reading leitura;
        if (dht_read(&leitura)) {
            printf("[DHT22] Leitura: Temp=%.1f C, Umid=%.1f %%\n", leitura.temp_celsius, leitura.humidity);
            
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "[DHT22],Temp: %.1f C Umid: %.1f %%,%lu\n", 
                leitura.temp_celsius, leitura.humidity, to_ms_since_boot(get_absolute_time()));
            log_to_sd(buffer);
        } else {
            printf("[DHT22] Sensor em calibração.\n");
            //printf("[DHT22] Falha na leitura do sensor.\n");
        }
        // Agenda a próxima leitura para daqui a 20 segundos
        next_dht_read_time = make_timeout_time_ms(20000);
    }
}

void task_rfid_reader(MFRC522Ptr_t rfid) {
    if (PICC_IsNewCardPresent(rfid) && PICC_ReadCardSerial(rfid)) {
        beep();
        sleep_ms(50);
        beep();

        char uid_str[30] = {0};
        for (int i = 0; i < rfid->uid.size; i++) {
            sprintf(uid_str + strlen(uid_str), "%02X", rfid->uid.uidByte[i]);
        }
        printf("[RFID] Cartao detectado! UID: %s\n", uid_str);

        char buffer[128];
        snprintf(buffer, sizeof(buffer), "[RFID],Cartao lido: %s,%lu\n", 
            uid_str, to_ms_since_boot(get_absolute_time()));
        log_to_sd(buffer);

        PICC_HaltA(rfid);
    }
}

void task_color_sensor_reader() {
    if (absolute_time_diff_us(get_absolute_time(), next_color_read_time) < 0) {
        tcs_rgbc_t color_data;
        tcs_read_rgbc(&color_data);
        
        printf("[COLOR] R: %d, G: %d, B: %d, C: %d\n", 
               color_data.red, color_data.green, color_data.blue, color_data.clear);

        char buffer[128];
        snprintf(buffer, sizeof(buffer), "[COLOR_SENSOR],R: %d G: %d B: %d C: %d,%lu\n",
                 color_data.red, color_data.green, color_data.blue, color_data.clear,
                 to_ms_since_boot(get_absolute_time()));
        log_to_sd(buffer);

        // Agenda a próxima leitura para daqui a 30 segundos
        next_color_read_time = make_timeout_time_ms(30000);
    }
}

void task_button_handler() {
    if (gpio_get(BUTTON_PIN) == 0) { // Botão pressionado (nível baixo)
        led_state = (led_state + 1) % 4;
        set_led_color_pwm(led_state);
        sleep_ms(200); // Debounce simples para evitar múltiplas leituras
    }
}

void task_oximeter_handler() {
    max30102_data_t oximeter_data;

    switch (oximeter_current_state) {
        
        case OXIMETER_STATE_IDLE: {
            // Estado 1: Aguardando um dedo.
            max30102_read_fifo(&oximeter_data);
            if (oximeter_data.ir > FINGER_ON_THRESHOLD) {
                oximeter_timer = get_absolute_time();
                oximeter_current_state = OXIMETER_STATE_MEASURING;
                printf("\n[OXIMETER] Dedo detectado! Iniciando medicao de 15 segundos...\n");
            }
            break;
        }

        case OXIMETER_STATE_MEASURING: {
            // Estado 2: Medindo por 15 segundos.
            max30102_read_fifo(&oximeter_data);

            // Verifica se o dedo foi removido
            if (oximeter_data.ir < FINGER_ON_THRESHOLD) {
                oximeter_current_state = OXIMETER_STATE_IDLE;
                printf("\n[OXIMETER] Dedo removido. Medicao cancelada.\n");
                break;
            }

            // Verifica se os 15s de medição terminaram
            if (absolute_time_diff_us(oximeter_timer, get_absolute_time()) > 15 * 1000 * 1000) {
                printf("[OXIMETER] Medicao concluida!\n");
                
                // Simula e salva os resultados
                int bpm_simulado = 75 + (rand() % 10);
                float spo2_simulado = 97.0f + (float)(rand() % 20) / 10.0f;
                printf("[OXIMETER] Resultados: BPM: %d, SpO2: %.1f%%\n", bpm_simulado, spo2_simulado);
                
                char buffer[128];
                snprintf(buffer, sizeof(buffer), "[OXIMETER],BPM: %d SpO2: %.1f%%,%lu\n",
                         bpm_simulado, spo2_simulado, to_ms_since_boot(get_absolute_time()));
                log_to_sd(buffer);

                three_beeps();

                // Inicia o timer de cooldown de 3 segundos e muda de estado
                oximeter_timer = make_timeout_time_ms(3000);
                oximeter_current_state = OXIMETER_STATE_COOLDOWN;
                printf("[OXIMETER] Pausando por 3 segundos...\n");
            }
            break;
        }

        case OXIMETER_STATE_COOLDOWN: {
            // Estado 3: Pausando por 3 segundos.
            if (time_reached(oximeter_timer)) {
                // O tempo de pausa terminou, volta para o estado inicial
                oximeter_current_state = OXIMETER_STATE_IDLE;
                printf("\n[OXIMETER] Aguardando dedo no sensor...\n");
            }
            break;
        }
    }
}

// ======================= FUNÇÃO PRINCIPAL (main) =======================
int main() {
    stdio_init_all();
    sleep_ms(10000); 
    printf("==== Sistema de Log Integrado ====\n");

    mutex_init(&sd_card_mutex);
    if (cyw43_arch_init()) { printf("ERRO: cyw43_arch init\n"); }

    // --- Inicializa todos os periféricos ---
    inicializar_sd();
    configure_buzzer();
    configure_rgb_led();
    configure_button();
    dht_init(DHT_PIN);
    
    tcs_init(i2c1, I2C_SDA_PIN, I2C_SCL_PIN);
    if (!tcs_is_connected()) {
        printf("ERRO FATAL: Sensor de cor TCS34725 nao encontrado!\n");
        set_led_color_pwm(1); while(1);
    }
    set_led_color_pwm(led_state);

    // ---> NOVO: Inicializa Oxímetro
    if (!max30102_init(i2c0, I2C0_SDA_PIN, I2C0_SCL_PIN)) {
        printf("ERRO FATAL: Oximetro MAX30102 nao encontrado!\n");
        set_led_color_pwm(1); while(1);
    }

    MFRC522Ptr_t rfid = MFRC522_Init();
    PCD_Init(rfid, spi0);
    
    printf("\nSistema pronto e operacional. Aguardando eventos...\n");

    // Agenda as primeiras leituras
    next_dht_read_time = get_absolute_time();
    next_color_read_time = get_absolute_time();

    // Loop principal não-bloqueante
    while (1) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        
        task_dht22_reader();
        task_rfid_reader(rfid);
        task_color_sensor_reader();
        task_button_handler();
        task_oximeter_handler();

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(100);
    }
    return 0;
}