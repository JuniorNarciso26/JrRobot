#pragma once
#include <stdbool.h>
#include <stddef.h>

bool jr_handle_command(const char *cmd, char *response, size_t response_len);
void jr_terminal_start(void);
void jr_print_help(void);
