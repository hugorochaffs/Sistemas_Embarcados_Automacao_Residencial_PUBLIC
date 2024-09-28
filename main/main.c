#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "nvs_utils.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "variaveis_globais.h"
#if defined(CONFIG_WIFI_CONFIG_MANUAL)
#include "wifi.h"
#else
#include "wifi_manager.h"
static TaskHandle_t xTaskConectadoWifi = NULL;
#endif
#include "mqtt.h"

#if defined(CONFIG_USE_GAS_SENSOR) || defined(CONFIG_USE_FIRE_SENSOR)
#include "driver/rtc_io.h"
#include "disaster_prevention.h"
#include "esp32/rom/uart.h"
#define GAS_SENSOR CONFIG_GAS_SENSOR_PIN
#define FIRE_SENSOR CONFIG_FIRE_SENSOR_PIN
static int sensor_de_gas = 1;
static int sensor_de_fogo = 0;
#endif

#if defined(CONFIG_USE_OLED)
#include "oled.h"
#include "driver/ledc.h"
#include <adc_module.h>
static bool fade_installed = false;
TaskHandle_t xHandleLed = NULL;
#define BOTAO CONFIG_PLACA_OLED_PINO_BOTAO
#define LED CONFIG_PLACA_OLED_LED
#define BUZZER CONFIG_PLACA_OLED_BUZZER
#define PORCENT_LIG_LAMP CONFIG_PORCENTAGEM_LUM_LIGA_LAMP
#endif

#if defined(CONFIG_USE_RELAYS)
#include <lampada.h>
#define BOTAO CONFIG_PLACA_RELAYS_BOTAO

#endif

#if defined(CONFIG_USE_LUM_TEMP_HUM_SENSOR)
#include <dht11.h>
int count = 0;
int tempBuff = 0;
int umidBuff = 0;
#endif

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;
QueueHandle_t filaDeInterrupcao;

static void IRAM_ATTR gpio_isr_handler(void *args)
{
  int pino = (int)args;
  xQueueSendFromISR(filaDeInterrupcao, &pino, NULL);
}

