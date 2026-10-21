#include "jr_memory_full.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_heap_task_info.h"
#include "esp_timer.h"
#include "jr_config.h"

#ifndef CONFIG_HEAP_TASK_TRACKING
#define CONFIG_HEAP_TASK_TRACKING 0
#endif

#define FULL_TASKS 64
#define FULL_HEAPS 256
#define FULL_ALLOCS 4096
#define FULL_REGIONS 32
#define FULL_JSON_CAP (256 * 1024)

typedef struct {
    uintptr_t start, end;
    size_t used, free, used_blocks, free_blocks, largest;
    unsigned kind;
} region_t;

typedef struct {
    region_t regions[FULL_REGIONS];
    size_t region_count;
    bool region_overflow;
#if CONFIG_HEAP_TASK_TRACKING
    task_stat_t tasks[FULL_TASKS];
    heap_stat_t heaps[FULL_HEAPS];
    heap_task_block_t blocks[FULL_ALLOCS];
#endif
} workspace_t;

typedef struct { char *data; size_t used, cap; bool ok; } json_t;

static void add(json_t *json, const char *format, ...) {
    if (!json->ok) return;
    va_list args;
    va_start(args, format);
    int n = vsnprintf(json->data + json->used, json->cap - json->used, format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= json->cap - json->used) json->ok = false;
    else json->used += (size_t)n;
}

#if CONFIG_HEAP_TASK_TRACKING
static void string(json_t *json, const char *text, size_t limit) {
    add(json, "\"");
    for (size_t i = 0; i < limit && text[i] && json->ok; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c == '"' || c == '\\') add(json, "\\%c", c);
        else if (c < 32) add(json, "\\u%04x", (unsigned)c);
        else add(json, "%c", c);
    }
    add(json, "\"");
}
#endif

static unsigned region_kind(uintptr_t start) {
    if ((start >= 0x3fc88000 && start < 0x3fd00000) ||
        (start >= 0x40370000 && start < 0x403e0000)) return 1; /* Main SRAM */
#if CONFIG_ESP32S3_DATA_CACHE_16KB
    if (start >= 0x3c000000 && start < 0x3c004000) return 1;
#endif
    if ((start >= 0x600fe000 && start < 0x60100000) ||
        (start >= 0x50000000 && start < 0x50002000)) return 2; /* RTC, separate */
    return 0;
}

/* Called under the allocator's heap lock: arithmetic only, no allocation,
 * output, RTOS calls, or reading application-owned payloads. */
static bool collect_region(walker_heap_into_t heap, walker_block_info_t block, void *arg) {
    workspace_t *work = arg;
    size_t i;
    for (i = 0; i < work->region_count; ++i) {
        if (work->regions[i].start == (uintptr_t)heap.start) break;
    }
    if (i == work->region_count) {
        if (i == FULL_REGIONS) { work->region_overflow = true; return false; }
        work->region_count++;
        work->regions[i].start = (uintptr_t)heap.start;
        work->regions[i].end = (uintptr_t)heap.end;
        work->regions[i].kind = region_kind((uintptr_t)heap.start);
    }
    region_t *region = &work->regions[i];
    if (block.used) { region->used += block.size; region->used_blocks++; }
    else {
        region->free += block.size;
        region->free_blocks++;
        if (block.size > region->largest) region->largest = block.size;
    }
    return true;
}

extern char _iram_start[], _iram_end[], _data_start[], _heap_start[];
extern char _data_end[], _bss_start[], _bss_end[], _noinit_start[], _noinit_end[];

