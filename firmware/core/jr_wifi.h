#pragma once
#include <stdbool.h>

void jr_wifi_start(void);
bool jr_wifi_is_connected(void);
const char *jr_wifi_ip(void);
