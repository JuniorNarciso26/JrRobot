#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#ifndef CONFIG_HEAP_TASK_TRACKING
#define CONFIG_HEAP_TASK_TRACKING 1
#endif
#define CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE 32768
#define CONFIG_ESP32S3_DATA_CACHE_SIZE 65536
typedef struct { TaskHandle_t task; void *address; uint32_t size; } heap_task_block_t;
typedef struct {
    const char *name;
    uint32_t caps;
    size_t size, current_usage, peak_usage, alloc_count;
    heap_task_block_t *alloc_stat;
} heap_stat_t;
typedef struct {
    char name[16]; TaskHandle_t handle; bool is_alive;
    size_t overall_peak_usage, overall_current_usage, heap_count;
    heap_stat_t *heap_stat;
} task_stat_t;
typedef struct {
    size_t task_count; task_stat_t *stat_arr;
    size_t heap_count; heap_stat_t *heap_stat_start;
    size_t alloc_count; heap_task_block_t *alloc_stat_start;
} heap_all_tasks_stat_t;
esp_err_t heap_caps_get_all_task_stat(heap_all_tasks_stat_t *stats);
