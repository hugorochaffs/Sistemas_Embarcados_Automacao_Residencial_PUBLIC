
#ifndef NVS_UTILS_H
#define NVS_UTILS_H

#include <nvs_flash.h>
#include <nvs.h>

void init_nvs(void);
void save_variable(const char* namespace_name, const char* key, int value);
void load_variable(const char* namespace_name, const char* key, int* value);
bool key_exists(const char* namespace_name, const char* key);
esp_err_t read_variable_if_exists(const char* namespace_name, const char* key, int* value);

#endif // NVS_UTILS_H
