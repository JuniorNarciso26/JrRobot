/* Exercise actual buffered synthesis/playback sequencing with fake inference
 * and a recording I2S sink. Does not simulate neural inference or ESP32 timing. */
#define JR_TTS_TWO_BLOCK_HOLD_MS 0U
#define JRBOT_WITH_VTTS 1
#include "../../firmware/module_tts/jr_tts.c"
#include <assert.h>
#include <stdatomic.h>

static _Atomic size_t writes, captured_count;
static int16_t captured[400000];
static int fail_write, fail_task, fail_alloc;
static _Atomic bool engine_done;
static bool pipeline_mode, late_second, fail_second;
static size_t largest_block = 4*1024*1024;
static unsigned text_calls;
static const char *expected_text;
static _Atomic int allocations, channels;
const char *esp_err_to_name(esp_err_t e) {(void)e; return "host";}
int64_t esp_timer_get_time(void) {struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1000000LL+t.tv_nsec/1000;}
void *heap_caps_malloc(size_t n, int caps) {(void)caps; if(fail_alloc)return NULL; void *p=malloc(n); if(p)allocations++; return p;}
void *heap_caps_calloc(size_t n,size_t bytes,int caps) {void *p=heap_caps_malloc(n*bytes,caps);if(p)memset(p,0,n*bytes);return p;}
void heap_caps_free(void *p) {if(p)allocations--; free(p);}
SemaphoreHandle_t xSemaphoreCreateBinary(void) {SemaphoreHandle_t s=calloc(1,sizeof(*s)); pthread_mutex_init(&s->lock,0);pthread_cond_init(&s->cv,0);return s;}
int xSemaphoreGive(SemaphoreHandle_t s) {pthread_mutex_lock(&s->lock);s->count++;pthread_cond_signal(&s->cv);pthread_mutex_unlock(&s->lock);return 1;}
int xSemaphoreTake(SemaphoreHandle_t s,uint32_t timeout) {pthread_mutex_lock(&s->lock);if(!timeout && !s->count){pthread_mutex_unlock(&s->lock);return 0;}while(!s->count)pthread_cond_wait(&s->cv,&s->lock);s->count--;pthread_mutex_unlock(&s->lock);return 1;}
void vSemaphoreDelete(SemaphoreHandle_t s) {pthread_mutex_destroy(&s->lock);pthread_cond_destroy(&s->cv);free(s);}
struct launch {void (*fn)(void *); void *arg;};
static void *launch(void *p) {struct launch x=*(struct launch *)p;free(p);x.fn(x.arg);return NULL;}
int xTaskCreatePinnedToCore(void (*fn)(void *),const char *name,unsigned stack,void *arg,unsigned priority,TaskHandle_t *task,int core) {(void)name;(void)stack;(void)priority;(void)task;(void)core;if(fail_task)return 0;struct launch *p=malloc(sizeof(*p));*p=(struct launch){fn,arg};pthread_t t;assert(!pthread_create(&t,NULL,launch,p));pthread_detach(t);return pdPASS;}
void vTaskDelete(void *p) {(void)p;pthread_exit(NULL);}
void vTaskDelay(unsigned n) {usleep(n*1000);}
esp_err_t i2s_new_channel(const i2s_chan_config_t *cfg,i2s_chan_handle_t *tx,void *rx) {(void)cfg;(void)rx;*tx=(void *)1;channels++;return ESP_OK;}
esp_err_t i2s_channel_init_std_mode(i2s_chan_handle_t h,const i2s_std_config_t *cfg) {(void)h;(void)cfg;return ESP_OK;}
esp_err_t i2s_channel_enable(i2s_chan_handle_t h) {(void)h;return ESP_OK;}
esp_err_t i2s_channel_disable(i2s_chan_handle_t h) {(void)h;return ESP_OK;}
esp_err_t i2s_del_channel(i2s_chan_handle_t h) {(void)h;channels--;return ESP_OK;}
esp_err_t i2s_channel_write(i2s_chan_handle_t h,const void *data,size_t n,size_t *sent,uint32_t timeout) {(void)h;(void)timeout;assert(engine_done);size_t call=++writes;if(fail_write && call>=(size_t)fail_write)return ESP_FAIL;size_t at=captured_count;assert(at+n/2<sizeof(captured)/sizeof(*captured));memcpy(captured+at,data,n);captured_count+=n/2;*sent=n;usleep(1000);return ESP_OK;}
int gpio_reset_pin(gpio_num_t g) {(void)g;return 0;}
int gpio_set_direction(gpio_num_t g,int d) {(void)g;(void)d;return 0;}
int gpio_set_level(gpio_num_t g,int l) {(void)g;(void)l;return 0;}


