#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "jr_memory.h"
static unsigned calls;
void heap_caps_get_info(multi_heap_info_t *info, uint32_t caps) {
    const unsigned expected[] = {MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT};
    assert(caps == expected[calls++ % 3]);
    info->total_free_bytes = 4294967295U;
    info->total_allocated_bytes = 4294967295U;
    info->largest_free_block = 4294967295U;
    info->minimum_free_bytes = 4294967295U;
}
int64_t esp_timer_get_time(void) { return INT64_MAX; }
int main(void) {
    char response[640], tiny[8];
    assert(jr_memory_format(response, sizeof(response)));
    assert(strstr(response, "JR_MEMORY protocol=1 uptime_ms=9223372036854775 "));
    assert(strstr(response, "dma_free=4294967295"));
    assert(strstr(response, "psram_min=4294967295"));
    assert(!jr_memory_format(tiny, sizeof(tiny)));
    assert(tiny[7] == '\0');
    puts("PASS: capability masks, maximum field sizes and bounded formatting");
}

