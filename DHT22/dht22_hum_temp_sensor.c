#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "dht.h"

const uint DHT_PIN = 8;

int main() {
    // Inicializa a comunicação serial via USB
    stdio_init_all();
    sleep_ms(2000); // Um tempo extra para o monitor serial conectar

    printf("[MAIN] Programa iniciado.\n");

    // Inicializa o chip CYW43 para controlar o LED
    if (cyw43_arch_init()) {
        printf("[MAIN] ERRO: Falha ao inicializar cyw43_arch.\n");
        return -1;
    }
    printf("[MAIN] cyw43_arch inicializado com sucesso.\n");

    printf("[MAIN] Antes de chamar dht_init()...\n");
    dht_init(DHT_PIN);
    printf("[MAIN] Depois de chamar dht_init().\n");

    dht_reading leitura;
    bool led_status = false;

    while (1) {
        printf("\n[MAIN] ---- Inicio do loop ----\n");

        // Pisca o LED para mostrar que o programa está rodando
        led_status = !led_status;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_status);
        printf("[MAIN] LED status: %d\n", led_status);

        printf("[MAIN] Antes de chamar dht_read()...\n");
        if (dht_read(&leitura)) {
            printf("[MAIN] dht_read() retornou SUCESSO.\n");
            printf("[MAIN] Umidade: %.1f%%, Temperatura: %.1f C\n", leitura.humidity, leitura.temp_celsius);
        } else {
            printf("[MAIN] dht_read() retornou FALHA.\n");
        }
        printf("[MAIN] Depois de chamar dht_read().\n");

        printf("[MAIN] Aguardando 2 segundos...\n");
        sleep_ms(2000);
    }

    return 0;
}