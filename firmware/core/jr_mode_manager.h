#pragma once

#include <stdint.h>

typedef enum {
    JR_MODE_IDLE = 0,
    JR_MODE_AUTONOMOUS,
    JR_MODE_LIVE,
    JR_MODE_RECORDING,
    JR_MODE_PLAYBACK,
    JR_MODE_DIAGNOSTIC,
} jr_mode_t;

typedef struct {
    jr_mode_t current;
    jr_mode_t previous;
    uint32_t transitions;
} jr_mode_status_t;

jr_mode_t jr_mode_get(void);
jr_mode_t jr_mode_set(jr_mode_t next, const char *reason);
const char *jr_mode_name(jr_mode_t mode);
void jr_mode_get_status(jr_mode_status_t *out);
