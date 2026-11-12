#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jr_memory_tasks.h"
#include "esp_heap_caps.h"
#include "freertos/task.h"
static unsigned allocated, released;
static bool fail_alloc, fail_snapshot;
void *heap_caps_malloc(size_t size, uint32_t caps) {
    assert(caps == (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (fail_alloc) return NULL;
    allocated++;
    return malloc(size);
}
void heap_caps_free(void *ptr) { assert(ptr); released++; free(ptr); }
int64_t esp_timer_get_time(void) { static int64_t t; return t += 100; }
TaskHandle_t xTaskGetHandle(const char *name) {
    if (!strcmp(name, "main")) return (void *)1;
    if (!strcmp(name, "jr_wa_peer")) return (void *)2;
    return NULL;
}
UBaseType_t uxTaskGetSystemState(TaskStatus_t *tasks, UBaseType_t cap, void *runtime) {
    assert(cap == 48); assert(!runtime);
    if (fail_snapshot) return 0;
    tasks[0] = (TaskStatus_t){.xHandle=(void *)1, .pcTaskName=(void *)1,
        .xTaskNumber=7, .eCurrentState=eBlocked, .uxCurrentPriority=1,
        .pxStackBase=(void *)0x1000, .usStackHighWaterMark=2048};
    tasks[1] = (TaskStatus_t){.xHandle=(void *)2, .pcTaskName=(void *)1,
        .xTaskNumber=9, .eCurrentState=eReady, .uxCurrentPriority=18,
        .pxStackBase=(void *)0x1000, .usStackHighWaterMark=8192};
    tasks[2] = (TaskStatus_t){.xHandle=(void *)3, .pcTaskName=(void *)1,
        .xTaskNumber=11, .eCurrentState=eDeleted, .uxCurrentPriority=1,
        .pxStackBase=(void *)0x2000, .usStackHighWaterMark=512};
    return 3;
}
int main(void) {
    char response[2048];
#if configUSE_TRACE_FACILITY == 1
    assert(jr_memory_tasks_format(response, sizeof(response)));
    assert(strstr(response, "[7,1,2,1,2048,6144,1]"));
    assert(strstr(response, "[9,7,1,18,8192,25600,1]"));
    assert(strstr(response, "[11,0,4,1,512,0,2]"));
    /* Deliberately invalid pcTaskName pointers ensure we never read TCB names. */
    assert(allocated == released);
    assert(!jr_memory_tasks_format(response, 64));
    assert(strstr(response, "tasks_response_too_large"));
    assert(allocated == released);
    fail_snapshot = true;
    assert(!jr_memory_tasks_format(response, sizeof(response)));
    assert(strstr(response, "tasks_snapshot_changed_or_over_limit"));
    assert(allocated == released);
    fail_snapshot = false; fail_alloc = true;
    assert(!jr_memory_tasks_format(response, sizeof(response)));
    assert(strstr(response, "tasks_psram_unavailable"));
    assert(allocated == released);
#else
    assert(!jr_memory_tasks_format(response, sizeof(response)));
    assert(strstr(response, "tasks_trace_disabled"));
    assert(!allocated);
#endif
    puts("PASS: task snapshot, known/unknown tasks, PSRAM-only workspace, cleanup and bounded responses");
}

