#include "esp_event.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include "disaster_prevention.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"

#define GAS_SENSOR CONFIG_GAS_SENSOR_PIN
#define FIRE_SENSOR CONFIG_FIRE_SENSOR_PIN
#define TAG "disaster_prevention"
void init_gas_sensor()
{
  gpio_set_direction(GAS_SENSOR, GPIO_MODE_INPUT);

  rtc_gpio_pullup_en(GAS_SENSOR);
  rtc_gpio_pulldown_dis(GAS_SENSOR);
}

int read_gas_sensor()
{
  int leitura_sensor_gas = gpio_get_level(GAS_SENSOR);
  ESP_LOGI(TAG, "Sensor de gas: %d", leitura_sensor_gas);
  return leitura_sensor_gas;
}

void init_fire_sensor()
{
  gpio_set_direction(FIRE_SENSOR, GPIO_MODE_INPUT);
  rtc_gpio_pulldown_en(FIRE_SENSOR);
  rtc_gpio_pullup_dis(FIRE_SENSOR);
}

void configure_sensors()
{
  init_fire_sensor();
  init_gas_sensor();
  const uint64_t ext_wakeup_fire_sensor_mask = 1ULL << FIRE_SENSOR;
  // const uint64_t ext_wakeup_gas_sensor_mask = 1ULL << GAS_SENSOR;
  esp_sleep_enable_ext0_wakeup(GAS_SENSOR, 0);
  esp_sleep_enable_ext1_wakeup_io(ext_wakeup_fire_sensor_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
  // esp_sleep_enable_ext1_wakeup_io(ext_wakeup_gas_sensor_mask, ESP_EXT1_WAKEUP_ALL_LOW);
}

int read_fire_sensor()
{
  int leitura_sensor_fogo = gpio_get_level(FIRE_SENSOR);
  ESP_LOGI(TAG, "Sensor de fogo: %d", leitura_sensor_fogo);
  return leitura_sensor_fogo;
}