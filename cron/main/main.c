/*
	Example to turn on/off USB using cron

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
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_sntp.h"

#include "ccronexpr.h"

#include "RCSwitch.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "MAIN";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

esp_err_t wifi_init_sta(void)
{
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&event_handler,
		NULL,
		&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&event_handler,
		NULL,
		&instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_ESP_WIFI_SSID,
			.password = CONFIG_ESP_WIFI_PASSWORD,
			/* Setting a password implies station will connect to all security modes including WEP/WPA.
			 * However these modes are deprecated and not advisable to be used. Incase your Access point
			 * doesn't support WPA2, these mode can be enabled by commenting below line */
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,

			.pmf_cfg = {
				.capable = true,
				.required = false
			},
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	esp_err_t ret_value = ESP_OK;
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
		WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
		pdFALSE,
		pdFALSE,
		portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
		ret_value = ESP_FAIL;
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
		ret_value = ESP_FAIL;
	}

	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	vEventGroupDelete(s_wifi_event_group);
	return ret_value;
}

static void printSPIFFS(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(__FUNCTION__,"%s d_name=%s d_ino=%d d_type=%x", path, pe->d_name, pe->d_ino, pe->d_type);
		if (pe->d_type == 2) {
			char subDir[127];
			sprintf(subDir,"%s%.64s", path, pe->d_name);
			ESP_LOGI(TAG, "subDir=[%s]", subDir);
			printSPIFFS(subDir);

		}
	}
	closedir(dir);
}

esp_err_t mountSPIFFS(char * partition_label, char * base_path) {
	ESP_LOGI(TAG, "Initializing SPIFFS file system");

	esp_vfs_spiffs_conf_t conf = {
		.base_path = base_path,
		.partition_label = partition_label,
		.max_files = 5,
		.format_if_mount_failed = true
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return ret;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}
	ESP_LOGI(TAG, "Mount SPIFFS filesystem");
	return ret;
}

void time_sync_notification_cb(struct timeval *tv)
{
	ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
	ESP_LOGI(TAG, "Initializing SNTP");
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	//sntp_setservername(0, "pool.ntp.org");
	ESP_LOGI(TAG, "Your NTP Server is %s", CONFIG_NTP_SERVER);
	esp_sntp_setservername(0, CONFIG_NTP_SERVER);
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
	esp_sntp_init();
}

static esp_err_t obtain_time(void)
{
	initialize_sntp();
	// wait for time to be set
	int retry = 0;
	const int retry_count = 10;
	while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
		ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}

	if (retry == retry_count) return ESP_FAIL;
	return ESP_OK;
}

typedef struct {
	char dateTime[64];
	char taskName[32];
	time_t next;
} CRON_t;


esp_err_t build_table(char *fileName, CRON_t **tables, int16_t *ntable) {
	FILE* f = fopen(fileName, "r");
	if (f == NULL) {
		ESP_LOGE(__FUNCTION__, "Failed to open file for reading");
		return ESP_FAIL;
	}
	char line[128];
	int _ntable = 0;
	while (1){
		if ( fgets(line, sizeof(line) ,f) == 0 ) break;
		// strip newline
		ESP_LOGD(__FUNCTION__, "line0=[%s]", line);
		char* pos = strchr(line, '\n');
		if (pos) *pos = '\0';
		if (strlen(line) == 0) continue;
		if (line[0] == '#') continue;
		ESP_LOGD(__FUNCTION__, "line1=[%s]", line);
		_ntable++;
	}
	fclose(f);
	ESP_LOGI(__FUNCTION__, "_ntable=%d", _ntable);

	*tables = calloc(_ntable, sizeof(CRON_t));
	if (*tables == NULL) {
		ESP_LOGE(__FUNCTION__, "Error allocating memory for topic");
		return ESP_ERR_NO_MEM;
	}

	char dateTime[64];
	char taskName[128];
	int index = 0;
	f = fopen(fileName, "r");
	while (1){
		if ( fgets(line, sizeof(line) ,f) == 0 ) break;
		// strip newline
		ESP_LOGD(__FUNCTION__, "line0=[%s]", line);
		char* pos = strchr(line, '\n');
		if (pos) *pos = '\0';
		if (strlen(line) == 0) continue;
		if (line[0] == '#') continue;
		ESP_LOGD(__FUNCTION__, "line1=[%s]", line);
		int items = 0;
		for(int pos=0;pos<strlen(line);pos++) {
			int c1 = line[pos];
			ESP_LOGD(__FUNCTION__, "c1[%d]=0x%x items=%d", pos, c1, items);
			if (c1 == 0x20) items++;
			if (items == 6) {
				strcpy(taskName, &line[pos+1]);
				break;
			}
			dateTime[pos] = c1;
			dateTime[pos+1] = 0;
		}

		// Remove comment from taskName
		for (int pos=0;pos<strlen(taskName);pos++) {
			int c1 = taskName[pos];
			if (c1 == 0x20) taskName[pos] = 0;
		}

		ESP_LOGI(__FUNCTION__, "dateTime=[%s]", dateTime);
		ESP_LOGI(__FUNCTION__, "taskName=[%s]", taskName);
		cron_expr expr;
		const char* err = NULL;
		memset(&expr, 0, sizeof(expr));
		cron_parse_expr(dateTime, &expr, &err);
		if (err) {
			ESP_LOGE(__FUNCTION__, "[%s] %s", line, err);
		} else {
			strcpy((*tables+index)->dateTime, dateTime);
			strcpy((*tables+index)->taskName, taskName);
			time_t cur = time(NULL);
			cur = cur + (CONFIG_LOCAL_TIMEZONE*60*60);
			(*tables+index)->next = cron_next(&expr, cur);
			index++;
		}
	}
	for (int i=0;i<index;i++) {
		ESP_LOGI(__FUNCTION__, "dateTime[%d]=[%s]", i, (*tables+i)->dateTime);
		ESP_LOGI(__FUNCTION__, "taskName[%d]=[%s]", i, (*tables+i)->taskName);
	}
	*ntable = index;
	return ESP_OK;
}

