#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_DMA 4
#define MALLOC_CAP_SPIRAM 8
#define MALLOC_CAP_RTCRAM 16
typedef struct { size_t total_free_bytes, total_allocated_bytes, largest_free_block, minimum_free_bytes; } multi_heap_info_t;
void heap_caps_get_info(multi_heap_info_t *info, uint32_t caps);
void *heap_caps_malloc(size_t size, uint32_t caps);
void heap_caps_free(void *ptr);
void *heap_caps_calloc(size_t count, size_t size, uint32_t caps);
typedef struct { intptr_t start, end; } walker_heap_into_t;
typedef struct { void *ptr; size_t size; bool used; } walker_block_info_t;
typedef bool (*heap_caps_walker_cb_t)(walker_heap_into_t, walker_block_info_t, void *);
void heap_caps_walk(uint32_t caps, heap_caps_walker_cb_t callback, void *arg);

