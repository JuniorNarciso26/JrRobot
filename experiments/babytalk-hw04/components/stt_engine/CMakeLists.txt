# Same pinned upstream sources; explicitly declare esp-dsp for header resolution.
idf_component_register(
    SRCS "${BABYTALK_ROOT}/components/stt_engine/stt_engine.c"
         "${BABYTALK_ROOT}/stt/main/stt_core.c" "${BABYTALK_ROOT}/stt/main/kws.c"
    INCLUDE_DIRS "${BABYTALK_ROOT}/components/stt_engine" "${BABYTALK_ROOT}/stt/main"
    REQUIRES mmrt sram_pool esp_partition esp_timer heap freertos spi_flash esp_psram esp_hw_support espressif__esp-dsp)
target_compile_definitions(${COMPONENT_LIB} PRIVATE STT_USE_DSP)
target_compile_options(${COMPONENT_LIB} PRIVATE -O2)
