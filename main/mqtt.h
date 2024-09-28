#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

// Declaração dos clientes MQTT para múltiplos brokers
extern esp_mqtt_client_handle_t client1;
extern esp_mqtt_client_handle_t client2;

// Função para iniciar as conexões MQTT e assinar tópicos
void mqtt_start();

// Função para enviar uma mensagem para o broker especificado
void mqtt_envia_mensagem(char *topico, char *mensagem, int broker_id);

// Função para assinar múltiplos tópicos
void mqtt_subscribe_to_topics(esp_mqtt_client_handle_t client, const char **topics, int num_topics);

void handle_mqtt_event_data(const char *topic, int topic_len, const char *data, int data_len);

void fire();
#endif // MQTT_H
