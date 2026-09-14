#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    JR_CAMERA_SIZE_QVGA = 0,
    JR_CAMERA_SIZE_VGA,
    JR_CAMERA_SIZE_SVGA,
    JR_CAMERA_SIZE_XGA,
    JR_CAMERA_SIZE_HD,
    JR_CAMERA_SIZE_UXGA,
} jr_camera_size_t;

typedef struct {
    uint8_t *data;
    size_t len;
    unsigned pid;
    unsigned width;
    unsigned height;
    int jpeg_quality;
    jr_camera_size_t size;
} jr_camera_jpeg_t;

bool jr_camera_probe_once(unsigned *pid_out);
bool jr_camera_test_once(char *response, size_t capacity);
bool jr_camera_parse_size(const char *name,jr_camera_size_t *out);
const char *jr_camera_size_name(jr_camera_size_t size);
esp_err_t jr_camera_capture_jpeg(jr_camera_jpeg_t *out);
esp_err_t jr_camera_capture_jpeg_ex(jr_camera_jpeg_t *out,jr_camera_size_t size,int jpeg_quality);
void jr_camera_jpeg_release(jr_camera_jpeg_t *frame);
