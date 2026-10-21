#pragma once
#include <stdbool.h>
#include <stddef.h>

/* Explicit diagnostic; may briefly pause scheduling while stacks are scanned. */
bool jr_memory_tasks_format(char *out, size_t cap);
