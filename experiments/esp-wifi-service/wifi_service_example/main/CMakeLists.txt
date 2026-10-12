idf_component_register(
    SRCS         "app_main.c" "wifi_service_console.c" "jr_mdns.c"
    INCLUDE_DIRS "."
    REQUIRES     bt driver nvs_flash mbedtls)

