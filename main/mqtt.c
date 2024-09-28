#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt.h"

#include "nvs_utils.h"

#include "variaveis_globais.h"

#define TAG "MQTT"

extern SemaphoreHandle_t conexaoMQTTSemaphore;

esp_mqtt_client_handle_t client1; // CLIENTE THINGSBOARD
esp_mqtt_client_handle_t client2; // CLIENTE MQTT GERAL COMUNICA ENTRE PLACAS

// Definição dos tópicos
const char *topics1[] = {
    "v1/devices/me/rpc/request/+"
};

#ifdef CONFIG_USE_OLED
const char *topics2[] = {
    "FSEAH2024/home-automation/disaster-prevention/gas-leak",
    "FSEAH2024/home-automation/disaster-prevention/fire",
    "FSEAH2024/home-automation/temperature-humidity",
    "FSEAH2024/home-automation/relay/status"
};

#elif defined(CONFIG_USE_RELAYS)
#include <lampada.h>
const char *topics2[] = {
    "FSEAH2024/home-automation/relay/set",
    "FSEAH2024/home-automation/relay/set/lampada",
    "FSEAH2024/home-automation/relay/set/geral"
};


#else
const char *topics2[] = {};
#endif

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, (int)event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        xSemaphoreGive(conexaoMQTTSemaphore);
        
        // Assinar tópicos após a conexão
        mqtt_subscribe_to_topics(client1, topics1, sizeof(topics1) / sizeof(topics1[0]));
        mqtt_subscribe_to_topics(client2, topics2, sizeof(topics2) / sizeof(topics2[0]));
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        handle_mqtt_event_data(event->topic, event->topic_len, event->data, event->data_len);
        //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        //printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_subscribe_to_topics(esp_mqtt_client_handle_t client, const char **topics, int num_topics)
{
    for (int i = 0; i < num_topics; i++) {
        int msg_id = esp_mqtt_client_subscribe(client, topics[i], 0);
        if (msg_id >= 0) {
            ESP_LOGI(TAG, "Subscribed to topic: %s, msg_id=%d", topics[i], msg_id);
        } else {
            ESP_LOGE(TAG, "Failed to subscribe to topic: %s", topics[i]);
        }
    }
}

void mqtt_start()
{
    char uri1[100];
    strcpy(uri1, "mqtt://"); // Copia "mqtt://" para o início do buffer
    strcat(uri1, CONFIG_ESP_MQTT_BROKER_THINGSBOARD); // Concatena o valor de ESP_MQTT_BROKER
    ESP_LOGI(TAG, "SERVIDOR MQTT THINGSBOARD=%s", uri1);

    // Configuração para o primeiro broker THINGSBOARD
    esp_mqtt_client_config_t mqtt_config1 = {
        .broker.address.uri = uri1,
        .credentials.username = CONFIG_ESP_MQTT_TOKEN_THINGSBOARD
    };
    client1 = esp_mqtt_client_init(&mqtt_config1);
    esp_mqtt_client_register_event(client1, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client1);

    char uri2[100];
    strcpy(uri2, "mqtt://"); // Copia "mqtt://" para o início do buffer
    strcat(uri2, CONFIG_ESP_MQTT_BROKER); // Concatena o valor de ESP_MQTT_BROKER
    ESP_LOGI(TAG, "SERVIDOR MQTT GERAL=%s", uri2);

    // Configuração para o segundo broker
    esp_mqtt_client_config_t mqtt_config2 = {
        .broker.address.uri = uri2
    };
    client2 = esp_mqtt_client_init(&mqtt_config2);
    esp_mqtt_client_register_event(client2, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client2);
}

void mqtt_envia_mensagem(char *topico, char *mensagem, int broker_id)
{
    int message_id;
    if (broker_id == 1) {
        message_id = esp_mqtt_client_publish(client1, topico, mensagem, 0, 1, 0);
        ESP_LOGI(TAG, "Mensagem enviada pelo Broker 1, ID: %d", message_id);
    } else if (broker_id == 2) {
        message_id = esp_mqtt_client_publish(client2, topico, mensagem, 0, 1, 0);
        ESP_LOGI(TAG, "Mensagem enviada pelo Broker 2, ID: %d", message_id);
    } else {
        ESP_LOGE(TAG, "Broker ID inválido");
    }
}

