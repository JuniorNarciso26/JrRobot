/* Exercise the actual firmware callbacks/player with pthread-backed FreeRTOS
 * and a recording I2S sink. Does not simulate neural inference or ESP32 timing. */
#define JRBOT_WITH_VTTS 1
#include "../../firmware/module_tts/jr_tts.c"
#include <assert.h>
#include <stdatomic.h>

static _Atomic size_t writes, captured_count;
static int16_t captured[400000];
static int fail_write, fail_task, fail_alloc;
static _Atomic int allocations, channels;
const char *esp_err_to_name(esp_err_t e) {(void)e; return "host";}
int64_t esp_timer_get_time(void) {struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1000000LL+t.tv_nsec/1000;}
void *heap_caps_malloc(size_t n, int caps) {(void)caps; if(fail_alloc)return NULL; void *p=malloc(n); if(p)allocations++; return p;}
void *heap_caps_calloc(size_t n,size_t bytes,int caps) {void *p=heap_caps_malloc(n*bytes,caps);if(p)memset(p,0,n*bytes);return p;}
void heap_caps_free(void *p) {if(p)allocations--; free(p);}
SemaphoreHandle_t xSemaphoreCreateBinary(void) {SemaphoreHandle_t s=calloc(1,sizeof(*s)); pthread_mutex_init(&s->lock,0);pthread_cond_init(&s->cv,0);return s;}
int xSemaphoreGive(SemaphoreHandle_t s) {pthread_mutex_lock(&s->lock);s->count++;pthread_cond_signal(&s->cv);pthread_mutex_unlock(&s->lock);return 1;}
int xSemaphoreTake(SemaphoreHandle_t s,uint32_t timeout) {(void)timeout;pthread_mutex_lock(&s->lock);while(!s->count)pthread_cond_wait(&s->cv,&s->lock);s->count--;pthread_mutex_unlock(&s->lock);return 1;}
void vSemaphoreDelete(SemaphoreHandle_t s) {pthread_mutex_destroy(&s->lock);pthread_cond_destroy(&s->cv);free(s);}
struct launch {void (*fn)(void *); void *arg;};
static void *launch(void *p) {struct launch x=*(struct launch *)p;free(p);x.fn(x.arg);return NULL;}
int xTaskCreatePinnedToCore(void (*fn)(void *),const char *name,unsigned stack,void *arg,unsigned priority,TaskHandle_t *task,int core) {(void)name;(void)stack;(void)priority;(void)task;(void)core;if(fail_task)return 0;struct launch *p=malloc(sizeof(*p));*p=(struct launch){fn,arg};pthread_t t;assert(!pthread_create(&t,NULL,launch,p));pthread_detach(t);return pdPASS;}
void vTaskDelete(void *p) {(void)p;pthread_exit(NULL);}
void vTaskDelay(unsigned n) {usleep(n*1000);}
StreamBufferHandle_t xStreamBufferCreateStatic(size_t n,size_t trigger,uint8_t *data,StaticStreamBuffer_t *s) {(void)trigger;pthread_mutex_init(&s->lock,NULL);pthread_cond_init(&s->cv,NULL);s->data=data;s->cap=n-1;return s;}
static void timed_wait(StreamBufferHandle_t s,unsigned ms) {struct timespec t;clock_gettime(CLOCK_REALTIME,&t);t.tv_nsec+=ms*1000000L;t.tv_sec+=t.tv_nsec/1000000000L;t.tv_nsec%=1000000000L;pthread_cond_timedwait(&s->cv,&s->lock,&t);}
size_t xStreamBufferSend(StreamBufferHandle_t s,const void *data,size_t n,uint32_t ms) {pthread_mutex_lock(&s->lock);if(s->used==s->cap)timed_wait(s,ms);size_t take=s->cap-s->used;if(take>n)take=n;for(size_t i=0;i<take;i++){s->data[s->tail]=((uint8_t *)data)[i];s->tail=(s->tail+1)%s->cap;}s->used+=take;pthread_cond_signal(&s->cv);pthread_mutex_unlock(&s->lock);return take;}
size_t xStreamBufferReceive(StreamBufferHandle_t s,void *data,size_t n,uint32_t ms) {pthread_mutex_lock(&s->lock);if(!s->used)timed_wait(s,ms);size_t take=s->used;if(take>n)take=n;for(size_t i=0;i<take;i++){((uint8_t *)data)[i]=s->data[s->head];s->head=(s->head+1)%s->cap;}s->used-=take;pthread_cond_signal(&s->cv);pthread_mutex_unlock(&s->lock);return take;}
void vStreamBufferDelete(StreamBufferHandle_t s) {pthread_mutex_destroy(&s->lock);pthread_cond_destroy(&s->cv);}
esp_err_t i2s_new_channel(const i2s_chan_config_t *cfg,i2s_chan_handle_t *tx,void *rx) {(void)cfg;(void)rx;*tx=(void *)1;channels++;return ESP_OK;}
esp_err_t i2s_channel_init_std_mode(i2s_chan_handle_t h,const i2s_std_config_t *cfg) {(void)h;(void)cfg;return ESP_OK;}
esp_err_t i2s_channel_enable(i2s_chan_handle_t h) {(void)h;return ESP_OK;}
esp_err_t i2s_channel_disable(i2s_chan_handle_t h) {(void)h;return ESP_OK;}
esp_err_t i2s_del_channel(i2s_chan_handle_t h) {(void)h;channels--;return ESP_OK;}
esp_err_t i2s_channel_write(i2s_chan_handle_t h,const void *data,size_t n,size_t *sent,uint32_t timeout) {(void)h;(void)timeout;size_t call=++writes;if(fail_write && call>=(size_t)fail_write)return ESP_FAIL;size_t at=captured_count;assert(at+n/2<sizeof(captured)/sizeof(*captured));memcpy(captured+at,data,n);captured_count+=n/2;*sent=n;usleep(1000);return ESP_OK;}
int gpio_reset_pin(gpio_num_t g) {(void)g;return 0;}
int gpio_set_direction(gpio_num_t g,int d) {(void)g;(void)d;return 0;}
int gpio_set_level(gpio_num_t g,int l) {(void)g;(void)l;return 0;}

