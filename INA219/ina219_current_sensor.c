#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ina219/ina219.h"

// Definição dos pinos I2C
#define I2C_PORT i2c0
#define I2C_SDA_PIN 0
#define I2C_SCL_PIN 1

int main() {
    stdio_init_all();
    sleep_ms(3000); // Mais tempo para conectar o monitor
    printf("--- INICIANDO DIAGNOSTICO INA219 ---\n");

    // Inicializa o I2C
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializa o sensor INA219
    if (!ina219_init(I2C_PORT, 0x40)) {
        printf("!!! ERRO FATAL: Sensor INA219 nao encontrado no barramento I2C.\n");
        printf("!!! VERIFIQUE A FIACAO: VCC, GND, SDA, SCL.\n");
        while (1);
    } else {
        printf("--- SUCESSO: Sensor INA219 encontrado e comunicando via I2C.\n");
    }

    // Configura o sensor
    ina219_configure(INA219_RANGE_16V, INA219_GAIN_320MV, INA219_BUS_ADC_12BIT, INA219_SHUNT_ADC_128S, INA219_MODE_SHUNT_BUS_CONTINUOUS);
    printf("--- SENSOR CONFIGURADO. Iniciando leituras...\n\n");

    while (1) {
        float bus_voltage = ina219_get_bus_voltage();
        float shunt_voltage_mv = ina219_get_shunt_voltage_mv();
        float current_ma = ina219_get_current_ma();

        printf("--------------------------------\n");
        printf("Tensao do Barramento (Bus): %.2f V\n", bus_voltage);

        // PONTO DE DECISAO CRITICO:
        if (bus_voltage < 1.0) {
            printf("!!! ATENCAO: A Tensao de Barramento esta em 0V ou proxima de 0V.\n");
            printf("!!! Isso indica um PROBLEMA FISICO na fiacao.\n");
            printf("!!! Verifique 100%% se o fio do GND da fonte externa esta ligado no GND da Pico.\n");
            printf("!!! Verifique 100%% se o fio de 12V(+) da fonte esta ligado no Vin+ do sensor.\n");
        }

        printf("Tensao do Shunt: %.2f mV\n", shunt_voltage_mv);
        printf("Corrente: %.2f mA\n", current_ma);

        sleep_ms(2000); // Aumentei o tempo para facilitar a leitura
    }

    return 0;
}