__asm__(".global _binary_model_s_vtts_start\n_binary_model_s_vtts_start: .byte 0\n.global _binary_model_s_vtts_end\n_binary_model_s_vtts_end:\n"
        ".global _binary_model_m_vtts_start\n_binary_model_m_vtts_start: .byte 0\n.global _binary_model_m_vtts_end\n_binary_model_m_vtts_end:\n"
        ".global _binary_lex_en_bin_start\n_binary_lex_en_bin_start: .byte 0\n.global _binary_lex_en_bin_end\n_binary_lex_en_bin_end:\n");
void *heap_caps_aligned_alloc(size_t align,size_t n,int caps) {(void)align;return heap_caps_malloc(n,caps);}
size_t heap_caps_get_largest_free_block(int caps) {(void)caps;return largest_block;}
size_t heap_caps_get_free_size(int caps) {(void)caps;return largest_block;}
bool jr_brain_enabled(void) {return false;}
bool jr_resource_i2s_acquire(jr_i2s_owner_t owner,uint32_t ms,const char *reason) {(void)owner;(void)ms;(void)reason;return true;}
void jr_resource_i2s_release(jr_i2s_owner_t owner,const char *reason) {(void)owner;(void)reason;}
void vTaskPrioritySet(TaskHandle_t task,unsigned priority) {(void)task;assert(priority==5 && engine_done && writes==0);}
void vtts_set_expressiveness(vtts_model *m,float x) {(void)m;(void)x;}
int vtts_init(vtts_model *m,const void *blob,size_t n) {(void)blob;(void)n;m->sample_rate=22050;return 0;}
size_t vtts_stream_arena_bytes(const vtts_model *m,int ids,int frames) {(void)m;(void)ids;assert(frames==128);return 4096;}
int vtts_lex_init(vtts_lex *lex,const void *blob,size_t n) {(void)lex;(void)blob;(void)n;return 0;}
long vtts_synthesize_stream_ex(const vtts_model *m,const int32_t *ids,int n,float speed,int first,int frames,int (*cb)(const float *,long,void *),void *user,void *arena,size_t bytes) {
    (void)m;(void)ids;(void)n;(void)speed;(void)first;(void)arena;(void)bytes;
    assert(frames==128 && (pipeline_mode || writes==0));float raw[2048];for(size_t i=0;i<2048;i++)raw[i]=pipeline_mode && text_calls==2 ? 0.2f : 0.1f;
    for(int i=0;i<12;i++)if(cb(raw,2048,user))return -1;
    return 12*2048;
}
int vtts_text_to_ids_ex(const char *text,vtts_text_ws *ws,const vtts_text_opts *opts,int (*cb)(const int32_t *,int,void *),void *user) {
    (void)ws;(void)opts;text_calls++;
    if (pipeline_mode) {
        assert(!strcmp(text,text_calls==1 ? JR_TTS_TEST_PREFIX : JR_TTS_TEST_TEXT+strlen(JR_TTS_TEST_PREFIX)+1));
        if(text_calls==2 && late_second){usleep(200000);assert(writes>0);}
        if(text_calls==2 && fail_second)return -1;
    } else {assert(!strcmp(text,expected_text));assert(writes==0);}
    int32_t ids[]={1,2,3};int result=cb(ids,3,user);engine_done=true;return result ? -1 : 1;
}

