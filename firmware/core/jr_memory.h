#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "jr_config.h"

/* Read only: no peripheral probes, background tasks or heap allocations. */
static inline bool jr_memory_format(char *out, size_t cap) {
    multi_heap_info_t internal, dma, psram;
    heap_caps_get_info(&internal, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    heap_caps_get_info(&dma, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    heap_caps_get_info(&psram, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    int n = snprintf(out, cap,
        "JR_MEMORY protocol=1 uptime_ms=%llu version=%s "
        "internal_free=%u internal_allocated=%u internal_largest=%u internal_min=%u "
        "dma_free=%u dma_allocated=%u dma_largest=%u dma_min=%u "
        "psram_free=%u psram_allocated=%u psram_largest=%u psram_min=%u",
        (unsigned long long)(esp_timer_get_time() / 1000),
        JR_APP_VERSION,
        (unsigned)internal.total_free_bytes, (unsigned)internal.total_allocated_bytes,
        (unsigned)internal.largest_free_block, (unsigned)internal.minimum_free_bytes,
        (unsigned)dma.total_free_bytes, (unsigned)dma.total_allocated_bytes,
        (unsigned)dma.largest_free_block, (unsigned)dma.minimum_free_bytes,
        (unsigned)psram.total_free_bytes, (unsigned)psram.total_allocated_bytes,
        (unsigned)psram.largest_free_block, (unsigned)psram.minimum_free_bytes);
    return n >= 0 && (size_t)n < cap;
}