esp_err_t jr_memory_full_json(char **out, size_t *length) {
    if (!out || !length) return ESP_ERR_INVALID_ARG;
    *out = NULL; *length = 0;
    workspace_t *work = heap_caps_calloc(1, sizeof(*work), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char *buffer = heap_caps_malloc(FULL_JSON_CAP, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!work || !buffer) {
        heap_caps_free(work); heap_caps_free(buffer);
        return ESP_ERR_NO_MEM;
    }
    int64_t start = esp_timer_get_time();
#if CONFIG_HEAP_TASK_TRACKING
    heap_all_tasks_stat_t stats = {
        .task_count = FULL_TASKS, .stat_arr = work->tasks,
        .heap_count = FULL_HEAPS, .heap_stat_start = work->heaps,
        .alloc_count = FULL_ALLOCS, .alloc_stat_start = work->blocks
    };
    esp_err_t err = heap_caps_get_all_task_stat(&stats);
    if (err != ESP_OK) {
        heap_caps_free(work); heap_caps_free(buffer);
        return err;
    }
#endif
    heap_caps_walk(MALLOC_CAP_INTERNAL, collect_region, work);
    size_t extent = 0, used = 0, free_bytes = 0, rtc_extent = 0;
    for (size_t i = 0; i < work->region_count; ++i) {
        region_t *r = &work->regions[i];
        if (r->kind == 1) { extent += r->end - r->start; used += r->used; free_bytes += r->free; }
        if (r->kind == 2) rtc_extent += r->end - r->start;
    }
    size_t iram = (uintptr_t)_iram_end - (uintptr_t)_iram_start;
    size_t dram = (uintptr_t)_heap_start - (uintptr_t)_data_start;
    size_t icache = CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE;
    size_t dcache = CONFIG_ESP32S3_DATA_CACHE_SIZE;
    int64_t admin = (int64_t)extent - (int64_t)used - (int64_t)free_bytes;
    int64_t residual = 512 * 1024 - (int64_t)icache - (int64_t)dcache - (int64_t)iram - (int64_t)dram - (int64_t)extent;
    json_t json = {.data = buffer, .cap = FULL_JSON_CAP, .ok = true};
    add(&json, "{\"v\":1,\"version\":\"%s\",\"uptime_ms\":%llu,\"diagnostic_psram_bytes\":%u,",
        JR_APP_VERSION, (unsigned long long)(esp_timer_get_time() / 1000), (unsigned)(sizeof(*work) + FULL_JSON_CAP));
    add(&json, "\"layout\":{\"physical_sram\":524288,\"icache\":%u,\"dcache\":%u,\"iram_image\":%u,\"dram_image\":%u,"
        "\"data\":%u,\"bss\":%u,\"noinit\":%u,\"heap_extent\":%u,\"heap_allocated_blocks\":%u,\"heap_free_blocks\":%u,"
        "\"heap_admin\":%lld,\"unclassified\":%lld,\"rtc_heap_extent\":%u,\"region_overflow\":%s},\"regions\":[",
        (unsigned)icache, (unsigned)dcache, (unsigned)iram, (unsigned)dram,
        (unsigned)((uintptr_t)_data_end - (uintptr_t)_data_start),
        (unsigned)((uintptr_t)_bss_end - (uintptr_t)_bss_start),
        (unsigned)((uintptr_t)_noinit_end - (uintptr_t)_noinit_start),
        (unsigned)extent, (unsigned)used, (unsigned)free_bytes,
        (long long)admin, (long long)residual, (unsigned)rtc_extent,
        work->region_overflow ? "true" : "false");
    for (size_t i = 0; i < work->region_count; ++i) {
        const region_t *r = &work->regions[i];
        add(&json, "%s{\"start\":%u,\"end\":%u,\"kind\":%u,\"used\":%u,\"free\":%u,\"largest\":%u,\"used_blocks\":%u,\"free_blocks\":%u}",
            i ? "," : "", (unsigned)r->start, (unsigned)r->end, r->kind,
            (unsigned)r->used, (unsigned)r->free, (unsigned)r->largest, (unsigned)r->used_blocks, (unsigned)r->free_blocks);
    }
    add(&json, "],\"tracking_enabled\":%s,\"tasks\":[", CONFIG_HEAP_TASK_TRACKING ? "true" : "false");
    bool complete = !work->region_overflow;
#if CONFIG_HEAP_TASK_TRACKING
    if (stats.task_count == FULL_TASKS || stats.heap_count == FULL_HEAPS || stats.alloc_count == FULL_ALLOCS) complete = false;
    for (size_t i = 0; i < stats.task_count && json.ok; ++i) {
        const task_stat_t *task = &stats.stat_arr[i];
        add(&json, "%s{\"name\":", i ? "," : "");
        string(&json, task->name, sizeof(task->name));
        add(&json, ",\"handle\":%u,\"alive\":%s,\"current_requested\":%u,\"peak_requested\":%u,\"heaps\":[",
            (unsigned)(uintptr_t)task->handle, task->is_alive ? "true" : "false",
            (unsigned)task->overall_current_usage, (unsigned)task->overall_peak_usage);
        if (task->heap_count && !task->heap_stat) complete = false;
        for (size_t h = 0; task->heap_stat && h < task->heap_count && json.ok; ++h) {
            const heap_stat_t *heap = &task->heap_stat[h];
            bool blocks_complete = !heap->alloc_count || heap->alloc_stat != NULL;
            for (size_t a = 0; heap->alloc_stat && a < heap->alloc_count; ++a) {
                if (!heap->alloc_stat[a].address || !heap->alloc_stat[a].size) blocks_complete = false;
            }
            add(&json, "%s{\"caps\":%u,\"current\":%u,\"peak\":%u,\"alloc_count\":%u,\"blocks_complete\":%s,\"blocks\":[",
                h ? "," : "", (unsigned)heap->caps, (unsigned)heap->current_usage, (unsigned)heap->peak_usage,
                (unsigned)heap->alloc_count, blocks_complete ? "true" : "false");
            if (!blocks_complete) complete = false;
            size_t emitted = 0;
            for (size_t a = 0; heap->alloc_stat && a < heap->alloc_count && json.ok; ++a) {
                if (!heap->alloc_stat[a].address || !heap->alloc_stat[a].size) continue;
                add(&json, "%s[%u,%u]", emitted++ ? "," : "",
                    (unsigned)(uintptr_t)heap->alloc_stat[a].address, (unsigned)heap->alloc_stat[a].size);
            }
            add(&json, "]}");
        }
        add(&json, "]}");
    }
#else
    complete = false;
#endif
    add(&json, "],\"complete\":%s,\"scan_us\":%llu,\"caps\":{\"internal\":%u,\"dma\":%u,\"psram\":%u,\"rtc\":%u}}",
        complete ? "true" : "false", (unsigned long long)(esp_timer_get_time() - start),
        (unsigned)MALLOC_CAP_INTERNAL, (unsigned)MALLOC_CAP_DMA, (unsigned)MALLOC_CAP_SPIRAM, (unsigned)MALLOC_CAP_RTCRAM);
    heap_caps_free(work);
    if (!json.ok) { heap_caps_free(buffer); return ESP_ERR_INVALID_SIZE; }
    *out = buffer; *length = json.used;
    return ESP_OK;
}
