# vTTS is vendored only for integration builds on develop. The pinned source, two
# pt-BR model binaries and English lexicon are prepared by tools/prepare_vtts_develop.py.
# Production main does not depend on this repository or these generated assets.
option(JRBOT_ENABLE_VTTS "Build experimental vTTS pt-BR voice in JrBot" OFF)
set(JR_TTS_SOURCES "")
set(JR_TTS_INCLUDES "")
if(JRBOT_ENABLE_VTTS)
    set(VTTS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../vendor/vtts_src/runtime")
    set(VTTS_ASSETS "${CMAKE_CURRENT_LIST_DIR}/../vendor/vtts_assets")
    foreach(required
            "${VTTS_ROOT}/include/vtts.h"
            "${VTTS_ROOT}/src/vtts_model.c"
            "${VTTS_ROOT}/ports/esp32s3/vtts_port_s3.c"
            "${VTTS_ROOT}/ports/freertos/vtts_par_freertos.c"
            "${VTTS_ASSETS}/model_s.vtts"
            "${VTTS_ASSETS}/model_m.vtts"
            "${VTTS_ASSETS}/lex_en.bin")
        if(NOT EXISTS "${required}")
            message(FATAL_ERROR "vTTS assets missing: ${required}; run python tools/prepare_vtts_develop.py first")
        endif()
    endforeach()
    # ESP32-S3 is the only supported target. Do NOT add vtts_kernels_ref.c:
    # its three weak defaults can satisfy unresolved symbols from this static
    # archive before the S3/FreeRTOS objects are extracted. Without the defaults,
    # missing optimized implementations must fail the link instead of silently
    # producing a scalar/single-core firmware. Upstream files stay unmodified.
    set(JR_TTS_SOURCES
        "${VTTS_ROOT}/ports/esp32s3/vtts_port_s3.c"
        "${VTTS_ROOT}/ports/freertos/vtts_par_freertos.c"
        "${VTTS_ROOT}/src/vtts_model.c"
        "${VTTS_ROOT}/src/vtts_synth.c"
        "${VTTS_ROOT}/src/vtts_text.c"
        "${VTTS_ROOT}/src/vtts_text_en.c"
        "${VTTS_ROOT}/src/vtts_text_zh.c"
        "${VTTS_ROOT}/src/vtts_text_ko.c"
        "${VTTS_ROOT}/src/vtts_text_hi.c"
        "${VTTS_ROOT}/src/vtts_text_ja.c"
        "${VTTS_ROOT}/src/vtts_text_lt.c"
        "${VTTS_ROOT}/src/vtts_text_th.c"
        "${VTTS_ROOT}/src/vtts_text_fr.c"
        "${VTTS_ROOT}/src/vtts_text_de.c"
        "${VTTS_ROOT}/src/vtts_text_it.c"
        "${VTTS_ROOT}/src/vtts_text_tr.c"
        "${VTTS_ROOT}/src/vtts_re.c"
        "${VTTS_ROOT}/src/vtts_text_ru.c"
        "${VTTS_ROOT}/src/vtts_text_pl.c"
        "${VTTS_ROOT}/src/vtts_text_ar.c"
        "${VTTS_ROOT}/src/vtts_text_fi.c"
        "${VTTS_ROOT}/src/vtts_text_el.c"
        "${VTTS_ROOT}/src/vtts_text_tl.c"
        "${VTTS_ROOT}/src/vtts_text_sw.c"
        "${VTTS_ROOT}/src/vtts_text_nx.c"
        "${VTTS_ROOT}/src/vtts_text_nx_nl.c"
        "${VTTS_ROOT}/src/vtts_text_nx_sc.c"
        "${VTTS_ROOT}/src/vtts_text_da.c"
        "${VTTS_ROOT}/src/vtts_text_lo.c"
        "${VTTS_ROOT}/src/vtts_text_nsp.c"
        "${VTTS_ROOT}/src/vtts_text_km.c"
        "${VTTS_ROOT}/src/vtts_text_my.c"
        "${VTTS_ROOT}/src/vtts_lex.c")
    set(JR_TTS_INCLUDES "${VTTS_ROOT}/include" "${VTTS_ROOT}/src")
endif()

set(JR_STT_SOURCES "")
set(JR_STT_REQUIRES "")
if(JRBOT_ENABLE_BABYTALK)
    set(JR_STT_SOURCES "../module_brain/jr_stt.c")
    set(JR_STT_REQUIRES stt_engine sram_pool)
endif()

idf_component_register(
    SRCS
        "main.c"
        "../core/jr_wifi.c"
        "../core/jr_wifi_provisioning.c"
        "../core/jr_mdns.c"
        "../core/jr_wifi_scan.c"
        "../core/jr_net_diag.c"
        "../core/jr_commands.c"
        "../core/jr_memory_tasks.c"
        "../core/jr_memory_full.c"
        "../core/jr_mode_manager.c"
        "../core/jr_resource_manager.c"
        "../core/jr_usb_terminal.c"
        "../module_face/jr_face.c"
        "../module_portal/jr_portal.c"
        "../module_portal/jr_wifi_web.c"
        "../module_portal/jr_audio_receive.c"
        "../module_camera/jr_camera_diag.c"
        "../module_audio/jr_audio.c"
        "../module_audio/jr_mic.c"
        "../module_audio/jr_webrtc_audio.c"
        "../module_brain/jr_brain.c"
        "../module_brain/jr_voice.c"
        "../module_brain/jr_reply_audio.c"
        "../module_runtime/jr_runtime_api.c"
        "../module_tts/jr_tts.c"
        ${JR_TTS_SOURCES}
        ${JR_STT_SOURCES}
    INCLUDE_DIRS
        "."
        "../core"
        "../module_face"
        "../module_portal"
        "../module_camera"
        "../module_audio"
        "../module_brain"
        "../module_runtime"
        "../module_tts"
        ${JR_TTS_INCLUDES}
    REQUIRES ${JR_STT_REQUIRES} esp_driver_i2c esp_driver_uart esp_driver_usb_serial_jtag esp_driver_gpio esp_driver_i2s esp_wifi esp_event esp_netif esp_http_server esp_https_server esp_psram esp32-camera esp-sr nvs_flash log esp_timer heap mdns
    EMBED_TXTFILES
        "jrskill_app.js"
        "memory_panel.js"
        "wifi_panel.js"
        "voice_panel.js"
)

# Generated inventory stays in the ignored build directory, never the source tree.
set(jr_memory_inventory "${CMAKE_BINARY_DIR}/jr_memory_build.json")
if(NOT EXISTS "${jr_memory_inventory}")
    file(WRITE "${jr_memory_inventory}" "{\"v\":1,\"ready\":false}\n")
endif()
target_add_binary_data(${COMPONENT_LIB} "${jr_memory_inventory}" TEXT RENAME_TO jr_memory_build_json)
target_add_binary_data(${COMPONENT_LIB} "../core/jr_wifi_setup.html" TEXT RENAME_TO jr_wifi_setup_html)


if(JRBOT_ENABLE_VTTS)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE JRBOT_WITH_VTTS=1
        VTTS_ENABLE_PT=1
        VTTS_ENABLE_ZH=0 VTTS_ENABLE_KO=0 VTTS_ENABLE_JA=0 VTTS_ENABLE_HI=0
        VTTS_ENABLE_TH=0 VTTS_ENABLE_ES=0 VTTS_ENABLE_ID=0
        VTTS_ENABLE_FR=0 VTTS_ENABLE_DE=0 VTTS_ENABLE_IT=0 VTTS_ENABLE_TR=0
        VTTS_ENABLE_RU=0 VTTS_ENABLE_PL=0 VTTS_ENABLE_AR=0 VTTS_ENABLE_FI=0
        VTTS_ENABLE_EL=0 VTTS_ENABLE_MS=0 VTTS_ENABLE_TL=0 VTTS_ENABLE_SW=0
        VTTS_ENABLE_NL=0 VTTS_ENABLE_SV=0 VTTS_ENABLE_NO=0 VTTS_ENABLE_DA=0
        VTTS_ENABLE_LO=0 VTTS_ENABLE_KM=0 VTTS_ENABLE_MY=0
        VTTS_BLOCK=64 VTTS_HEAD_BLOCK=16 VTTS_STREAM_STEP=64)
    # Apply optimization only to vTTS and its wrapper. Do not change compiler
    # flags for the camera, Wi-Fi, WakeNet, OLED or WebRTC components.
    set_source_files_properties(${JR_TTS_SOURCES} "../module_tts/jr_tts.c"
        PROPERTIES COMPILE_OPTIONS "-O2;-fno-math-errno;-Wno-unused-function")
    target_add_binary_data(${COMPONENT_LIB} "${VTTS_ASSETS}/model_s.vtts" BINARY RENAME_TO model_s.vtts)
    target_add_binary_data(${COMPONENT_LIB} "${VTTS_ASSETS}/model_m.vtts" BINARY RENAME_TO model_m.vtts)
    target_add_binary_data(${COMPONENT_LIB} "${VTTS_ASSETS}/lex_en.bin" BINARY RENAME_TO lex_en.bin)
else()
    target_compile_definitions(${COMPONENT_LIB} PRIVATE JRBOT_WITH_VTTS=0)
endif()

if(JRBOT_ENABLE_BABYTALK)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE JRBOT_WITH_BABYTALK=1)
else()
    target_compile_definitions(${COMPONENT_LIB} PRIVATE JRBOT_WITH_BABYTALK=0)
endif()