// Turn on USB
void task_on(void *pvParameters) {
	ESP_LOGI(pcTaskGetName(NULL), "%"PRIu32" Start", (uint32_t)xTaskGetCurrentTaskHandle());

	// Open NVS
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(pcTaskGetName(NULL), "nvs_open error (%s)", esp_err_to_name(err));
		vTaskDelete(NULL);
	}

	// Read NVS
	uint32_t ValueOn = 0;
	uint16_t BitlengthOn = 0;
	uint16_t ProtocolOn = 0;

	err = nvs_get_u32(my_handle, "ValueOn", &ValueOn);
	if (err != ESP_OK) {
		ESP_LOGE(pcTaskGetName(NULL), "ValueOn get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(pcTaskGetName(NULL), "ValueOn=%lu", ValueOn);

	err = nvs_get_u16(my_handle, "BitlengthOn", &BitlengthOn);
	if (err != ESP_OK) {
		ESP_LOGE(pcTaskGetName(NULL), "BitlengthOn get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(pcTaskGetName(NULL), "BitlengthOn=%u", BitlengthOn);

	err = nvs_get_u16(my_handle, "ProtocolOn", &ProtocolOn);
	if (err != ESP_OK) {
		ESP_LOGE(pcTaskGetName(NULL), "ProtocolOn get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(pcTaskGetName(NULL), "ProtocolOn=%u", ProtocolOn);

	// Close NVS
	nvs_close(my_handle);

	// Initialize RF
	RCSWITCH_t RCSwitch;
	initSwich(&RCSwitch);
	enableTransmit(&RCSwitch, CONFIG_RF_GPIO);
	setRepeatTransmit(&RCSwitch, 10);

	while(1) {
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		ESP_LOGI(pcTaskGetName(NULL), "Wake Up");
		setProtocol(&RCSwitch, ProtocolOn);
		sendCode(&RCSwitch, ValueOn, BitlengthOn);
		ESP_LOGD(pcTaskGetName(NULL), "Wait for Notify");
	}
	vTaskDelete(NULL);
}

// Turn off USB
void task_off(void *pvParameters) {
	ESP_LOGI(pcTaskGetName(NULL), "%"PRIu32" Start", (uint32_t)xTaskGetCurrentTaskHandle());

	// Open NVS
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(pcTaskGetName(NULL), "nvs_open error (%s)", esp_err_to_name(err));
		vTaskDelete(NULL);
	}

	// Read NVS
	uint32_t ValueOff = 0;
	uint16_t BitlengthOff = 0;
	uint16_t ProtocolOff = 0;

	err = nvs_get_u32(my_handle, "ValueOff", &ValueOff);
	if (err != ESP_OK) {
		ESP_LOGE(pcTaskGetName(NULL), "ValueOff get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(pcTaskGetName(NULL), "ValueOff=%lu", ValueOff);

	err = nvs_get_u16(my_handle, "BitlengthOff", &BitlengthOff);
	if (err != ESP_OK) {
		ESP_LOGE(pcTaskGetName(NULL), "BitlengthOff get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(pcTaskGetName(NULL), "BitlengthOff=%u", BitlengthOff);

	err = nvs_get_u16(my_handle, "ProtocolOff", &ProtocolOff);
	if (err != ESP_OK) {
		ESP_LOGE(pcTaskGetName(NULL), "ProtocolOff get failed");
		vTaskDelete(NULL);
	}
	ESP_LOGI(pcTaskGetName(NULL), "ProtocolOff=%u", ProtocolOff);

	// Close NVS
	nvs_close(my_handle);

	// Initialize RF
	RCSWITCH_t RCSwitch;
	initSwich(&RCSwitch);
	enableTransmit(&RCSwitch, CONFIG_RF_GPIO);
	setRepeatTransmit(&RCSwitch, 10);

	while(1) {
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		ESP_LOGI(pcTaskGetName(NULL), "Wake Up");
		setProtocol(&RCSwitch, ProtocolOff);
		sendCode(&RCSwitch, ValueOff, BitlengthOff);
		ESP_LOGD(pcTaskGetName(NULL), "Wait for Notify");
	}
	vTaskDelete(NULL);
}

void app_main(void)
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Initialize WiFi
	ESP_ERROR_CHECK(wifi_init_sta());

	// Obtain time over NTP
	ret = obtain_time();
	if(ret != ESP_OK) {
		ESP_LOGE(TAG, "Fail to getting time over NTP.");
		return;
	}

#if 0
	// Print current time
	char buffer[32];
	time_t cur = time(NULL);
	cur = cur + (CONFIG_LOCAL_TIMEZONE*60*60);
	struct tm *cur_timeinfo;
	cur_timeinfo = gmtime(&cur);
	strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", cur_timeinfo);
	ESP_LOGI(TAG, "buffer=[%s]", buffer);
#endif

	// Mount SPIFFS
	char *partition_label = "storage";
	char *base_path = "/spiffs"; 
	ret = mountSPIFFS(partition_label, base_path);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "mountSPIFFS fail");
		while(1) { vTaskDelay(1); }
	}
	printSPIFFS(base_path);
	
	// Read crontab
	CRON_t *crontab;
	int16_t	lcrontab;
	char fileName[128];
	sprintf(fileName, "%s/crontab", base_path);
	ret = build_table(fileName, &crontab, &lcrontab);

	// Start task
	xTaskCreate(task_on, "task_on", 1024*4, NULL, 2, NULL);
	xTaskCreate(task_off, "task_off", 1024*4, NULL, 2, NULL);

	// Start main loop
	cron_expr expr;
	const char* err = NULL;
	while (1) {
		// Get current date and time
		time_t cur = time(NULL);
		cur = cur + (CONFIG_LOCAL_TIMEZONE*60*60);

		// Format current date and time
		struct tm *cur_timeinfo;
		cur_timeinfo = gmtime(&cur);
		char cur_buffer[32];
		strftime(cur_buffer, sizeof(cur_buffer), "%Y/%m/%d %H:%M:%S", cur_timeinfo);
		ESP_LOGD(TAG, "current time=[%s]", cur_buffer);

		for (int index=0;index<lcrontab;index++) {
			ESP_LOGD(TAG, "dateTime[%d]=[%s]", index, (crontab+index)->dateTime);
			ESP_LOGD(TAG, "taskName[%d]=[%s]", index, (crontab+index)->taskName);

			// Format the next "fire" date and time
			time_t next = (crontab+index)->next;
			struct tm *next_timeinfo;
			next_timeinfo = gmtime(&next);
			char next_buffer[32];
			strftime(next_buffer, sizeof(next_buffer), "%Y/%m/%d %H:%M:%S", next_timeinfo);
			ESP_LOGD(TAG, "next=[%s]", next_buffer);

			// Compare the current date and time with the next "fire" date and time
			if (strcmp(cur_buffer, next_buffer) == 0) {

				// Get the task handle to notify
				TaskHandle_t taskHandle = xTaskGetHandle((crontab+index)->taskName);
				ESP_LOGI(TAG, "taskname=[%s] taskHandle=%"PRIu32, (crontab+index)->taskName, (uint32_t)taskHandle);
				if (taskHandle != NULL) {
#if 0
					eTaskState taskState = eTaskGetState(taskHandle);
					ESP_LOGI(TAG, "taskState=%d", taskState);
					if (taskState == eReady) ESP_LOGI(TAG, "taskState=eReady");
					if (taskState == eRunning) ESP_LOGI(TAG, "taskState=eRunning");
					if (taskState == eBlocked) ESP_LOGI(TAG, "taskState=eBlocked");
					if (taskState == eSuspended) ESP_LOGI(TAG, "taskState=eSuspended");
					if (taskState == eDeleted) ESP_LOGI(TAG, "taskState=eDeleted");
#endif
					ESP_LOGI(TAG, "current time=[%s]", cur_buffer);
					ESP_LOGI(TAG, "NotifGive to %s [%s]", (crontab+index)->taskName, (crontab+index)->dateTime);
					xTaskNotifyGive(taskHandle);

					// Parses specified cron expression
					memset(&expr, 0, sizeof(expr));
					cron_parse_expr((crontab+index)->dateTime, &expr, &err);

					// Set the specified expression to calculate the next 'fire' date after the specified date
					(crontab+index)->next = cron_next(&expr, cur);
				} else {
					ESP_LOGE(TAG, "%s not active", (crontab+index)->taskName);
				}
			}
		}
		// delay 1 second
		vTaskDelay(100);
	} // end while
}
