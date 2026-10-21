#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_heap_task_info.h"
#include "jr_memory_full.h"
static unsigned allocated, released;
static int fail_alloc, fail_tracking;
void *heap_caps_malloc(size_t size, uint32_t caps) {
    assert(caps == (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (fail_alloc) return NULL;
    allocated++; return malloc(size);
}
void *heap_caps_calloc(size_t count, size_t size, uint32_t caps) {
    void *ptr = heap_caps_malloc(count * size, caps);
    if (ptr) memset(ptr, 0, count * size);
    return ptr;
}
void heap_caps_free(void *ptr) { if (ptr) { released++; free(ptr); } }
int64_t esp_timer_get_time(void) { static int64_t t = 123000; return t += 100; }
void heap_caps_walk(uint32_t caps, heap_caps_walker_cb_t callback, void *arg) {
    assert(caps == MALLOC_CAP_INTERNAL);
    walker_heap_into_t main_heap = {0x3fc9c000, 0x3fcf0000};
    walker_heap_into_t rtc = {0x600fe000, 0x60100000};
    callback(main_heap, (walker_block_info_t){(void *)0x3fc9c010, 65536, true}, arg);
    callback(main_heap, (walker_block_info_t){(void *)0x3fcac010, 278400, false}, arg);
    callback(rtc, (walker_block_info_t){(void *)0x600fe010, 1024, true}, arg);
    callback(rtc, (walker_block_info_t){(void *)0x600fe410, 7040, false}, arg);
}
esp_err_t heap_caps_get_all_task_stat(heap_all_tasks_stat_t *stats) {
    if (fail_tracking) return ESP_FAIL;
    assert(stats->task_count == 64 && stats->heap_count == 256 && stats->alloc_count == 4096);
    stats->task_count = 2; stats->heap_count = 2; stats->alloc_count = 2;
    strcpy(stats->stat_arr[0].name, "httpd\"\\\n");
    stats->stat_arr[0].handle = (void *)0x3fc9c010;
    stats->stat_arr[0].is_alive = true;
    stats->stat_arr[0].overall_current_usage = 65500;
    stats->stat_arr[0].overall_peak_usage = 80000;
    stats->stat_arr[0].heap_count = 1;
    stats->stat_arr[0].heap_stat = &stats->heap_stat_start[0];
    stats->heap_stat_start[0] = (heap_stat_t){.caps=MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA,
        .current_usage=65500, .peak_usage=80000, .alloc_count=1, .alloc_stat=&stats->alloc_stat_start[0]};
    stats->alloc_stat_start[0] = (heap_task_block_t){.address=(void *)0x3fc9c010, .size=65500};
    strcpy(stats->stat_arr[1].name, "Pre-scheduler");
    stats->stat_arr[1].overall_current_usage = 1000;
    stats->stat_arr[1].heap_count = 1;
    stats->stat_arr[1].heap_stat = &stats->heap_stat_start[1];
    stats->heap_stat_start[1] = (heap_stat_t){.caps=MALLOC_CAP_INTERNAL|MALLOC_CAP_RTCRAM,
        .current_usage=1000, .peak_usage=2000, .alloc_count=1, .alloc_stat=&stats->alloc_stat_start[1]};
    stats->alloc_stat_start[1] = (heap_task_block_t){.address=(void *)0x600fe010, .size=1000};
    return ESP_OK;
}
int main(void) {
    char *out; size_t length;
    assert(jr_memory_full_json(NULL, &length) == ESP_ERR_INVALID_ARG);
    fail_alloc = 1;
    assert(jr_memory_full_json(&out, &length) == ESP_ERR_NO_MEM);
    assert(!out && length == 0 && allocated == released);
    fail_alloc = 0;
#if CONFIG_HEAP_TASK_TRACKING
    fail_tracking = 1;
    assert(jr_memory_full_json(&out, &length) == ESP_FAIL);
    assert(!out && allocated == released);
    fail_tracking = 0;
#endif
    assert(jr_memory_full_json(&out, &length) == ESP_OK);
    assert(out && length == strlen(out));
    puts(out);
    heap_caps_free(out);
    assert(allocated == released);
}