static void run_buffered(bool overflow,bool allocation_failure,bool io_failure) {
    writes=0;captured_count=0;text_calls=0;engine_done=false;
    fail_alloc=allocation_failure;fail_write=io_failure?3:0;
    largest_block=overflow?JR_TTS_PSRAM_RESERVE+6000:4*1024*1024;
    expected_text="Olá! Vamos ler todas as palavras de uma frase completa, sem cortes artificiais.";
    jr_tts_job_t job={.opts={.volume=100,.model='s',.speed=1.0f},.requested_us=esp_timer_get_time()};
    strcpy(job.text,expected_text);uint32_t audio=0,synth=0;float rtf=0;
    esp_err_t result=synth_and_play(&job,&audio,&synth,&rtf,false);
    if(allocation_failure) {assert(result==ESP_ERR_NO_MEM && writes==0 && text_calls==0);}
    else if(overflow) {assert(result==ESP_ERR_INVALID_SIZE && writes==0 && text_calls==1);}
    else if(io_failure) {assert(result==ESP_FAIL && text_calls==1);}
    else {
        assert(result==ESP_OK && text_calls==1 && audio>0);
        assert(captured_count==12*2048*2+5*512);
        for(size_t i=0;i<12*2048*2;i++)assert(captured[i]==3277);
        for(size_t i=12*2048*2;i<captured_count;i++)assert(captured[i]==0);
    }
    assert(allocations==0 && channels==0);
}
static void run_pipeline(bool late, bool fail_synth, bool fail_io, bool fail_spawn) {
    pipeline_mode=true;late_second=late;fail_second=fail_synth;
    fail_write=fail_io?3:0;fail_task=fail_spawn;fail_alloc=0;
    writes=0;captured_count=0;text_calls=0;engine_done=false;state.last_underruns=0;
    largest_block=4*1024*1024;
    jr_tts_job_t job={.opts={.volume=100,.model='s',.speed=1.0f},.requested_us=esp_timer_get_time()};
    strcpy(job.text,JR_TTS_TEST_TEXT);uint32_t audio=0,synth=0;float rtf=0;
    esp_err_t result=synth_and_play(&job,&audio,&synth,&rtf,false);
    assert(allocations==0 && channels==0);
    if(fail_spawn){assert(result==ESP_ERR_NO_MEM && text_calls==1 && writes==0);}
    else if(fail_synth || fail_io){assert(result==ESP_FAIL && text_calls==2);}
    else {
        assert(result==ESP_OK && text_calls==2 && audio>0);
        const size_t clip=12*2048*2, gap=2205*2;
        assert(captured_count==2*clip+gap+5*512);
        for(size_t i=0;i<clip;i++)assert(captured[i]==3277);
        for(size_t i=clip;i<clip+gap;i++)assert(captured[i]==0);
        for(size_t i=clip+gap;i<2*clip+gap;i++)assert(captured[i]==6553);
        for(size_t i=2*clip+gap;i<captured_count;i++)assert(captured[i]==0);
        assert(state.last_underruns==(late?1U:0U));
    }
    pipeline_mode=false;fail_write=0;fail_task=0;
}
int main(void) {
    synth_mutex=xSemaphoreCreateBinary();xSemaphoreGive(synth_mutex);
    run_buffered(false,false,false);
    run_buffered(true,false,false);
    run_buffered(false,true,false);
    run_buffered(false,false,true);
    run_pipeline(false,false,false,false);
    run_pipeline(true,false,false,false);
    run_pipeline(false,true,false,false);
    run_pipeline(false,false,true,false);
    run_pipeline(false,false,false,true);
    vSemaphoreDelete(synth_mutex);
    float raw[]={-2,-0.5f,0,0.5f,2};int16_t clip[5]={0};
    jr_tts_synth_t c={.pcm=clip,.cap=5};assert(on_pcm(raw,5,&c)==0);assert(clip[0]==-32767 && clip[4]==32767 && c.count==5);assert(on_pcm(raw,1,&c)==1 && c.error==ESP_ERR_INVALID_SIZE);
    puts("PASS: complete text passed once, playback after generation, exact samples/final drain, overflow plays nothing, allocation/I2S failures, cached PCM bounds; two-block overlap, exact boundary samples, late producer, synthesis/I2S/task failures");
}
