include("${CMAKE_CURRENT_LIST_DIR}/../../vendor/babytalk/paths.cmake")
idf_component_register(SRCS "jr_stt_pool.c"
    INCLUDE_DIRS "${BABYTALK_ROOT}/components/sram_pool"
    REQUIRES mmrt heap)
