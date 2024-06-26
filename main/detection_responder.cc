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

/*
 * SPDX-FileCopyrightText: 2019-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"


#include "detection_responder.h"
#include "tensorflow/lite/micro/micro_log.h"

#include "esp_main.h"
#include "esp_log.h"

#if DISPLAY_SUPPORT
#include "image_provider.h"
#include <bsp/esp-bsp.h>
// #include "esp_lcd.h"
// static QueueHandle_t xQueueLCDFrame = NULL;


static lv_obj_t *camera_canvas = NULL;
#define IMG_WD (96 * 2)
#define IMG_HT (120 * 2)
#endif

#include "app_camera_esp.h"
#include "esp_camera.h"
#include "model_settings.h"
#include "image_provider.h"
#include "esp_main.h"

#include "mbedtls/base64.h"

#include "esp_http_client.h"
#include "esp_tls.h"

static const char *TAG = "node1";
#define MAX_HTTP_RECV_BUFFER 2048
#define MAX_HTTP_OUTPUT_BUFFER 2048


// static const char *REQUEST = "GET /alert HTTP/1.0\r\n"
//     "Host: 192.168.41.124\r\n"
//     "User-Agent: esp-idf/1.0 esp32\r\n";

 // Code I wrote based on HTTP example from ESP documentation
static void send_data()
{

    struct in_addr *addr;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(2040);
    int s, r;
    camera_fb_t* fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);

  size_t _jpg_buf_len;
  uint8_t * _jpg_buf;

  size_t outlen;

  bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);

  MicroPrintf("Jpeg compression success: %d\n",jpeg_converted); 
  MicroPrintf("Jpeg compression size: %d\n",_jpg_buf_len); 
 

    char recv_buf[64];
        s = socket(AF_INET, SOCK_STREAM, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "Failed to allocate socket.");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Allocated socket");
        if (inet_pton(AF_INET, "192.168.207.124", &serv_addr.sin_addr) <= 0) {
        printf(
            "\nInvalid address/ Address not supported \n");
        return;
        }

        if(connect(s, (struct sockaddr*)&serv_addr,sizeof(serv_addr) ) != 0) {
            ESP_LOGE(TAG, "Socket connect failed errno=%d", errno);
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
        }

        ESP_LOGI(TAG, "Connected");

        if (write(s, TAG, strlen(TAG)) < 0) {
            ESP_LOGE(TAG, "Socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
        }
        else {

            write(s, _jpg_buf, _jpg_buf_len); 


        }
        ESP_LOGI(TAG, "Socket send success");
        if(jpeg_converted){
        free(_jpg_buf);
        }
        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "Failed to set timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Set socket timeout successfully");

        // Read response
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(TAG, "Return=%d Error=%d.", r, errno);
        close(s);
    
}

//End of code I wrote

void RespondToDetection(float person_score, float no_person_score) {
  int person_score_int = (person_score) * 100 + 0.5;
  (void) no_person_score; // unused
#if DISPLAY_SUPPORT
#if 1 // USE LVGL
    // Create LVGL canvas for camera image
    if (!camera_canvas) {
      bsp_display_start();
      bsp_display_backlight_on(); // Set display brightness to 100%
      bsp_display_lock(0);
      camera_canvas = lv_canvas_create(lv_scr_act());
      assert(camera_canvas);
      lv_obj_center(camera_canvas);
      bsp_display_unlock();
    }

    uint16_t *buf = (uint16_t *) image_provider_get_display_buf();

    int color = 0x1f << 6; // red
    if (person_score_int < 60) { // treat score less than 60% as no person
      color = 0x3f; // green
    }
    for (int i = 192 * 192; i < 192 * 240; i++) {
        buf[i] = color;
    }
    bsp_display_lock(0);
    lv_canvas_set_buffer(camera_canvas, buf, IMG_WD, IMG_HT, LV_IMG_CF_TRUE_COLOR);
    bsp_display_unlock();
#else

#endif
#endif
    //Code I wrote
  if(person_score_int>70){
  MicroPrintf("A person has been detected with confidence %d%%. Sending alerts!", person_score_int);
  send_data();
   //End of code I wrote

  }
}
