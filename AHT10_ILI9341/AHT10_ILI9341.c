#include "pico/stdlib.h"
#include "ili9341.h"
#include "gfx.h"
#include "aht10.h"
#include <stdio.h>

#define HUMIDITY_THRESHOLD 70.0f
#define TEMP_THRESHOLD     20.0f

int main() {
    // Inicialização da tela
    LCD_initDisplay();
    LCD_setRotation(1);
    GFX_createFramebuf();

    // Inicialização do sensor AHT10
    aht10_init();

    // Configurações de texto da tela
    GFX_setTextSize(3);

    // Variável para guardar os dados do sensor
    aht10_reading sensor_data;

    while (true) {
        GFX_clearScreen();

        // Tenta ler os dados do sensor
        if (aht10_read_data(&sensor_data)) {
            
            GFX_setCursor(10, 20);
            GFX_setTextColor(ILI9341_YELLOW);
            GFX_printf("Temp: %.1f C", sensor_data.temperature);
            
            GFX_setCursor(10, 80);
            GFX_setTextColor(ILI9341_CYAN);
            GFX_printf("Umid: %.1f %%", sensor_data.humidity);

            // 1. Verifica se a temperatura está abaixo do limite
            if (sensor_data.temperature < TEMP_THRESHOLD) {
                GFX_setCursor(10, 150);
                GFX_setTextColor(ILI9341_RED); // Cor do alerta: Vermelho
                GFX_setTextSize(2); 
                GFX_printf("ALERTA: TEMP. BAIXA!");
            }

            // 2. Verifica se a umidade está acima do limite
            if (sensor_data.humidity > HUMIDITY_THRESHOLD) {
                GFX_setCursor(10, 180);
                GFX_setTextColor(ILI9341_RED);
                GFX_setTextSize(2);
                GFX_printf("ALERTA: UMID. ALTA!");
            }

            GFX_setTextSize(3);

        } else {
            GFX_setCursor(10, 20);
            GFX_setTextColor(ILI9341_RED);
            GFX_printf("Sensor ocupado...");
        }

        GFX_flush();
        sleep_ms(5000);
    }
}