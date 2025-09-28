#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "rfm95_lora.h"

// Definições do display
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15

int main() {
    stdio_init_all();
    sleep_ms(2000); 
    printf("Iniciando Transmissor LoRa (TX)...\n");

    if (!lora_init()) {
        printf("Falha na comunicacao com o RFM95. Travando. ❌\n");
        while(1);
    }
    printf("Comunicacao com RFM95 OK! ✅\n");

    lora_set_power(17);

    int counter = 0;
    char message_buffer[50];

    while (1) {
        snprintf(message_buffer, sizeof(message_buffer), "Ola #%d", counter);
        
        lora_send_packet((uint8_t*)message_buffer, strlen(message_buffer));
        
        printf("Pacote enviado: '%s'\n", message_buffer);
        
        counter++;
        sleep_ms(5000);
    }

    return 0;
}