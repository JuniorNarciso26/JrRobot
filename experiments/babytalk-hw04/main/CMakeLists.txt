idf_component_register(SRCS "main.c" INCLUDE_DIRS "."
    REQUIRES stt_engine sram_pool esp_driver_gpio esp_driver_i2s esp_timer heap)
