#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "RCSwitch.h"

static const char *TAG = "MAIN";

void app_main()
{
	// Initialize NVS
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK( err );

	// Open NVS
	nvs_handle_t my_handle;
	err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "nvs_open error (%s)", esp_err_to_name(err));
		vTaskDelete(NULL);
	}

	// Read NVS
	uint32_t ValueOn = 0;
	uint16_t BitlengthOn = 0;
	uint16_t ProtocolOn = 0;
	uint32_t ValueOff = 0;
	uint16_t BitlengthOff = 0;
	uint16_t ProtocolOff = 0;

	err = nvs_get_u32(my_handle, "ValueOn", &ValueOn);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ValueOn get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "ValueOn=%lu", ValueOn);

	err = nvs_get_u16(my_handle, "BitlengthOn", &BitlengthOn);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "BitlengthOn get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "BitlengthOn=%u", BitlengthOn);

	err = nvs_get_u16(my_handle, "ProtocolOn", &ProtocolOn);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ProtocolOn get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "ProtocolOn=%u", ProtocolOn);

	err = nvs_get_u32(my_handle, "ValueOff", &ValueOff);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ValueOff get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "ValueOff=%lu", ValueOff);

	err = nvs_get_u16(my_handle, "BitlengthOff", &BitlengthOff);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "BitlengthOff get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "BitlengthOff=%u", BitlengthOff);

	err = nvs_get_u16(my_handle, "ProtocolOff", &ProtocolOff);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ProtocolOff get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "ProtocolOff=%u", ProtocolOff);

	// Close NVS
	nvs_close(my_handle);

	// Initialize RF
	RCSWITCH_t RCSwitch;
	initSwich(&RCSwitch);
	enableTransmit(&RCSwitch, CONFIG_RF_GPIO);
	setRepeatTransmit(&RCSwitch, 3);

	int32_t interval_to_off = CONFIG_INTERVAL_TO_OFF * 1000;
	int32_t interval_to_on = CONFIG_INTERVAL_TO_ON * 1000;

#if CONFIG_INITIAL_STATE_ON
	ESP_LOGI(TAG, "USB ON");
	setProtocol(&RCSwitch, ProtocolOn);
	send(&RCSwitch, ValueOn, BitlengthOn);
	vTaskDelay(pdMS_TO_TICKS(interval_to_off));

	while(1) {
		ESP_LOGI(TAG, "USB OFF");
		setProtocol(&RCSwitch, ProtocolOff);
		send(&RCSwitch, ValueOff, BitlengthOff);
		vTaskDelay(pdMS_TO_TICKS(interval_to_on));
		ESP_LOGI(TAG, "USB ON");
		setProtocol(&RCSwitch, ProtocolOn);
		send(&RCSwitch, ValueOn, BitlengthOn);
		vTaskDelay(pdMS_TO_TICKS(interval_to_off));
	} // end while

#elif CONFIG_INITIAL_STATE_OFF
	ESP_LOGI(TAG, "USB OFF");
	setProtocol(&RCSwitch, ProtocolOff);
	send(&RCSwitch, ValueOff, BitlengthOff);
	vTaskDelay(pdMS_TO_TICKS(interval_to_on));

	while(1) {
		ESP_LOGI(TAG, "USB ON");
		setProtocol(&RCSwitch, ProtocolOn);
		send(&RCSwitch, ValueOn, BitlengthOn);
		vTaskDelay(pdMS_TO_TICKS(interval_to_off));
		ESP_LOGI(TAG, "USB OFF");
		setProtocol(&RCSwitch, ProtocolOff);
		send(&RCSwitch, ValueOff, BitlengthOff);
		vTaskDelay(pdMS_TO_TICKS(interval_to_on));
	} // end while
#endif
}
