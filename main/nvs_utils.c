// nvs_utils.c
#include "nvs_utils.h"
#include <stdio.h>

void init_nvs() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void save_variable(const char* namespace_name, const char* key, int value) {
    nvs_handle my_handle;
    esp_err_t err;

    err = nvs_open(namespace_name, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        err = nvs_set_i32(my_handle, key, value);
        if (err != ESP_OK) {
            printf("Error (%s) setting value!\n", esp_err_to_name(err));
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK) {
            printf("Error (%s) committing value!\n", esp_err_to_name(err));
        }

        nvs_close(my_handle);
    }
}

void load_variable(const char* namespace_name, const char* key, int* value) {
    nvs_handle my_handle;
    esp_err_t err;

    err = nvs_open(namespace_name, NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        err = nvs_get_i32(my_handle, key, value);
        if (err == ESP_OK) {
            printf("Value read: %d\n", *value);
        } else if (err == ESP_ERR_NVS_NOT_FOUND) {
            printf("The value is not initialized yet!\n");
        } else {
            printf("Error (%s) reading value!\n", esp_err_to_name(err));
        }

        nvs_close(my_handle);
    }
}
bool key_exists(const char* namespace_name, const char* key) {
    nvs_handle my_handle;
    esp_err_t err;
    int value;

    // Abre o namespace NVS em modo leitura
    err = nvs_open(namespace_name, NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return false;
    }

    // Tenta ler o valor da chave
    err = nvs_get_i32(my_handle, key, &value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // A chave não foi encontrada
        nvs_close(my_handle);
        return false;
    } else if (err != ESP_OK) {
        // Outro erro ao ler o valor
        printf("Error (%s) reading value!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }

    // Fecha o namespace NVS
    nvs_close(my_handle);
    return true;
}

esp_err_t read_variable_if_exists(const char* namespace_name, const char* key, int* value) {
    nvs_handle my_handle;
    esp_err_t err;

    if (key_exists(namespace_name, key)) {
        // A chave existe, agora leia o valor
        err = nvs_open(namespace_name, NVS_READONLY, &my_handle);
        if (err != ESP_OK) {
            printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
            return err;
        }

        err = nvs_get_i32(my_handle, key, value);
        if (err != ESP_OK) {
            printf("Error (%s) reading value!\n", esp_err_to_name(err));
        }

        nvs_close(my_handle);
        return err;
    } else {
        return ESP_ERR_NVS_NOT_FOUND;
    }
}