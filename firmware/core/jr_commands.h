#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#define JR_COMMAND_MAX_BYTES 768
#define JR_RESPONSE_MAX_BYTES 2048
#define JR_SERIAL_LINE_MAX_BYTES (JR_COMMAND_MAX_BYTES + 40)
esp_err_t jr_commands_init(void);
bool jr_handle_command(const char *cmd, char *response, size_t response_len);
bool jr_format_status(char *response, size_t response_len);
void jr_print_help(void);
void jr_terminal_start(void);
/* Bounded percent decoding. Rejects malformed sequences, NUL and controls. */
bool jr_decode_component(const char *src, size_t len, char *dst, size_t cap, bool plus_space);
/* Handles one line, including optional @id envelope; never echoes credentials. */
void jr_terminal_process_line(const char *line);
