/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "main_functions.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#define EXAMPLE_ESP_WIFI_SSID      "Pixel_7759"
#define EXAMPLE_ESP_WIFI_PASS      "canyons123"
#define EXAMPLE_ESP_MAXIMUM_RETRY  10

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "node1";

#include "esp_http_client.h"
#include "esp_tls.h"

#define MAX_HTTP_RECV_BUFFER 2048
#define MAX_HTTP_OUTPUT_BUFFER 2048


static int s_retry_num = 0;

// esp_err_t _http_event_handler(esp_http_client_event_t *evt)
// {
//     static char *output_buffer;  // Buffer to store response of http request from event handler
//     static int output_len;       // Stores number of bytes read
//     static esp_err_t err;
//     int mbedtls_err;
//     switch(evt->event_id) {
//         case HTTP_EVENT_ERROR:
//             ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
//             break;
//         case HTTP_EVENT_ON_CONNECTED:
//             ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
//             break;
//         case HTTP_EVENT_HEADER_SENT:
//             ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
//             break;
//         case HTTP_EVENT_ON_HEADER:
//             ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
//             break;
//         case HTTP_EVENT_ON_DATA:
//             ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
//             // Clean the buffer in case of a new request
//             if (output_len == 0 && evt->user_data) {
//                 // we are just starting to copy the output data into the use
//                 memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
//             }

//             break;
//         case HTTP_EVENT_ON_FINISH:
//             ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
//             if (output_buffer != NULL) {
//                 // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
//                 // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
//                 // free(output_buffer);
//                 output_buffer = NULL;
//             }
//             output_len = 0;
//             break;
//         case HTTP_EVENT_DISCONNECTED:
//             ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
//             mbedtls_err = 0;
//             err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
//             if (err != 0) {
//                 ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
//                 ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
//             }
//             if (output_buffer != NULL) {
//                 free(output_buffer);
//                 output_buffer = NULL;
//             }
//             output_len = 0;
//             break;
//         case HTTP_EVENT_REDIRECT:
//             ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
//             esp_http_client_set_header(evt->client, "From", "user@example.com");
//             esp_http_client_set_header(evt->client, "Accept", "text/html");
//             esp_http_client_set_redirection(evt->client);
//             break;
//     }
//     return ESP_OK;
// }

// extern void http_rest_with_url(void)
// {
//     // Declare local_response_buffer with size (MAX_HTTP_OUTPUT_BUFFER + 1) to prevent out of bound access when
//     // it is used by functions like strlen(). The buffer should only be used upto size MAX_HTTP_OUTPUT_BUFFER
//     char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};
//     /**
//      * NOTE: All the configuration parameters for http_client must be spefied either in URL or as host and path parameters.
//      * If host and path parameters are not set, query parameter will be ignored. In such cases,
//      * query parameter should be specified in URL.
//      *
//      * If URL as well as host and path parameters are specified, values of host and path will be considered.
//      */
//     esp_http_client_config_t config = {
//         .host = "192.168.41.124",
//         .path = "/get",
//         .query = "esp",
//         .event_handler = _http_event_handler,
//         .user_data = local_response_buffer,        // Pass address of local buffer to get response
//     };
//     esp_http_client_handle_t client = esp_http_client_init(&config);

//     // GET
//     esp_err_t err = esp_http_client_perform(client);
//     if (err == ESP_OK) {
//         ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
//                 esp_http_client_get_status_code(client),
//                 esp_http_client_get_content_length(client));
//     } else {
//         ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
//     }
//     ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));

//     // // POST
//     // const char *post_data = "{\"field1\":\"value1\"}";
//     // esp_http_client_set_url(client, "http://192.168.41.124/post");
//     // esp_http_client_set_method(client, HTTP_METHOD_POST);
//     // esp_http_client_set_header(client, "Content-Type", "application/json");
//     // esp_http_client_set_post_field(client, post_data, strlen(post_data));
//     // err = esp_http_client_perform(client);
//     // if (err == ESP_OK) {
//     //     ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
//     //             esp_http_client_get_status_code(client),
//     //             esp_http_client_get_content_length(client));
//     // } else {
//     //     ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
//     // }


//     esp_http_client_cleanup(client);
// }


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
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

void wifi_init_sta(void)
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
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

#include "esp_main.h"

#if CLI_ONLY_INFERENCE
#include "esp_cli.h"
#endif

void tf_main(void) {
  setup();
#if CLI_ONLY_INFERENCE
  esp_cli_init();
  esp_cli_register_cmds();
  vTaskDelay(portMAX_DELAY);
#else
  while (true) {
    loop();
  }
#endif
}

extern "C" void app_main() {
  
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("node1", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    // http_rest_with_url();
    // mqtt_app_start();
  xTaskCreate((TaskFunction_t)&tf_main, "tf_main", 4 * 1024, NULL, 8, NULL);
  // vTaskDelete(NULL);
}
