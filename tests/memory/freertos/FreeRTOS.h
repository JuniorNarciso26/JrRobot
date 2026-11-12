#pragma once
#include <stdint.h>
#include <stddef.h>
#ifndef configUSE_TRACE_FACILITY
#define configUSE_TRACE_FACILITY 1
#endif
#define CONFIG_ESP_MAIN_TASK_STACK_SIZE 6144
#define CONFIG_MDNS_TASK_STACK_SIZE 4096
typedef unsigned UBaseType_t;
typedef uint8_t StackType_t;
typedef void *TaskHandle_t;
typedef enum {eRunning, eReady, eBlocked, eSuspended, eDeleted, eInvalid} eTaskState;
typedef struct {
    TaskHandle_t xHandle;
    const char *pcTaskName;
    UBaseType_t xTaskNumber;
    eTaskState eCurrentState;
    UBaseType_t uxCurrentPriority;
    StackType_t *pxStackBase;
    unsigned usStackHighWaterMark;
} TaskStatus_t;

