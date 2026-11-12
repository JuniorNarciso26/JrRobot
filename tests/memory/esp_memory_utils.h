#pragma once
#include <stdbool.h>
#include <stdint.h>
static inline bool esp_ptr_external_ram(const void *ptr) { return (uintptr_t)ptr == 0x2000; }
static inline bool esp_ptr_internal(const void *ptr) { return (uintptr_t)ptr == 0x1000; }

