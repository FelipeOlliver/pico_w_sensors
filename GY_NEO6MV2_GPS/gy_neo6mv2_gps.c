#include <stdio.h>
#include <string.h> // Para usar a função strstr e sscanf
#include <stdlib.h> // Para usar atof (string para float)
#include <math.h>   // Para usar floor
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

// --- Configurações da UART (as mesmas de antes) ---
#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// Função para converter do formato NMEA (DDMM.MMMM) para Graus Decimais
// Ex: 0343.75056 -> 3 graus e 43.75056 minutos -> 3 + (43.75056 / 60) = 3.7291759
void convert_coords(double nmea_coord, char hemisphere, double *decimal_degrees) {
    // Pega a parte dos graus (ex: 0343.75056 -> 3)
    double degrees = floor(nmea_coord / 100.0);
    // Pega a parte dos minutos (ex: 43.75056)
    double minutes = nmea_coord - (degrees * 100.0);
    // Converte para graus decimais
    *decimal_degrees = degrees + (minutes / 60.0);

    // Inverte o sinal se for Sul ou Oeste
    if (hemisphere == 'S' || hemisphere == 'W') {
        *decimal_degrees = -(*decimal_degrees);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Iniciando leitor e parser GPS...\n\n");

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    char gps_buffer[120];
    int index = 0;

    while (1) {
        if (uart_is_readable(UART_ID)) {
            char ch = uart_getc(UART_ID);
            if (ch == '\n') {
                gps_buffer[index] = '\0';

                // --- INÍCIO DA LÓGICA DE PARSING ---

                // 1. Verifica se a sentença recebida é a $GPGGA
                if (strstr(gps_buffer, "$GPGGA") != NULL) {
                    
                    // Variáveis para armazenar os dados extraídos
                    float utc_time, nmea_lat, nmea_lon, altitude;
                    char ns, ew;
                    int fix_quality, num_sats;

                    // 2. Usa sscanf para extrair os valores da string
                    // sscanf é como um "printf reverso", ele lê de uma string
                    // para dentro de variáveis.
                    int parsed_items = sscanf(gps_buffer, 
                                            "$GPGGA,%f,%f,%c,%f,%c,%d,%d,%*f,%f",
                                            &utc_time, &nmea_lat, &ns, &nmea_lon, &ew, 
                                            &fix_quality, &num_sats, &altitude);
                    
                    // 3. Se os itens foram extraídos com sucesso, exibe-os
                    if (parsed_items >= 8) { // Queremos ao menos 8 itens
                        printf("----------------------------------------\n");

                        // --- Qualidade da Fixação e Satélites ---
                        printf("Qualidade da Fixacao: ");
                        switch(fix_quality) {
                            case 0: printf("Invalido\n"); break;
                            case 1: printf("GPS Fix (SPS)\n"); break;
                            case 2: printf("DGPS Fix\n"); break;
                            default: printf("N/A\n"); break;
                        }
                        printf("Satelites Encontrados: %d\n", num_sats);
                        
                        // Só exibe os outros dados se o sinal for válido
                        if (fix_quality > 0) {
                            // --- Tempo ---
                            int hour = (int)(utc_time / 10000);
                            int min = ((int)utc_time / 100) % 100;
                            int sec = (int)utc_time % 100;
                            // Ajuste para Fuso Horário do Brasil (UTC-3)
                            hour -= 3;
                            if (hour < 0) {
                                hour += 24; // Volta um dia
                            }
                            printf("Hora Local (BRT): %02d:%02d:%02d\n", hour, min, sec);

                            // --- Coordenadas ---
                            double dec_lat, dec_lon;
                            convert_coords(nmea_lat, ns, &dec_lat);
                            convert_coords(nmea_lon, ew, &dec_lon);
                            printf("Latitude:  %.6f\n", dec_lat);
                            printf("Longitude: %.6f\n", dec_lon);

                            // --- Altitude ---
                            printf("Altitude: %.1f metros\n", altitude);
                        }
                    }
                }
                // --- FIM DA LÓGICA DE PARSING ---
                index = 0; // Reseta o buffer para a próxima sentença
            } else {
                if (index < sizeof(gps_buffer) - 1) {
                    gps_buffer[index++] = ch;
                }
            }
        }
    }
    return 0;
}