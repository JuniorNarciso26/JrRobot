#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* Serialized by the camera module; never called from the boot sequence. */
typedef struct {
    uint8_t *data;
    size_t len;
    unsigned pid;
    unsigned width;
    unsigned height;
} jr_camera_jpeg_t;

bool jr_camera_probe_once(unsigned *pid_out);
bool jr_camera_test_once(char *response, size_t capacity);
esp_err_t jr_camera_capture_jpeg(jr_camera_jpeg_t *out);
void jr_camera_jpeg_release(jr_camera_jpeg_t *frame);
