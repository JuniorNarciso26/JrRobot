#pragma once
#include <stdbool.h>
#include <stddef.h>
/* Serialized by the command router; never called from the boot sequence. */
bool jr_camera_probe_once(unsigned *pid_out);
bool jr_camera_test_once(char *response, size_t capacity);
