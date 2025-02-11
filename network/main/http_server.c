/*
	 HTTP Server Example

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
#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_http_server.h"

static const char *TAG = "HTTP";

#define SCRATCH_BUFSIZE (1024)

typedef struct rest_server_context {
	char base_path[ESP_VFS_PATH_MAX + 1]; // Not used in this project
	char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;


/* Handler for root get */
static esp_err_t root_get_handler(httpd_req_t *req)
{
	ESP_LOGD(__FUNCTION__, "root_get_handler req->uri=[%s]", req->uri);

	/* Send empty chunk to signal HTTP response completion */
	httpd_resp_sendstr_chunk(req, NULL);

	return ESP_OK;
}

/* Handler for on */
static esp_err_t switch_on_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s] req->content_len=%d", req->uri, req->content_len);
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';
	ESP_LOGI(__FUNCTION__, "buf=[%s]", buf);

	httpd_resp_sendstr(req, "on successfully\n");

	TaskHandle_t taskHandle = xTaskGetHandle("task_on");
	ESP_LOGI(TAG, "taskHandle=%"PRIu32, (uint32_t)taskHandle);
	xTaskNotifyGive(taskHandle);

	return ESP_OK;
}

/* Handler for off */
static esp_err_t switch_off_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s] req->content_len=%d", req->uri, req->content_len);
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';
	ESP_LOGI(__FUNCTION__, "buf=[%s]", buf);

	httpd_resp_sendstr(req, "off successfully\n");

	TaskHandle_t taskHandle = xTaskGetHandle("task_off");
	ESP_LOGI(TAG, "taskHandle=%"PRIu32, (uint32_t)taskHandle);
	xTaskNotifyGive(taskHandle);

	return ESP_OK;
}

/* favicon get handler */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "favicon_get_handler req->uri=[%s]", req->uri);
	return ESP_OK;
}

/* Function to start the file server */
esp_err_t start_server(int port)
{
	rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
	if (rest_context == NULL) {
		ESP_LOGE(__FUNCTION__, "No memory for rest context");
		while(1) { vTaskDelay(1); }
	}

	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = port;

	/* Use the URI wildcard matching function in order to
	 * allow the same handler to respond to multiple different
	 * target URIs which match the wildcard scheme */
	config.uri_match_fn = httpd_uri_match_wildcard;

	ESP_LOGD(__FUNCTION__, "Starting HTTP Server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) != ESP_OK) {
		ESP_LOGE(__FUNCTION__, "Failed to start file server!");
		return ESP_FAIL;
	}

	/* URI handler for root */
	httpd_uri_t root = {
		.uri		 = "/",	// Match all URIs of type /path/to/file
		.method		 = HTTP_GET,
		.handler	 = root_get_handler,
		//.user_ctx  = server_data	// Pass server data as context
	};
	httpd_register_uri_handler(server, &root);

	/* URI handler for on */
	httpd_uri_t switch_on_uri = {
		.uri		 = "/api/on",
		.method		 = HTTP_POST,
		.handler	 = switch_on_handler, 
		.user_ctx	 = rest_context
	};
	httpd_register_uri_handler(server, &switch_on_uri);

	/* URI handler for off */
	httpd_uri_t switch_off_uri = {
		.uri		 = "/api/off",
		.method		 = HTTP_POST,
		.handler	 = switch_off_handler, 
		.user_ctx	 = rest_context
	};
	httpd_register_uri_handler(server, &switch_off_uri);

	/* URI handler for favicon.ico */
	httpd_uri_t _favicon_get_handler = {
		.uri		 = "/favicon.ico",
		.method		 = HTTP_GET,
		.handler	 = favicon_get_handler,
		//.user_ctx  = server_data	// Pass server data as context
	};
	httpd_register_uri_handler(server, &_favicon_get_handler);

	return ESP_OK;
}

#define WEB_PORT 8080

void http_server(void *pvParameters)
{
	char *task_parameter = (char *)pvParameters;
	ESP_LOGI(TAG, "Start task_parameter=%s", task_parameter);
	char url[64];
	sprintf(url, "http://%s:%d", task_parameter, WEB_PORT);
	ESP_LOGI(TAG, "Starting HTTP server on %s", url);
	ESP_ERROR_CHECK(start_server(WEB_PORT));

	while(1) {
		vTaskDelay(1);
	}

	// Never reach here
	ESP_LOGI(TAG, "finish");
	vTaskDelete(NULL);
}
