/* BabyTalk-only provider: reserve its exact SIMD arena on demand, return it
 * when mmrt_s3_deinit releases ownership. All calls belong to the voice task. */
#include "sram_pool.h"
#include "mmrt_s3.h"
#include "esp_heap_caps.h"
static void *arena;
static int owner;
static sram_pool_evict_fn evict_owner;
int sram_pool_reserve(void) { return 0; } /* no permanent boot reservation */
void *sram_pool_acquire(int next, sram_pool_evict_fn evict) {
    if (owner && owner!=next) {
        if (evict_owner && evict_owner()) return NULL;
        if (owner) sram_pool_release(owner);
    }
    if (!arena) arena=heap_caps_aligned_alloc(16,MMRT_S3_BUFFER_BYTES,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    if (!arena) return NULL;
    owner=next; evict_owner=evict;
    return arena;
}
void sram_pool_release(int previous) {
    if (owner!=previous) return;
    heap_caps_free(arena); arena=NULL; owner=0; evict_owner=NULL;
}
