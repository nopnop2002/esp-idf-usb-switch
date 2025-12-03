/*
	MQTT (over TCP) Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "mdns.h"
#include "mqtt_client.h"

#include "mqtt.h"

static const char *TAG = "MQTT";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	MQTT_t *mqttBuf = handler_args;
	ESP_LOGI(TAG, "taskHandle=0x%x", (unsigned int)mqttBuf->taskHandle);
	mqttBuf->event_id = event->event_id;
	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(__FUNCTION__, "MQTT_EVENT_CONNECTED");
			xTaskNotifyGive( mqttBuf->taskHandle );
			break;
		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(__FUNCTION__, "MQTT_EVENT_DISCONNECTED");
			xTaskNotifyGive( mqttBuf->taskHandle );
			break;
		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGI(__FUNCTION__, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			ESP_LOGI(__FUNCTION__, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_PUBLISHED:
			ESP_LOGI(__FUNCTION__, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_DATA:
			ESP_LOGD(__FUNCTION__, "MQTT_EVENT_DATA");
			ESP_LOGD(__FUNCTION__, "TOPIC=[%.*s] DATA=[%.*s]\r", event->topic_len, event->topic, event->data_len, event->data);

			mqttBuf->topic_len = event->topic_len;
			for(int i=0;i<event->topic_len;i++) {
				mqttBuf->topic[i] = event->topic[i];
				mqttBuf->topic[i+1] = 0;
			}
			mqttBuf->data_len = event->data_len;
			for(int i=0;i<event->data_len;i++) {
				mqttBuf->data[i] = event->data[i];
				mqttBuf->data[i+1] = 0;
			}
			xTaskNotifyGive( mqttBuf->taskHandle );
			break;
		case MQTT_EVENT_ERROR:
			ESP_LOGI(__FUNCTION__, "MQTT_EVENT_ERROR");
			xTaskNotifyGive( mqttBuf->taskHandle );
			break;
		default:
			ESP_LOGI(__FUNCTION__, "Other event id:%d", event->event_id);
			break;
	}
	return;
}

esp_err_t query_mdns_host(const char * host_name, char *ip)
{
	ESP_LOGD(__FUNCTION__, "Query A: %s", host_name);

	struct esp_ip4_addr addr;
	addr.addr = 0;

	esp_err_t err = mdns_query_a(host_name, 10000,	&addr);
	if(err){
		if(err == ESP_ERR_NOT_FOUND){
			ESP_LOGW(__FUNCTION__, "%s: Host was not found!", host_name);
		} else {
			ESP_LOGE(__FUNCTION__, "Query Failed: %s", esp_err_to_name(err));
		}
		return ESP_FAIL;
	}

	ESP_LOGD(__FUNCTION__, "Query A: %s.local resolved to: " IPSTR, host_name, IP2STR(&addr));
	sprintf(ip, IPSTR, IP2STR(&addr));
	return ESP_OK;
}

void convert_mdns_host(char * from, char * to)
{
	ESP_LOGI(__FUNCTION__, "from=[%s]",from);
	strcpy(to, from);
	char *sp;
	sp = strstr(from, ".local");
	if (sp == NULL) return;

	int _len = sp - from;
	ESP_LOGD(__FUNCTION__, "_len=%d", _len);
	char _from[128];
	strcpy(_from, from);
	_from[_len] = 0;
	ESP_LOGI(__FUNCTION__, "_from=[%s]", _from);

	char _ip[128];
	esp_err_t ret = query_mdns_host(_from, _ip);
	ESP_LOGI(__FUNCTION__, "query_mdns_host=%d _ip=[%s]", ret, _ip);
	if (ret != ESP_OK) return;

	strcpy(to, _ip);
	ESP_LOGI(__FUNCTION__, "to=[%s]", to);
}


void mqtt(void *pvParameters)
{
	ESP_LOGI(TAG, "start CONFIG_MQTT_BROKER=[%s]", CONFIG_MQTT_BROKER);

	// Set client id from mac
	uint8_t mac[8];
	ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));
	for(int i=0;i<8;i++) {
		ESP_LOGI(TAG, "mac[%d]=%x", i, mac[i]);
	}
	char client_id[64];
	sprintf(client_id, "esp32-%02x%02x%02x%02x%02x%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	ESP_LOGI(TAG, "client_id=[%s]", client_id);

	// Resolve mDNS host name
	char ip[128];
	ESP_LOGI(TAG, "CONFIG_MQTT_BROKER=[%s]", CONFIG_MQTT_BROKER);
	convert_mdns_host(CONFIG_MQTT_BROKER, ip);
	ESP_LOGI(TAG, "ip=[%s]", ip);
	char uri[138];
	sprintf(uri, "mqtt://%s", ip);
	ESP_LOGI(TAG, "uri=[%s]", uri);

	// Initialize user context
	MQTT_t mqttBuf;
	mqttBuf.taskHandle = xTaskGetCurrentTaskHandle();
	ESP_LOGI(TAG, "taskHandle=0x%x", (unsigned int)mqttBuf.taskHandle);
	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = uri,
		.broker.address.port = 1883,
		.credentials.client_id = client_id
	};

	esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, &mqttBuf);
	esp_mqtt_client_start(mqtt_client);

	// Get base topic (/api/hd44780/puts --> /api/hd44780/)
	int base_topic_len = strlen(CONFIG_MQTT_SUB_TOPIC)-1;
	ESP_LOGI(TAG, "base_topic_len=%d", base_topic_len);
	char base_topic[64];
	strcpy(base_topic, CONFIG_MQTT_SUB_TOPIC);
	base_topic[base_topic_len] = 0;
	ESP_LOGI(TAG, "base_topic=[%s]", base_topic);

	while (1) {
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		ESP_LOGD(TAG, "ulTaskNotifyTake");
		ESP_LOGI(TAG, "event_id=%"PRIi32, mqttBuf.event_id);

		if (mqttBuf.event_id == MQTT_EVENT_CONNECTED) {
			esp_mqtt_client_subscribe(mqtt_client, CONFIG_MQTT_SUB_TOPIC, 0);
			ESP_LOGI(TAG, "Subscribe to MQTT Server");
		} else if (mqttBuf.event_id == MQTT_EVENT_DISCONNECTED) {
			break;
		} else if (mqttBuf.event_id == MQTT_EVENT_DATA) {
			ESP_LOGI(TAG, "TOPIC=[%.*s]\r", mqttBuf.topic_len, mqttBuf.topic);
			ESP_LOGI(TAG, "DATA=[%.*s]\r", mqttBuf.data_len, mqttBuf.data);
			char bottom_topic[64];
			strcpy(bottom_topic, &mqttBuf.topic[base_topic_len]);
			ESP_LOGI(TAG, "bottom_topic=[%s]", bottom_topic);
			if (strcmp(bottom_topic, "on") == 0) {
				TaskHandle_t taskHandle = xTaskGetHandle("task_on");
				ESP_LOGI(TAG, "taskHandle=%"PRIu32, (uint32_t)taskHandle);
				xTaskNotifyGive(taskHandle);
			} else if (strcmp(bottom_topic, "off") == 0) {
				TaskHandle_t taskHandle = xTaskGetHandle("task_off");
				ESP_LOGI(TAG, "taskHandle=%"PRIu32, (uint32_t)taskHandle);
				xTaskNotifyGive(taskHandle);
			}
		} else if (mqttBuf.event_id == MQTT_EVENT_ERROR) {
			ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
			break;
		}
	} // end while

	ESP_LOGI(TAG, "Task Delete");
	esp_mqtt_client_stop(mqtt_client);
	vTaskDelete(NULL);
}
