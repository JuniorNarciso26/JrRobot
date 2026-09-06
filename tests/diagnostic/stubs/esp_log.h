#pragma once
void diag_test_log(const char *, const char *, ...);
#define ESP_LOGI(tag,...) diag_test_log(tag,__VA_ARGS__)
#define ESP_LOGW(tag,...) diag_test_log(tag,__VA_ARGS__)
#define ESP_LOGE(tag,...) diag_test_log(tag,__VA_ARGS__)
