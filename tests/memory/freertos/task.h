#pragma once
#include "FreeRTOS.h"
TaskHandle_t xTaskGetHandle(const char *name);
UBaseType_t uxTaskGetSystemState(TaskStatus_t *tasks, UBaseType_t cap, void *runtime);

