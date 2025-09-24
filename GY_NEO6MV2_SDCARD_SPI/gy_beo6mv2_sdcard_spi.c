#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"

#define PIN_MISO     16
#define PIN_MOSI     19
#define PIN_SCK      18
#define PIN_CS       17

#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1

FATFS fs;
FIL file;

// --- Estrutura para armazenar os dados do GPS ---
typedef struct {
    int hour, minute, second;
    double latitude, longitude;
    int fix_quality, num_sats;
    float altitude;
    bool has_fix;
} gps_data_t;

// Função de conversão de coordenadas (do código do GPS)
void convert_coords(double nmea_coord, char hemisphere, double *decimal_degrees) {
    double degrees = floor(nmea_coord / 100.0);
    double minutes = nmea_coord - (degrees * 100.0);
    *decimal_degrees = degrees + (minutes / 60.0);
    if (hemisphere == 'S' || hemisphere == 'W') {
        *decimal_degrees = -(*decimal_degrees);
    }
}

// === Inicializa cartão SD via SPI ===
void inicializar_sd() {
    spi_init(spi0, 1000 * 1000); // Inicializa SPI a 1MHz
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("Erro ao montar SD: %d\n", fr);
        return;
    }
    printf("Cartao SD montado com sucesso.\n");

    fr = f_open(&file, "gps_log.csv", FA_WRITE | FA_OPEN_APPEND);
    if (fr != FR_OK) {
        printf("Erro ao abrir/criar gps_log.csv: %d\n", fr);
        return;
    }
    
    if (f_size(&file) == 0) {
        f_puts("Hora_Local,Latitude,Longitude,Altitude_Metros,Satelites\n", &file);
        f_sync(&file);
    }
    printf("Arquivo gps_log.csv pronto para registrar dados.\n");
}

// === Registra os dados do GPS no arquivo CSV ===
void registrar_log_gps(gps_data_t* data) {
    if (!data->has_fix) return;

    char linha[128];
    snprintf(linha, sizeof(linha), "%02d:%02d:%02d,%.6f,%.6f,%.1f,%d\n",
             data->hour, data->minute, data->second,
             data->latitude, data->longitude,
             data->altitude, data->num_sats);
    
    UINT bytes_written;
    FRESULT fr = f_write(&file, linha, strlen(linha), &bytes_written);
    if (fr != FR_OK) {
        printf("Erro ao escrever no arquivo: %d\n", fr);
    } else {
        f_sync(&file);
        // Alterado para não poluir o log principal com esta mensagem
        // printf("Log salvo: %s", linha); 
    }
}

// === Função principal ===
int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Iniciando GPS Logger...\n");

    inicializar_sd();

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    printf("Aguardando dados do GPS (dados brutos serao exibidos abaixo)...\n\n");
    
    char gps_buffer[120];
    int buffer_index = 0;
    
    gps_data_t latest_gps_data = {0}; 
    
    absolute_time_t proximo_log = delayed_by_ms(get_absolute_time(), 5000);

    while (true) {
        if (uart_is_readable(UART_ID)) {
            char ch = uart_getc(UART_ID);
            if (ch == '\n' && buffer_index > 0) {
                gps_buffer[buffer_index] = '\0';
                
                // <<< LINHA ADICIONADA PARA DEBUG >>>
                // Imprime a sentença NMEA completa assim que ela é recebida.
                printf("%s\n", gps_buffer);

                if (strstr(gps_buffer, "$GPGGA") != NULL) {
                    float utc_time, nmea_lat, nmea_lon;
                    char ns, ew;
                    int fix, sats;
                    float alt;

                    int parsed_items = sscanf(gps_buffer, "$GPGGA,%f,%f,%c,%f,%c,%d,%d,%*f,%f",
                                            &utc_time, &nmea_lat, &ns, &nmea_lon, &ew, 
                                            &fix, &sats, &alt);

                    if (parsed_items >= 8 && fix > 0) {
                        latest_gps_data.has_fix = true;
                        latest_gps_data.fix_quality = fix;
                        latest_gps_data.num_sats = sats;
                        latest_gps_data.altitude = alt;
                        
                        latest_gps_data.hour = (int)(utc_time / 10000) - 3;
                        if (latest_gps_data.hour < 0) latest_gps_data.hour += 24;
                        latest_gps_data.minute = ((int)utc_time / 100) % 100;
                        latest_gps_data.second = (int)utc_time % 100;
                        
                        convert_coords(nmea_lat, ns, &latest_gps_data.latitude);
                        convert_coords(nmea_lon, ew, &latest_gps_data.longitude);
                    } else {
                        latest_gps_data.has_fix = false;
                    }
                }
                buffer_index = 0;
            } else if (ch != '\r') {
                if (buffer_index < sizeof(gps_buffer) - 1) {
                    gps_buffer[buffer_index++] = ch;
                }
            }
        }

        if (time_reached(proximo_log)) {
            if (latest_gps_data.has_fix) {
                registrar_log_gps(&latest_gps_data);
                printf("--- DADOS VALIDOS REGISTRADOS NO SD ---\n");
            } else {
                printf("--- SEM SINAL, NENHUM LOG SALVO ---\n");
            }
            proximo_log = delayed_by_ms(get_absolute_time(), 5000);
        }
    }

    f_close(&file);
    return 0;
}