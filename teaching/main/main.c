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

	ESP_LOGI(TAG, "Start receiver");
	RCSWITCH_t RCSwitch;
	initSwich(&RCSwitch);
	enableReceive(&RCSwitch, CONFIG_RF_GPIO);

	ESP_LOGW(TAG, "Press the ON switch");
	unsigned long ValueOn = 0;
	unsigned int BitlengthOn = 0;
	unsigned int ProtocolOn = 0;
	while(1) {
		if (available(&RCSwitch)) {
			ValueOn = getReceivedValue(&RCSwitch);
			BitlengthOn = getReceivedBitlength(&RCSwitch);
			ProtocolOn = getReceivedProtocol(&RCSwitch);
			ESP_LOGI(TAG, "Received %lu / %dbit Protocol: %d", ValueOn, BitlengthOn, ProtocolOn);
			resetAvailable(&RCSwitch);
			break;
		} else {
			vTaskDelay(1);
		}
	} // end while

	// Clear RF
	for (int i=0;i<100;i++) {
		if (available(&RCSwitch)) {
			getReceivedValue(&RCSwitch);
			resetAvailable(&RCSwitch);
		} else {
			vTaskDelay(1);
		}
	}

	ESP_LOGW(TAG, "Press the OFF switch");
	unsigned long ValueOff = 0;
	unsigned int BitlengthOff = 0;
	unsigned int ProtocolOff = 0;
	while(1) {
		if (available(&RCSwitch)) {
			ValueOff = getReceivedValue(&RCSwitch);
			BitlengthOff = getReceivedBitlength(&RCSwitch);
			ProtocolOff = getReceivedProtocol(&RCSwitch);
			ESP_LOGI(TAG, "Received %lu / %dbit Protocol: %d", ValueOff, BitlengthOff, ProtocolOff);
			resetAvailable(&RCSwitch);
			break;
		} else {
			vTaskDelay(1);
		}
	} // end while

	// Open NVS
	nvs_handle_t my_handle;
	err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "nvs_open error (%s)", esp_err_to_name(err));
		vTaskDelete(NULL);
	}

	// Set NVS
	err = nvs_set_u32(my_handle, "ValueOn", ValueOn);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ValueOn set failed");
		vTaskDelete(NULL);
	}

	err = nvs_set_u16(my_handle, "BitlengthOn", BitlengthOn);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "BitlengthOn set failed");
		vTaskDelete(NULL);
	}

	err = nvs_set_u16(my_handle, "ProtocolOn", ProtocolOn);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ProtocolOn set failed");
		vTaskDelete(NULL);
	}

	err = nvs_set_u32(my_handle, "ValueOff", ValueOff);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ValueOff set failed");
		vTaskDelete(NULL);
	}

	err = nvs_set_u16(my_handle, "BitlengthOff", BitlengthOff);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "BitlengthOff set failed");
		vTaskDelete(NULL);
	}

	err = nvs_set_u16(my_handle, "ProtocolOff", ProtocolOff);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ProtocolOff set failed");
		vTaskDelete(NULL);
	}

	// Commit written value.
	// After setting any values, nvs_commit() must be called to ensure changes are written
	// to flash storage. Implementations may write to storage at other times,
	// but this is not guaranteed.
	err = nvs_commit(my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "nvs_commit failed");
		vTaskDelete(NULL);
	}

	// Close NVS
	nvs_close(my_handle);
}
