#include "sdk_mock.h"

#define ESP_LOGI(tag,...) do { (void)(tag); printf(__VA_ARGS__); puts(""); } while(0)
