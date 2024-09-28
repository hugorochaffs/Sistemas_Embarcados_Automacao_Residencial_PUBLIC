#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "variaveis_globais.h"
#include "oled.h"
#include "ssd1306.h"
#include "font8x8_basic.h"

#define tag "OLED"

SSD1306_t dev;
int top, center, bottom;
char lineChar[20];

void init_oled() {
    ESP_LOGI(tag, "INTERFACE is i2c");
	ESP_LOGI(tag, "CONFIG_SDA_GPIO=%d", CONFIG_SDA_GPIO);
	ESP_LOGI(tag, "CONFIG_SCL_GPIO=%d", CONFIG_SCL_GPIO);
	ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d", CONFIG_RESET_GPIO);
	i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);

    #if CONFIG_FLIP
	dev._flip = true;
	ESP_LOGW(tag, "Flip upside down");
    #endif

    ESP_LOGI(tag, "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);
}

void update_oled() {
    if (alrmDisp) {
        displayModoEmergencia();
    } else {
        displayInfoGeral();
    }
}

void displayInfoGeral() {
    ssd1306_clear_screen(&dev, false);
	

    top = 0;
    center = 1;
    bottom = 3;

    // Exibe o status do alarme
    snprintf(lineChar, sizeof(lineChar), "Alarme: %s", alarmeStatus ? "ON" : "OFF");
    ssd1306_display_text(&dev, top, lineChar, strlen(lineChar), false);

    // Exibe temperatura e umidade
    snprintf(lineChar, sizeof(lineChar), "Temp: %dC", temperatura);
    ssd1306_display_text(&dev, top + 2, lineChar, strlen(lineChar), false);

    snprintf(lineChar, sizeof(lineChar), "Umid: %d%%", umidade);
    ssd1306_display_text(&dev, top + 3, lineChar, strlen(lineChar), false);

	 snprintf(lineChar, sizeof(lineChar), "Lum: %d %%", pLuminosidade);
    ssd1306_display_text(&dev, top + 4, lineChar, strlen(lineChar), false);

    // Exibe o status dos relés
    snprintf(lineChar, sizeof(lineChar), "Lamp: %s", lampada ? "ON" : "OFF");
    ssd1306_display_text(&dev, top + 5, lineChar, strlen(lineChar), false);

    snprintf(lineChar, sizeof(lineChar), "Geral: %s", geral ? "ON" : "OFF");
    ssd1306_display_text(&dev, top + 6, lineChar, strlen(lineChar), false);
}

void displayModoEmergencia() {
    ssd1306_clear_screen(&dev, false);

    center = 3;  // Posiciona o texto no centro da tela

    if (calrmDisp == 1) {
        ssd1306_display_text_x3(&dev, center, "GAS", 3, true);
    } else if (calrmDisp == 2) {
        ssd1306_display_text_x3(&dev, center, "FOGO", 4, true);
    }
	else if (calrmDisp == 3) {
        ssd1306_display_text_x3(&dev, center, "GAS/FOGO", 9, true);
    }
}