void handle_mqtt_event_data(const char *topic, int topic_len, const char *data, int data_len) {

    #ifdef CONFIG_USE_OLED /////////////////////////////////////////////////
    int request_id;
    char method[32];

    char topic_str[topic_len + 1];
    char data_str[data_len + 1];

    // Copia o tópico e os dados para buffers temporários
    strncpy(topic_str, topic, topic_len);
    topic_str[topic_len] = '\0'; // Adiciona o caractere nulo ao final da string

    strncpy(data_str, data, data_len);
    data_str[data_len] = '\0'; // Adiciona o caractere nulo ao final da string

    // Verifica se o tópico é "FSEAH2024/home-automation/disaster-prevention/gas-leak"
    if (strcmp(topic_str, "FSEAH2024/home-automation/disaster-prevention/gas-leak") == 0) {
        // Verifica se os dados são {"hasLeak":true}
        if (strcmp(data_str, "{\"hasLeak\":true}") == 0) {
            if(alarmeStatus == 1){
                save_variable("storage", "alrmDisp", 1);
                alrmDisp = 1;
                if(calrmDisp == 2){calrmDisp = 3;}
                else{calrmDisp = 1;}
                save_variable("storage", "calrmDisp", calrmDisp);
            }
        }
    }
    // Verifica se o tópico é "FSEAH2024/home-automation/disaster-prevention/fire"
    else if (strcmp(topic_str, "FSEAH2024/home-automation/disaster-prevention/fire") == 0) {
        // Verifica se os dados são {"hasFire":true}
        if (strcmp(data_str, "{\"hasFire\":true}") == 0) {
            if(alarmeStatus == 1){
                save_variable("storage", "alrmDisp", 1);
                if(calrmDisp == 1){calrmDisp = 3;}else{calrmDisp = 2;}
                alrmDisp = 1;
                save_variable("storage", "calrmDisp", calrmDisp);
            }
        }
    }
    else if (strcmp(topic_str, "FSEAH2024/home-automation/temperature-humidity") == 0) {
        // Verifica se os dados são {"hasFire":true}
        if (sscanf(data_str, "{\"temperature\": %d, \"humidity\": %d }",
                    &temperatura, &umidade) == 2) {
            
        }
    }
    else if (strcmp(topic_str, "FSEAH2024/home-automation/relay/status") == 0) {
        // Verifica se os dados são {"hasFire":true}
        if (sscanf(data_str, "{ \"lampada\": %d , \"geral\": %d}",
                   &lampada, &geral) == 2) {
            
        }
    }
    else if (sscanf(topic_str, "v1/devices/me/rpc/request/%d", &request_id) == 1) {
        if (strcmp(data_str, "{\"method\":\"ligaDesligaAlarme\",\"params\":true}") == 0) {
            if(alarmeStatus == 0){alarmeStatus = 1;save_variable("storage", "alarmeStatus", 1);save_variable("storage", "calrmDisp", 0);}
            }
        else if (strcmp(data_str, "{\"method\":\"ligaDesligaAlarme\",\"params\":false}") == 0) {
            if (alarmeStatus == 1){save_variable("storage", "alarmeStatus", 0);save_variable("storage", "calrmDisp", 0);alarmeStatus = 0;}
        }

        else if (sscanf(data_str, "{\"method\":\"%31[^\"]\",\"params\":{}}", method) == 1) {
            if (strcmp(method, "paraAlarme") == 0) {
                printf("RECEBIDO PARA ALARME\n");
                if (alarmeStatus == 1 && alrmDisp == 1) {
                    alrmDisp = 0;
                    alarmeStatus = 1;
                    save_variable("storage", "alrmDisp", 0);
                    save_variable("storage", "calrmDisp", 0);
                }
            }
        }
    }

    #elif defined(CONFIG_USE_RELAYS)

    int request_id;
    char method[32];

    char topic_str[topic_len + 1];
    char data_str[data_len + 1];

    // Copia o tópico e os dados para buffers temporários
    strncpy(topic_str, topic, topic_len);
    topic_str[topic_len] = '\0'; // Adiciona o caractere nulo ao final da string

    strncpy(data_str, data, data_len);
    data_str[data_len] = '\0'; // Adiciona o caractere nulo ao final da string

    if (strcmp(topic_str, "FSEAH2024/home-automation/relay/set/lampada") == 0) {

        if (strcmp(data_str, "{true}") == 0) {
            if(lampada == 1){lampada = 0;save_variable("storage", "slamp", 0);relay_set_state(1,0);}
        }else if (strcmp(data_str, "{false}") == 0) {
            if (lampada == 0){lampada = 1;save_variable("storage", "slamp", 1);relay_set_state(1,1);}
        }
    }else if(strcmp(topic_str, "FSEAH2024/home-automation/relay/set/geral") == 0){
        if (strcmp(data_str, "{true}") == 0) {
            if(geral == 1){geral = 0;save_variable("storage", "sgeral", 0);relay_set_state(2,0);}
        }else if (strcmp(data_str, "{false}") == 0) {
            if (geral == 0){geral = 1;save_variable("storage", "sgeral", 1);relay_set_state(2,1);}
        }

    }
    else if (sscanf(topic_str, "v1/devices/me/rpc/request/%d", &request_id) == 1) {
        if (strcmp(data_str, "{\"method\":\"ligaDesligaLampada\",\"params\":true}") == 0) {
            if(lampada == 1){lampada = 0;save_variable("storage", "slamp", 0);relay_set_state(1,0);}
            }
        else if (strcmp(data_str, "{\"method\":\"ligaDesligaLampada\",\"params\":false}") == 0) {
            if (lampada == 0){lampada = 1;save_variable("storage", "slamp", 1);relay_set_state(1,1);}
        }
        else if (strcmp(data_str, "{\"method\":\"ligaDesligaGeral\",\"params\":true}") == 0) {
            if(geral == 1){geral = 0;save_variable("storage", "sgeral", 0);relay_set_state(2,0);}
            }
        else if (strcmp(data_str, "{\"method\":\"ligaDesligaGeral\",\"params\":false}") == 0) {
            if (geral == 0){geral = 1;save_variable("storage", "sgeral", 1);relay_set_state(2,1);}
        }
    }

        



    #endif /////////////////////////////////////////////////

    printf("TOPIC=%.*s\r\n", topic_len, topic);
    printf("DATA=%.*s\r\n", data_len, data);
}

