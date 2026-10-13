#include "sdk_mock.h"
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_DMA 4
#define MALLOC_CAP_SPIRAM 8
size_t heap_caps_get_free_size(unsigned);
size_t heap_caps_get_largest_free_block(unsigned);
