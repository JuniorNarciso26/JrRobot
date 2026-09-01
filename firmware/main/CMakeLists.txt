idf_component_register(
    SRCS
        "main.c"
        "../core/jr_wifi.c"
        "../core/jr_commands.c"
        "../module_face/jr_face.c"
        "../module_portal/jr_portal.c"
    INCLUDE_DIRS
        "."
        "../core"
        "../module_face"
        "../module_portal"
    REQUIRES esp_driver_i2c esp_driver_uart esp_driver_gpio esp_wifi esp_event esp_netif esp_http_server nvs_flash log
)
