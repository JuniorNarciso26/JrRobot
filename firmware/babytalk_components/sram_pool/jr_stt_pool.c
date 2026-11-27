/* BabyTalk-only provider: reserve its exact SIMD arena on demand, return it
 * when mmrt_s3_deinit releases ownership. All calls belong to the voice task. */
#include "sram_pool.h"
#include "mmrt_s3.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
static void *arena;
static int owner;
static sram_pool_evict_fn evict_owner;
int sram_pool_reserve(void) { return 0; } /* no permanent boot reservation */
void *sram_pool_acquire(int next, sram_pool_evict_fn evict) {
    if (owner && owner!=next) {
        if (evict_owner && evict_owner()) return NULL;
        if (owner) sram_pool_release(owner);
    }
    if (!arena) {
        const char *memory="internal";
        if (heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)>=MMRT_S3_BUFFER_BYTES &&
            heap_caps_get_free_size(MALLOC_CAP_INTERNAL)>=MMRT_S3_BUFFER_BYTES+32768)
            arena=heap_caps_aligned_alloc(16,MMRT_S3_BUFFER_BYTES,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
        if (!arena) {
            /* The upstream SRAM pool supports PSRAM too. Keep internal memory
             * for the worker/FFT when no sufficiently large block is available. */
            memory="psram";
            arena=heap_caps_aligned_alloc(16,MMRT_S3_BUFFER_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
        }
        if (!arena) return NULL;
        ESP_LOGI("jrbot_stt","JR_STT_ARENA memory=%s bytes=%u",memory,(unsigned)MMRT_S3_BUFFER_BYTES);
        (void)memory;
    }
    owner=next; evict_owner=evict;
    return arena;
}
void sram_pool_release(int previous) {
    if (owner!=previous) return;
    heap_caps_free(arena); arena=NULL; owner=0; evict_owner=NULL;
}
