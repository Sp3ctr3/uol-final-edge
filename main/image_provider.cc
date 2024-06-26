
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
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "app_camera_esp.h"
#include "esp_camera.h"
#include "model_settings.h"
#include "image_provider.h"
#include "esp_main.h"


// // Code from ESP32 Camera driver
// #include "esp_http_server.h"
// typedef struct {
//         httpd_req_t *req;
//         size_t len;
// } jpg_chunking_t;


// static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
//     jpg_chunking_t *j = (jpg_chunking_t *)arg;
//     if(!index){
//         j->len = 0;
//     }
//     if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
//         return 0;
//     }
//     j->len += len;
//     return len;
// }
// // End of code from ESP32 Camera driver

static const char* TAG = "app_camera";

static uint16_t *display_buf; // buffer to hold data to be sent to display

// Get the camera module ready
TfLiteStatus InitCamera() {
#if CLI_ONLY_INFERENCE
  ESP_LOGI(TAG, "CLI_ONLY_INFERENCE enabled, skipping camera init");
  return kTfLiteOk;
#endif
// if display support is present, initialise display buf
#if DISPLAY_SUPPORT
  if (display_buf == NULL) {
    display_buf = (uint16_t *) heap_caps_malloc(96 * 2 * 120 * 2 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  if (display_buf == NULL) {
    ESP_LOGE(TAG, "Couldn't allocate display buffer");
    return kTfLiteError;
  }
#endif

#if ESP_CAMERA_SUPPORTED
  int ret = app_camera_init();
  if (ret != 0) {
    MicroPrintf("Camera init failed\n");
    return kTfLiteError;
  }
  MicroPrintf("Detection system is online\n");
#else
  ESP_LOGE(TAG, "Camera not supported for this device");
#endif
  return kTfLiteOk;
}

void *image_provider_get_display_buf()
{
  return (void *) display_buf;
}

// Get an image from the camera module
TfLiteStatus GetImage(int image_width, int image_height, int channels, int8_t* image_data, uint8_t* jpg_data) {
#if ESP_CAMERA_SUPPORTED
  camera_fb_t* fb = esp_camera_fb_get();
  // bool converted = frame2bmp(fb, &bmp_data, (size_t *) 1);
  // MicroPrintf("Image in bmp %d\n",(size_t *) sizeof(bmp_data));
  // size_t _jpg_buf_len;
  // uint8_t * _jpg_buf;
  // bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
  // MicroPrintf("Jpeg compression success: %d\n",jpeg_converted); 
  // MicroPrintf("Jpeg compression size: %d\n",_jpg_buf_len); 
  // if(jpeg_converted){
    // memcpy(jpg_data,_jpg_buf,_jpg_buf_len);
    // free(_jpg_buf);
  // }
  if (!fb) {
    ESP_LOGE(TAG, "Camera capture failed");
    return kTfLiteError;
  }

#if DISPLAY_SUPPORT
  // In case if display support is enabled, we initialise camera in rgb mode
  // Hence, we need to convert this data to grayscale to send it to tf model
  // For display we extra-polate the data to 192X192
  for (int i = 0; i < kNumRows; i++) {
    for (int j = 0; j < kNumCols; j++) {
      uint16_t pixel = ((uint16_t *) (fb->buf))[i * kNumCols + j];

      // for inference
      uint8_t hb = pixel & 0xFF;
      uint8_t lb = pixel >> 8;
      uint8_t r = (lb & 0x1F) << 3;
      uint8_t g = ((hb & 0x07) << 5) | ((lb & 0xE0) >> 3);
      uint8_t b = (hb & 0xF8);

      /**
       * Gamma corected rgb to greyscale formula: Y = 0.299R + 0.587G + 0.114B
       * for effiency we use some tricks on this + quantize to [-128, 127]
       */
      int8_t grey_pixel = ((305 * r + 600 * g + 119 * b) >> 10) - 128;

      image_data[i * kNumCols + j] = grey_pixel;

      // to display
      display_buf[2 * i * kNumCols * 2 + 2 * j] = pixel;
      display_buf[2 * i * kNumCols * 2 + 2 * j + 1] = pixel;
      display_buf[(2 * i + 1) * kNumCols * 2 + 2 * j] = pixel;
      display_buf[(2 * i + 1) * kNumCols * 2 + 2 * j + 1] = pixel;
    }
  }
#else
  // MicroPrintf("Image Captured\n");
  // We have initialised camera to grayscale
  // Just quantize to int8_t
  for (int i = 0; i < image_width * image_height; i++) {
    image_data[i] = ((uint8_t *) fb->buf)[i] ^ 0x80;
  }
#endif

  esp_camera_fb_return(fb);
  /* here the esp camera can give you grayscale image directly */
  return kTfLiteOk;
#else
  return kTfLiteError;
#endif
}
