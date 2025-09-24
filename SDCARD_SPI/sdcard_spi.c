#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"

#define PIN_MISO     16
#define PIN_MOSI     19
#define PIN_SCK      18
#define PIN_CS       17

FATFS fs;
FIL file;

// === Inicializa cartão SD via SPI ===
void inicializar_sd() {
    spi_init(spi0, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("Erro ao montar SD: %d\n", fr);
    } else {
        printf("Cartão SD montado com sucesso.\n");
        fr = f_open(&file, "bitdoglab.txt", FA_WRITE | FA_OPEN_APPEND);
        if (fr != FR_OK) {
            printf("Erro ao abrir arquivo: %d\n", fr);
        } else {
            f_puts("Evento,Timestamp\n", &file);
            f_sync(&file);
        }
    }
}

// === Registra evento no SD com timestamp ===
void registrar_evento(const char* evento) {
    char linha[64];
    snprintf(linha, sizeof(linha), "%s,%lu\n", evento, to_ms_since_boot(get_absolute_time()));
    UINT bw;
    FRESULT fr = f_write(&file, linha, strlen(linha), &bw);
    if (fr != FR_OK) {
        printf("Erro ao escrever no arquivo: %d\n", fr);
    } else {
        f_sync(&file);
        printf("Evento registrado: %s", linha);
    }
}



int c;
char evento[64] = {0};
// === Função principal ===
int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(100);

    // Inicializa cartão SD
    inicializar_sd();

    // === Loop principal ===
    while (true) {
        sprintf(evento, "Evento: %d!", c++);
        registrar_evento(evento);

        sleep_ms(5000);
    }

    return 0;
}