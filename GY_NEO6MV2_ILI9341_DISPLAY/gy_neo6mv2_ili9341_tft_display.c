#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/uart.h" // Para o GPS
#include "hardware/gpio.h"

#include "ili9341.h" // Para o display
#include "gfx.h"     // Para as funções gráficas

// --- Configurações da UART para o GPS ---
#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// Converte coordenadas do formato NMEA para Graus Decimais
void convert_coords(double nmea_coord, char hemisphere, double *decimal_degrees) {
    double degrees = floor(nmea_coord / 100.0);
    double minutes = nmea_coord - (degrees * 100.0);
    *decimal_degrees = degrees + (minutes / 60.0);
    if (hemisphere == 'S' || hemisphere == 'W') {
        *decimal_degrees = -(*decimal_degrees);
    }
}

// Estrutura para guardar os dados do GPS de forma organizada
typedef struct {
    int hour, minute, second;
    double latitude, longitude;
    int fix_quality, num_sats;
    float altitude;
    bool has_fix;
} gps_data_t;

// --- Função Principal ---
int main() {
    // Inicialização da E/S padrão (para debug via USB, se necessário)
    stdio_init_all();

    // --- Inicialização do Display ---
    LCD_initDisplay();
    LCD_setRotation(1); // Rotação 1 = 320x240 (paisagem)
    GFX_createFramebuf();
    GFX_fillScreen(ILI9341_BLACK); // Limpa a tela com fundo preto
    GFX_flush();

    // --- Inicialização da UART para o GPS ---
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // --- Mensagem inicial no display ---
    GFX_setCursor(10, 10);
    GFX_setTextSize(2); // Tamanho da fonte 2
    GFX_setTextColor(ILI9341_YELLOW);
    GFX_printf("Aguardando sinal GPS...");
    GFX_flush(); // Envia o framebuffer para o display

    char gps_buffer[120];
    int buffer_index = 0;
    gps_data_t current_gps_data = {0}; // Inicializa a estrutura de dados

    // --- Loop Principal ---
    while (true) {
        // Verifica se há dados chegando do GPS
        if (uart_is_readable(UART_ID)) {
            char ch = uart_getc(UART_ID);

            if (ch == '\n' && buffer_index > 0) {
                gps_buffer[buffer_index] = '\0';

                // Se for uma sentença GPGGA, faz o parsing
                if (strstr(gps_buffer, "$GPGGA") != NULL) {
                    float utc_time, nmea_lat, nmea_lon;
                    char ns, ew;
                    
                    int parsed_items = sscanf(gps_buffer, 
                                            "$GPGGA,%f,%f,%c,%f,%c,%d,%d,%*f,%f",
                                            &utc_time, &nmea_lat, &ns, &nmea_lon, &ew, 
                                            &current_gps_data.fix_quality, &current_gps_data.num_sats, &current_gps_data.altitude);

                    if (parsed_items >= 8 && current_gps_data.fix_quality > 0) {
                        current_gps_data.has_fix = true;

                        // Tempo (UTC-3)
                        current_gps_data.hour = (int)(utc_time / 10000) - 3;
                        if (current_gps_data.hour < 0) current_gps_data.hour += 24;
                        current_gps_data.minute = ((int)utc_time / 100) % 100;
                        current_gps_data.second = (int)utc_time % 100;

                        // Coordenadas
                        convert_coords(nmea_lat, ns, &current_gps_data.latitude);
                        convert_coords(nmea_lon, ew, &current_gps_data.longitude);

                    } else {
                        current_gps_data.has_fix = false;
                    }
                    
                    // --- ATUALIZAÇÃO DO DISPLAY ---
                    GFX_fillScreen(ILI9341_BLACK); // Limpa a tela para redesenhar
                    GFX_setCursor(0, 10);
                    GFX_setTextSize(2); // Fonte menor para caber tudo

                    if (current_gps_data.has_fix) {
                        // Define a cor do texto para verde se tiver sinal
                        GFX_setTextColor(ILI9341_GREEN);
                        GFX_printf(" Sinal OK (%d satelites)\n\n", current_gps_data.num_sats);
                        
                        // Mostra os dados formatados
                        GFX_setTextColor(ILI9341_WHITE);
                        GFX_printf(" Hora: %02d:%02d:%02d\n\n", current_gps_data.hour, current_gps_data.minute, current_gps_data.second);
                        GFX_printf(" Lat:  %.5f\n\n", current_gps_data.latitude);
                        GFX_printf(" Lon:  %.5f\n\n", current_gps_data.longitude);
                        GFX_printf(" Alt:  %.1f m", current_gps_data.altitude);

                    } else {
                        // Se não tiver sinal, mantém a mensagem de espera
                        GFX_setTextColor(ILI9341_YELLOW);
                        GFX_printf(" Aguardando sinal GPS...");
                    }
                    
                    GFX_flush(); // Envia as atualizações para o display
                }

                buffer_index = 0; // Reseta o buffer da UART
            } else if (ch != '\r') {
                if (buffer_index < sizeof(gps_buffer) - 1) {
                    gps_buffer[buffer_index++] = ch;
                }
            }
        }
    }
    return 0; // Nunca vai chegar aqui
}