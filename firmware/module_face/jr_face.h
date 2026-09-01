#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t jr_face_start(void);
void jr_face_loop(void);
bool jr_face_set_expression(const char *command);
const char *jr_face_expression_name(void);
void jr_face_set_demo(bool enabled);
bool jr_face_demo_enabled(void);
uint32_t jr_face_command_count(void);
uint32_t jr_face_frame_count(void);
void jr_face_increment_command_count(void);
void jr_face_print_status(void);
