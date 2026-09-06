idf_component_register(
    SRCS
        "main.c"
        "../core/jr_wifi.c"
        "../core/jr_commands.c"
        "../module_face/jr_face.c"
        "../module_portal/jr_portal.c"
        "../module_camera/jr_camera.c"
        "../module_audio/jr_audio.c"
    INCLUDE_DIRS
        "."
        "../core"
        "../module_face"
        "../module_portal"
        "../module_camera"
        "../module_audio"
    REQUIRES esp_driver_i2c esp_driver_uart esp_driver_gpio esp_driver_i2s esp_wifi esp_event esp_netif esp_http_server esp_psram esp32-camera nvs_flash log esp_timer
)
