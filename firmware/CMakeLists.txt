cmake_minimum_required(VERSION 3.16)
set(IDF_TARGET "esp32s3" CACHE STRING "JrBot target")
if(NOT IDF_TARGET STREQUAL "esp32s3")
    message(FATAL_ERROR "JrBot requires esp32s3")
endif()
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/version.txt" PROJECT_VER LIMIT_COUNT 1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/version.txt")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(jrbot_v2)

# project() has now created the final ELF target (ESP-IDF EXECUTABLE property).
# Validate the selected implementations, not just the source file list.
if(JRBOT_ENABLE_VTTS)
    idf_build_get_property(jr_tts_elf EXECUTABLE)
    idf_build_get_property(jr_tts_python PYTHON)
    set(jr_tts_check "${CMAKE_CURRENT_LIST_DIR}/../tools/check_tts_acceleration.py")
    set(jr_tts_inspect "${CMAKE_CURRENT_LIST_DIR}/../tools/tts_build_diagnostics.py")
    set_property(TARGET ${jr_tts_elf} APPEND PROPERTY LINK_DEPENDS "${jr_tts_check}" "${jr_tts_inspect}")
    add_custom_command(TARGET ${jr_tts_elf} POST_BUILD
        COMMAND "${jr_tts_python}" "${jr_tts_check}" --elf "$<TARGET_FILE:${jr_tts_elf}>"
        VERBATIM)
endif()
