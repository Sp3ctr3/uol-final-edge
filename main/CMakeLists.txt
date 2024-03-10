
#
# Main component of TF Micro project 'person_detection'.
#

idf_component_register(
    SRCS
        "detection_responder.cc"
        "image_provider.cc"
        "main.cc"
        "main_functions.cc"
        "model_settings.cc"
        "person_detect_model_data.cc"
        "app_camera_esp.c"
        # "esp_lcd.c"
        "esp_cli.c"

    PRIV_REQUIRES console static_images spi_flash
    REQUIRES esp_wifi nvs_flash esp_http_client esp-tls esp_http_server
    INCLUDE_DIRS "")
