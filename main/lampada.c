#include "lampada.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_utils.h"
#include "variaveis_globais.h"

static const char *TAG = "Lampada";
#define RELAY_1_GPIO CONFIG_PLACA_RELAYS_PINO_LAMPADA
#define RELAY_2_GPIO CONFIG_PLACA_RELAYS_PINO_GERAL

// Função para inicializar os pinos dos relés
void relay_init(void)
{
    esp_rom_gpio_pad_select_gpio(RELAY_1_GPIO);
    esp_rom_gpio_pad_select_gpio(RELAY_2_GPIO);
    gpio_set_direction(RELAY_1_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RELAY_2_GPIO, GPIO_MODE_OUTPUT);

    // Definindo status dos reles conforme a ultima utilizacao (NVS)
    relay_set_state(1, lampada);
    relay_set_state(2, geral);

    ESP_LOGI(TAG, "Relays initialized");
}

// Função para controlar o estado de um relé
void relay_set_state(int relay, int state)
{
    if (relay == 1)
    {
        gpio_set_level(RELAY_1_GPIO, state);
        save_variable("storage", "slamp", state);
        lampada = state;
        ESP_LOGI(TAG, "Relay 1 set to %d", state);
    }
    else if (relay == 2)
    {
        gpio_set_level(RELAY_2_GPIO, state);
        save_variable("storage", "sgeral", state);
        geral = state;
        ESP_LOGI(TAG, "Relay 2 set to %d", state);
    }
}