#define TAG "MAIN"
void conectadoWifi(void *params)
{
  while (true)
  {
    if (xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
    {
      // Processamento Internet
      mqtt_start();
    }
  }
}
#ifndef CONFIG_WIFI_CONFIG_MANUAL
void conectadoWifiManager(void *params)
{
  if (xTaskConectadoWifi == NULL)
  {
    // Cria a tarefa se ela não existir
    xSemaphoreGive(conexaoWifiSemaphore);
    xTaskCreate(&conectadoWifi, "Conexão ao MQTT", 4096, NULL, 1, &xTaskConectadoWifi);
  }
}
#endif

#ifdef CONFIG_USE_OLED

void trataComunicacaoComServidor(void *params) // TRATAMENTO DE COMUNICACAO PLACA OLED (HUGO) ->ENVIO(PUB)
{
  char mensagem[50];
  char msg[250];
  char msg2[100];
  if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    while (true)
    {

      sprintf(msg, "{ \"sinalLuminoso\": %d, \"buzzer\": %d, \"alarmeStatus\": %d, \"alrmDisp\": %d, \"temperatura\": %d, \"umidade\": %d, \"lampada\": %s, \"geral\": %s, \"stAlarmeOled\": %s}", sinalLuminoso, buzzer, alarmeStatus, alrmDisp, temperatura, umidade, lampada ? "ON" : "OFF", geral ? "ON" : "OFF", alarmeStatus ? "ON" : "OFF");
      sprintf(msg2, "{ \"luminosidade\": %d, \"pLuminosidade\": %d}", luminosidade, pLuminosidade);

      mqtt_envia_mensagem("v1/devices/me/attributes", msg, 1); // ENVIA TEMP PARA O BROKER 1 (estou testando em apenas um broker, por isso o final /1 e /2 para diferenciar)
      mqtt_envia_mensagem("v1/devices/me/telemetry", msg2, 1);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}

#elif defined(CONFIG_USE_RELAYS)
void trataComunicacaoComServidor(void *params) // TRATAMENTO DE COMUNICACAO PLACA RELAYS (JACKES) ->ENVIO(PUB)
{
  char mensagem[50];
  char msg[250];
  if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    while (true)
    {
      int envLampada = !lampada; // OS ESTADOS DO RELE SAO 0 - ON e 1- OFF
      int envGeral = !geral;

      sprintf(msg, "{ \"lampada\": %d, \"geral\": %d}", envLampada, envGeral);

      mqtt_envia_mensagem("v1/devices/me/attributes", msg, 1);               // ENVIA TEMP PARA O BROKER 1 (estou testando em apenas um broker, por isso o final /1 e /2 para diferenciar)
      mqtt_envia_mensagem("FSEAH2024/home-automation/relay/status", msg, 2); // ENVIA TEMP PARA O BROKER 2
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}

#elif defined(CONFIG_USE_LUM_TEMP_HUM_SENSOR)
void trataComunicacaoComServidor(void *params) // TRATAMENTO DE COMUNICACAO PLACA RELAYS (JACKES) ->ENVIO(PUB)
{
  char msg[250];
  if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    while (true)
    {

      sprintf(msg, "{\"temperature\": %d, \"humidity\": %d}", temperatura, umidade);

      mqtt_envia_mensagem("v1/devices/me/telemetry", msg, 1);                                   // ENVIA TEMP PARA O BROKER 1 (estou testando em apenas um broker, por isso o final /1 e /2 para diferenciar)
      mqtt_envia_mensagem("FSEAH2024/home-automation/luminosity-temperature-humidity", msg, 2); // ENVIA TEMP PARA O BROKER 2
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}

#else
void trataComunicacaoComServidor(void *params) // COLOCAR O SEU COMO ELSE IF E DEIXAR ESSE COMO ELSE
{
  char mensagem[50];
  if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    while (true)
    {
      float temperatura = 20.0 + (float)rand() / (float)(RAND_MAX / 10.0);
      sprintf(mensagem, "temperatura1: %f", temperatura);
      mqtt_envia_mensagem("sensores/temperatura/1", mensagem, 1); // ENVIA TEMP PARA O BROKER 1 (estou testando em apenas um broker, por isso o final /1 e /2 para diferenciar)
      mqtt_envia_mensagem("sensores/temperatura/2", mensagem, 2); // ENVIA TEMP PARA O BROKER 2
      vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
  }
}
#endif

#ifdef CONFIG_USE_OLED
void taskEMode()
{
  if (!fade_installed)
  {
    ledc_fade_func_install(0);
    fade_installed = true;
  }
  while (true)
  {
    if (sinalLuminoso)
    {
      ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 1000, LEDC_FADE_WAIT_DONE);
      ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 255, 1000, LEDC_FADE_WAIT_DONE);
    }
    else
    {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void trata_alarme(void *params)
{
  init_oled();
  ESP_LOGI(TAG, "OLED está ativo");
  while (true)
  {
    update_oled();
    if (alrmDisp)
    {
      sinalLuminoso = 1;
      buzzer = 1;
      gpio_set_level(BUZZER, 1);
    }
    else
    {
      sinalLuminoso = 0;
      buzzer = 0;
      gpio_set_level(BUZZER, 0);
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

#endif

void trataInterrupcaoBotao(void *params)
{
  int pino;
  int contador = 0;

  while (true)
  {
    if (xQueueReceive(filaDeInterrupcao, &pino, portMAX_DELAY))
    {
      // De-bouncing
      int estado = gpio_get_level(pino);
      if (estado == 1)
      {
        gpio_isr_handler_remove(pino);
        while (gpio_get_level(pino) == estado)
        {
          vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        contador++;
        printf("Os botões foram acionados %d vezes. Botão: %d\n", contador, pino);
#ifdef CONFIG_USE_OLED
        if (alarmeStatus == 1 && alrmDisp == 1)
        {
          alrmDisp = 0;
          alarmeStatus = 1;
          save_variable("storage", "alrmDisp", 0);
          save_variable("storage", "calrmDisp", 0);
        }
        else if (alarmeStatus == 1)
        {
          save_variable("storage", "alarmeStatus", 0);
          save_variable("storage", "calrmDisp", 0);
          alarmeStatus = 0;
        }
        else if (alarmeStatus == 0)
        {
          alarmeStatus = 1;
          save_variable("storage", "alarmeStatus", 1);
          save_variable("storage", "calrmDisp", 0);
        }
#endif

#ifdef CONFIG_USE_RELAYS

        if (lampada == 0)
        {
          relay_set_state(1, 1);
        }
        else
        {
          relay_set_state(1, 0);
        }

#endif

        // Habilitar novamente a interrupção
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_isr_handler_add(pino, gpio_isr_handler, (void *)pino);
      }
    }
  }
}

#if defined(CONFIG_USE_OLED)
int calibrate_lux(int adc_value)
{
  // Considerando que o ADC tem 4096 níveis (12 bits) e a tensão máxima é de aproximadamente 3.3V
  // com atenuação de 12 dB, o valor da tensão de referência (Vref) será próximo de 3.9V.

  // Calcular a tensão correspondente ao valor lido no ADC
  float v_out = (adc_value / 4095.0) * 3.9; // Ajustando para a tensão de referência correta

  // Ajustar a escala e o offset para a calibração de lux
  // Sabendo que 2.5V corresponde a aproximadamente 120 lux, ajustamos a escala
  float escala = 120.0 / 2.5; // Escala = 120 lux / 2.5V -> 48 lux/V
  float offset = 0.0;         // Não há necessidade de offset se 0V corresponder a 0 lux

  // Calcular lux a partir da tensão medida
  float lux = (v_out * escala) + offset;

  // Retornar o valor de lux como inteiro, arredondado
  return (int)(lux + 0.5);
}

void ldr_task()
{
  adc_channel_t channel;
  int adc_min = 0;
  int adc_max = 4096;

#if CONFIG_MY_ADC_CHANNEL_0
  channel = ADC_CHANNEL_0;
#elif CONFIG_MY_ADC_CHANNEL_1
  channel = ADC_CHANNEL_1;
#elif CONFIG_MY_ADC_CHANNEL_2
  channel = ADC_CHANNEL_2;
#elif CONFIG_MY_ADC_CHANNEL_3
  channel = ADC_CHANNEL_3;
#elif CONFIG_MY_ADC_CHANNEL_4
  channel = ADC_CHANNEL_4;
#elif CONFIG_MY_ADC_CHANNEL_5
  channel = ADC_CHANNEL_5;
#elif CONFIG_MY_ADC_CHANNEL_6
  channel = ADC_CHANNEL_6;
#elif CONFIG_MY_ADC_CHANNEL_7
  channel = ADC_CHANNEL_7;
#elif CONFIG_MY_ADC_CHANNEL_8
  channel = ADC_CHANNEL_8;
#elif CONFIG_MY_ADC_CHANNEL_9
  channel = ADC_CHANNEL_9;
#endif

  adc_init(ADC_UNIT_1);    // Inicializa ADC1
  adc_config_pin(channel); // Configura o pino LDR

  while (1)
  {
    int sum = 0;
    int num_readings = 10;

    for (int i = 0; i < num_readings; i++)
    {
      int ldr_value = analogRead(channel); // Leitura do LDR
      sum += ldr_value;
      vTaskDelay(100 / portTICK_PERIOD_MS); // Intervalo de 100 ms entre leituras
    }

    int ldr_avg = sum / num_readings; // Calcula a média
    pLuminosidade = ((ldr_avg - adc_min) * 100) / (adc_max - adc_min);

    int lx = calibrate_lux(ldr_avg);
    luminosidade = lx;

    if (pLuminosidade < PORCENT_LIG_LAMP && lampada == 0)
    {
      char msgl[20];
      sprintf(msgl, "{true}");
      mqtt_envia_mensagem("FSEAH2024/home-automation/relay/set/lampada", msgl, 2); // ENVIA COMANDO LIGAR LAMP
    }
    if (pLuminosidade > PORCENT_LIG_LAMP + 10 && lampada == 1)
    {
      char msgd[20];
      sprintf(msgd, "{false}");
      mqtt_envia_mensagem("FSEAH2024/home-automation/relay/set/lampada", msgd, 2); // ENVIA COMANDO LIGAR LAMP

      vTaskDelay(1000 / portTICK_PERIOD_MS); // Espera 1 segundo antes da próxima média
    }
  }

  adc_deinit();
}
#endif

#if defined(CONFIG_USE_LUM_TEMP_HUM_SENSOR)

void dht_task()
{
  DHT11_init(CONFIG_TEMPERATURE_HUMIDITY_SENSOR_PIN);

  while (1)
  {
    struct dht11_reading data = DHT11_read();

    if (data.status == DHT11_OK)
    {
      if (count <= 10)
      {
        tempBuff = tempBuff + data.temperature;
        umidBuff = umidBuff + data.humidity;
        count = count + 1;
      }
      if (count >= 10)
      {
        temperatura = tempBuff / count;
        umidade = umidBuff / count;
        tempBuff = 0;
        umidBuff = 0;
        count = 0;
      }

      // printf("Temperatura: %d°C, Umidade: %d%%\n", data.temperature, data.humidity);
    }
    else if (data.status == DHT11_TIMEOUT_ERROR)
    {
      printf("Erro: Timeout na leitura do sensor DHT11.\n");
    }
    else if (data.status == DHT11_CRC_ERROR)
    {
      printf("Erro: CRC incorreto na leitura do sensor DHT11.\n");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // Aguarde 2 segundos entre as leituras
  }

  vTaskDelay(2000 / portTICK_PERIOD_MS); // Aguarde 2 segundos entre as leituras
}
#endif

#if defined(CONFIG_USE_GAS_SENSOR) || defined(CONFIG_USE_FIRE_SENSOR)
void deep_sleep_mode()
{
  configure_sensors();
  switch (esp_sleep_get_wakeup_cause())
  {

  case ESP_SLEEP_WAKEUP_EXT1:
  {
    uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_pin_mask != 0)
    {
      int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
      printf("Wake up from GPIO %d\n", pin);
    }
    else
    {
      printf("Wake up from GPIO\n");
    }
    break;
  }
  default:
  {
    break;
  }
  }
  if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    while (sensor_de_fogo = read_fire_sensor(), sensor_de_gas = read_gas_sensor(), sensor_de_fogo != 0 || sensor_de_gas != 1)
    {
      ESP_LOGI(TAG, "Sensor de fogo leu valor %d\n", sensor_de_fogo);
      ESP_LOGI(TAG, "Leu sensor de Gas: %d\n", sensor_de_gas);

      if (sensor_de_gas == 0)
      {
        char mensagem[50];
        sprintf(mensagem, "{\"hasLeak\":true}");
        ESP_LOGI(TAG, "Enviando informações do sensor de gás para o servidor");
        mqtt_envia_mensagem("v1/devices/me/attributes", mensagem, 1);
        mqtt_envia_mensagem("FSEAH2024/home-automation/disaster-prevention/gas-leak", mensagem, 2);
        mqtt_envia_mensagem("FSEAH2024/home-automation/relay/set/geral", mensagem, 2);
      }

      if (sensor_de_fogo == 1)
      {
        char mensagem[50];
        sprintf(mensagem, "{\"hasFire\":true}");
        ESP_LOGI(TAG, "Enviando informações do sensor de fogo para o servidor");
        mqtt_envia_mensagem("v1/devices/me/attributes", mensagem, 1);
        mqtt_envia_mensagem("FSEAH2024/home-automation/disaster-prevention/fire", mensagem, 2);
      }
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }
  }
  char *deactivate_gas_sensor_msg = "{\"hasLeak\":false}";

  mqtt_envia_mensagem("v1/devices/me/attributes", deactivate_gas_sensor_msg, 1);
  mqtt_envia_mensagem("FSEAH2024/home-automation/disaster-prevention/gas-leak", deactivate_gas_sensor_msg, 2);
  mqtt_envia_mensagem("FSEAH2024/home-automation/relay/set/geral", deactivate_gas_sensor_msg, 2);

  char *deactivate_fire_sensor_msg = "{\"hasFire\":false}";
  mqtt_envia_mensagem("v1/devices/me/attributes", deactivate_fire_sensor_msg, 1);
  mqtt_envia_mensagem("FSEAH2024/home-automation/disaster-prevention/fire", deactivate_fire_sensor_msg, 2);

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  ESP_LOGI(TAG, "Entrando em modo deep sleep.");
  uart_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);
  esp_deep_sleep_start();
}
#endif
void app_main(void)
{
  conexaoWifiSemaphore = xSemaphoreCreateBinary();
  conexaoMQTTSemaphore = xSemaphoreCreateBinary();
  // Inicializa o NVS
  init_nvs();

  // FUNCOES ABAIXO SERVEM PARA LER A NVS E SALVAR NA VARIAVEL GLOBAL
#ifdef CONFIG_USE_OLED
  int ad = 0;
  esp_err_t result = read_variable_if_exists("storage", "alrmDisp", &ad);
  if (result == ESP_OK)
  {
    // printf("Value read: %d\n", value);
    alrmDisp = ad;
  }
  else
  {
    alrmDisp = 0;
  }

  if (alrmDisp == 1)
  {
    printf("FOGOOOOOOO!!!!!!!!");
  }

  int as = 0;
  esp_err_t result1 = read_variable_if_exists("storage", "alarmeStatus", &as);
  if (result1 == ESP_OK)
  {
    // printf("Value read: %d\n", value);
    alarmeStatus = as;
  }
  else
  {
    alarmeStatus = 1;
  }

  int cad = 0;
  esp_err_t result2 = read_variable_if_exists("storage", "calrmDisp", &cad);
  if (result2 == ESP_OK)
  {
    // printf("Value read: %d\n", value);
    calrmDisp = cad;
  }
  else
  {
    calrmDisp = 0;
  }

#endif

#ifdef CONFIG_USE_RELAYS

  int slamp = 0;
  esp_err_t resultSlamp = read_variable_if_exists("storage", "slamp", &slamp);
  if (resultSlamp == ESP_OK)
  {
    // printf("Value read: %d\n", value);
    relay_set_state(1, slamp);
  }
  else
  {
    relay_set_state(1, 1);
  }

  int sgeral = 0;
  esp_err_t resultSgeral = read_variable_if_exists("storage", "slamp", &sgeral);
  if (resultSgeral == ESP_OK)
  {
    // printf("Value read: %d\n", value);
    relay_set_state(2, sgeral);
  }
  else
  {
    relay_set_state(2, 0);
  }

  relay_init();

#endif

  conexaoWifiSemaphore = xSemaphoreCreateBinary();
  conexaoMQTTSemaphore = xSemaphoreCreateBinary();

#if defined(CONFIG_WIFI_CONFIG_MANUAL)
  wifi_start();
  xTaskCreate(&conectadoWifi, "Conexão ao MQTT", 4096, NULL, 1, NULL);
#else
  wifi_manager_start();
  wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &conectadoWifiManager);

#endif

  xTaskCreate(&trataComunicacaoComServidor, "Comunicação com Broker", 4096, NULL, 1, NULL);

#if defined(CONFIG_USE_GAS_SENSOR) || defined(CONFIG_USE_FIRE_SENSOR)
  deep_sleep_mode();
#endif

#ifdef CONFIG_USE_OLED
  // TIMER DO LED PWM
  ledc_timer_config_t timer_config = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_8_BIT,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = 1000,
      .clk_cfg = LEDC_AUTO_CLK};
  ledc_timer_config(&timer_config);
  // CONFIG CANAL PWM LED
  ledc_channel_config_t channel_config = {
      .gpio_num = LED,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0,
      .hpoint = 0};
  ledc_channel_config(&channel_config);
  xTaskCreate(&taskEMode, "LUZ DE EMERGENCIA", 2048, NULL, 1, &xHandleLed);

  xTaskCreate(&trata_alarme, "oled", 4096, NULL, 1, NULL);
  esp_rom_gpio_pad_select_gpio(BOTAO);
  esp_rom_gpio_pad_select_gpio(BUZZER);
  gpio_set_direction(BOTAO, GPIO_MODE_INPUT);
  gpio_set_direction(BUZZER, GPIO_MODE_OUTPUT);
  gpio_pulldown_en(BOTAO);
  gpio_pullup_dis(BOTAO);
  gpio_set_intr_type(BOTAO, GPIO_INTR_POSEDGE);
  filaDeInterrupcao = xQueueCreate(10, sizeof(int));
  xTaskCreate(trataInterrupcaoBotao, "TrataBotao", 2048, NULL, 1, NULL);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(BOTAO, gpio_isr_handler, (void *)BOTAO);
  xTaskCreate(&ldr_task, "ldr_task", 2048, NULL, 5, NULL);

#elif defined(CONFIG_USE_RELAYS)
  gpio_set_intr_type(BOTAO, GPIO_INTR_POSEDGE);
  filaDeInterrupcao = xQueueCreate(10, sizeof(int));
  xTaskCreate(trataInterrupcaoBotao, "TrataBotao", 2048, NULL, 1, NULL);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(BOTAO, gpio_isr_handler, (void *)BOTAO);

#elif defined(CONFIG_USE_LUM_TEMP_HUM_SENSOR)
  xTaskCreate(&dht_task, "dht_task", 2048, NULL, 5, NULL);

#else
  ESP_LOGI(TAG, "oled não foi ativado");
#endif
}
