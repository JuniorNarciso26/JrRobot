#include "jr_memory_tasks.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "jr_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define JR_MEMORY_MAX_TASKS 48

#if configUSE_TRACE_FACILITY == 1

/* IDs (index + 1) shared with memory_panel.js. Budgets are configured bytes,
 * not runtime allocation sizes; 0 means not known. No TCB pointers are read.
 * Names returned by uxTaskGetSystemState can become invalid after deletion;
 * identify known tasks by handles looked up by the RTOS instead. */
static const struct { const char *name; unsigned budget; } catalog[] = {
    {"main", CONFIG_ESP_MAIN_TASK_STACK_SIZE},
    {"terminal", 8192}, {"usb_terminal", 6144}, {"httpd", 0},
    {"mdns", CONFIG_MDNS_TASK_STACK_SIZE}, {"mdns_task", CONFIG_MDNS_TASK_STACK_SIZE},
    {"jr_wa_peer", 25 * 1024}, {"jr_wa_mic", 8192},
    {"jr_wa_play", 12288}, {"jr_wa_video", 6144}, {"jr_wa_signal", 6144},
    {"jr_mic_stream", 12288}, {"jr_mic_ws", 12288},
    {"wifi", 0}, {"tiT", 0}, {"sys_evt", 0}, {"esp_timer", 0},
    {"Tmr Svc", 0}, {"IDLE0", 0}, {"IDLE1", 0},
    {"ipc0", 0}, {"ipc1", 0}, {"cam_task", 0}
};

static bool append(char *out, size_t cap, size_t *used, const char *format, ...) {
    if (*used >= cap) return false;
    va_list args;
    va_start(args, format);
    int n = vsnprintf(out + *used, cap - *used, format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= cap - *used) return false;
    *used += (size_t)n;
    return true;
}
#endif

bool jr_memory_tasks_format(char *out, size_t cap) {
#if configUSE_TRACE_FACILITY == 1
    TaskHandle_t handles[sizeof(catalog) / sizeof(catalog[0])];
    /* PSRAM only: this diagnostic must not exhaust the internal DMA heap. */
    const size_t workspace = JR_MEMORY_MAX_TASKS * sizeof(TaskStatus_t);
    TaskStatus_t *tasks = heap_caps_malloc(workspace, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!tasks) {
        snprintf(out, cap, "JR_ERROR tasks_psram_unavailable");
        return false;
    }
    for (size_t i = 0; i < sizeof(catalog) / sizeof(catalog[0]); ++i) {
        handles[i] = xTaskGetHandle(catalog[i].name);
    }
    int64_t start = esp_timer_get_time();
    UBaseType_t count = uxTaskGetSystemState(tasks, JR_MEMORY_MAX_TASKS, NULL);
    unsigned elapsed = (unsigned)(esp_timer_get_time() - start);
    if (!count) {
        heap_caps_free(tasks);
        snprintf(out, cap, "JR_ERROR tasks_snapshot_changed_or_over_limit limit=%u", JR_MEMORY_MAX_TASKS);
        return false;
    }
    size_t used = 0;
    bool ok = append(out, cap, &used,
        "JR_TASKS {\"v\":1,\"version\":\"%s\",\"uptime_ms\":%llu,\"total\":%u,\"scan_us\":%u,\"workspace_bytes\":%u,\"rows\":[",
        JR_APP_VERSION, (unsigned long long)(esp_timer_get_time() / 1000),
        (unsigned)count, elapsed, (unsigned)workspace);
    for (UBaseType_t i = 0; i < count && ok; ++i) {
        unsigned code = 0, budget = 0;
        if (tasks[i].eCurrentState != eDeleted) {
            for (size_t j = 0; j < sizeof(catalog) / sizeof(catalog[0]); ++j) {
                if (handles[j] && handles[j] == tasks[i].xHandle) {
                    code = (unsigned)j + 1;
                    budget = catalog[j].budget;
                    break;
                }
            }
        }
        unsigned memory = esp_ptr_external_ram(tasks[i].pxStackBase) ? 2 :
                          esp_ptr_internal(tasks[i].pxStackBase) ? 1 : 0;
        ok = append(out, cap, &used, "%s[%u,%u,%u,%u,%u,%u,%u]",
            i ? "," : "", (unsigned)tasks[i].xTaskNumber, code,
            (unsigned)tasks[i].eCurrentState, (unsigned)tasks[i].uxCurrentPriority,
            (unsigned)(tasks[i].usStackHighWaterMark * sizeof(StackType_t)), budget, memory);
    }
    heap_caps_free(tasks);
    if (ok) ok = append(out, cap, &used, "]}");
    if (!ok) snprintf(out, cap, "JR_ERROR tasks_response_too_large");
    return ok;
#else
    snprintf(out, cap, "JR_ERROR tasks_trace_disabled reinstall_STEP03D");
    return false;
#endif
}