static void finish(jr_tts_stream_t *s) {portENTER_CRITICAL(&state_lock);s->done=true;portEXIT_CRITICAL(&state_lock);assert(xSemaphoreTake(s->finished,portMAX_DELAY));}
static void test_boundaries(void) {
    const char *samples[]={"Olá! Eu sou o JrBot. Agora vamos ler um texto longo, com acentuação e emoção.","palavras\ncom\ttabulacao","açãoaçãoaçãoaçãoaçãoaçãoaçãoação", "  ", ""};
    for(size_t k=0;k<sizeof(samples)/sizeof(*samples);k++)for(size_t target=1;target<80;target++) {
        const char *t=samples[k];size_t pos=0;
        while(t[pos]) {size_t n=jr_tts_chunk_bytes(t+pos,target);assert(n>0 && n<=strlen(t+pos));pos+=n;assert(!t[pos] || ((unsigned char)t[pos]&0xc0)!=0x80);if(t[pos])assert(t[pos]==' ' || t[pos]=='\n' || t[pos]=='\t' || t[pos-1]==' ' || t[pos-1]=='\n' || t[pos-1]=='\t');}
        assert(pos==strlen(t));
    }
}
int main(void) {
    test_boundaries();
    jr_tts_job_t job={.opts={.volume=100},.requested_us=esp_timer_get_time()};
    int16_t input[60000];for(size_t i=0;i<60000;i++)input[i]=(int16_t)(i%10000);
    jr_tts_stream_t *s=stream_start(&job,22050);assert(s);
    assert(stream_send(s,input,256)==ESP_OK);
    usleep(150000);assert(writes>0); /* First playback precedes producer completion. */
    assert(stream_send(s,input+256,60000-256)==ESP_OK); /* Wraps a full FIFO repeatedly. */
    finish(s);assert(s->error==ESP_OK);assert(s->underruns>=1);
    for(size_t i=0;i<60000;i++)assert(captured[2*i]==input[i] && captured[2*i+1]==input[i]);
    assert(captured_count==120000+5*512); /* All final samples + DMA drain, exactly once. */
    stream_destroy(s);assert(allocations==0 && channels==0);
    writes=0;captured_count=0;fail_write=3;
    s=stream_start(&job,22050);assert(s);assert(stream_send(s,input,60000)==ESP_FAIL);
    finish(s);assert(s->error==ESP_FAIL);stream_destroy(s);assert(allocations==0 && channels==0);
    fail_write=0;fail_task=1;assert(!stream_start(&job,22050));assert(allocations==0);
    fail_task=0;fail_alloc=1;assert(!stream_start(&job,22050));assert(allocations==0);
    fail_alloc=0;
    float raw[]={-2,-0.5f,0,0.5f,2};int16_t clip[5]={0};
    jr_tts_synth_t c={.pcm=clip,.cap=5};assert(on_pcm(raw,5,&c)==0);assert(clip[0]==-32767 && clip[4]==32767 && c.count==5);assert(on_pcm(raw,1,&c)==1 && c.error==ESP_ERR_INVALID_SIZE);
    puts("PASS: boundaries, early playback, FIFO order/wrap, final drain, I2S failure, allocation/task failure, cached PCM cap");
}
