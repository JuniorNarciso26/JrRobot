#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum { JR_OLED_UNINITIALIZED, JR_OLED_READY, JR_OLED_DEGRADED, JR_OLED_RECOVERING, JR_OLED_OFFLINE } jr_oled_state_t;
typedef struct {
    jr_oled_state_t state;
    char expression[20];
    bool demo;
    uint8_t address;
    uint32_t commands, rendered, tx_ok, tx_failed, skipped, init_failed;
    uint32_t consecutive_failures, recoveries, last_success_ms;
    esp_err_t last_error;
} jr_face_status_t;

/* Start/step/loop belong to one owner task. Other tasks only use snapshots/setters. */
esp_err_t jr_face_start(void);
void jr_face_step(uint32_t ms);
void jr_face_loop(void);
void jr_face_get_status(jr_face_status_t *out);
const char *jr_face_state_name(jr_oled_state_t state);
bool jr_face_set_expression(const char *command);
const char *jr_face_expression_name(void);
void jr_face_set_demo(bool enabled);
bool jr_face_demo_enabled(void);
uint32_t jr_face_command_count(void);
uint32_t jr_face_frame_count(void);
void jr_face_increment_command_count(void);
void jr_face_print_status(void);
