#pragma once
#include <stdbool.h>
#include <stddef.h>
bool jr_wifi_serial_scan(bool start,char *out,size_t cap);
bool jr_wifi_serial_profiles(char *out,size_t cap);
bool jr_wifi_serial_delete(const char *ssid,char *out,size_t cap);
bool jr_wifi_serial_join(const char *ssid,const char *password,char *out,size_t cap);
