#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_SIZE 2
#define ESP_ERR_INVALID_STATE 3
#define ESP_ERR_INVALID_ARG 4
#define ESP_ERR_NO_MEM 5
#define ESP_ERR_TIMEOUT 6
#define ESP_ERR_NOT_SUPPORTED 7
const char *esp_err_to_name(esp_err_t e);
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_INTERNAL 4
void *heap_caps_malloc(size_t, int);
void *heap_caps_calloc(size_t, size_t, int);
void *heap_caps_aligned_alloc(size_t, size_t, int);
void heap_caps_free(void *);
size_t heap_caps_get_free_size(int);
size_t heap_caps_get_largest_free_block(int);
int64_t esp_timer_get_time(void);
static inline void host_log(const char *tag, const char *format, ...) {(void)tag; (void)format;}
#define ESP_LOGI host_log
#define ESP_LOGW host_log
typedef pthread_mutex_t portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define portENTER_CRITICAL(p) pthread_mutex_lock(p)
#define portEXIT_CRITICAL(p) pthread_mutex_unlock(p)
#define pdMS_TO_TICKS(ms) (ms)
#define portMAX_DELAY UINT32_MAX
#define pdTRUE 1
#define pdPASS 1
#define tskIDLE_PRIORITY 0
typedef int BaseType_t;
typedef void *TaskHandle_t;
typedef struct {pthread_mutex_t lock; pthread_cond_t cv; unsigned count;} *SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateMutex(void);
SemaphoreHandle_t xSemaphoreCreateBinary(void);
int xSemaphoreTake(SemaphoreHandle_t, uint32_t);
int xSemaphoreGive(SemaphoreHandle_t);
void vSemaphoreDelete(SemaphoreHandle_t);
int xTaskCreatePinnedToCore(void (*)(void *), const char *, unsigned, void *, unsigned, TaskHandle_t *, int);
void vTaskDelete(void *);
void vTaskDelay(unsigned);
void vTaskPrioritySet(TaskHandle_t, unsigned);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
void xTaskNotifyGive(TaskHandle_t);
unsigned ulTaskNotifyTake(int, uint32_t);
typedef int nvs_handle_t;
#define NVS_READONLY 0
#define NVS_READWRITE 1
esp_err_t nvs_open(const char *, int, nvs_handle_t *);
esp_err_t nvs_get_u8(nvs_handle_t, const char *, uint8_t *);
esp_err_t nvs_get_i32(nvs_handle_t, const char *, int32_t *);
esp_err_t nvs_set_u8(nvs_handle_t, const char *, uint8_t);
esp_err_t nvs_set_i32(nvs_handle_t, const char *, int32_t);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
typedef int gpio_num_t;
#define GPIO_MODE_OUTPUT 1
int gpio_reset_pin(gpio_num_t);
int gpio_set_direction(gpio_num_t, int);
int gpio_set_level(gpio_num_t, int);
typedef void *i2s_chan_handle_t;
typedef struct {bool auto_clear; int dma_desc_num, dma_frame_num;} i2s_chan_config_t;
#define I2S_CHANNEL_DEFAULT_CONFIG(a,b) ((i2s_chan_config_t){0})
#define I2S_NUM_AUTO 0
#define I2S_ROLE_MASTER 0
#define I2S_STD_CLK_DEFAULT_CONFIG(a) (a)
#define I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(a,b) 0
#define I2S_DATA_BIT_WIDTH_16BIT 16
#define I2S_SLOT_MODE_STEREO 2
#define I2S_GPIO_UNUSED -1
typedef struct {int clk_cfg,slot_cfg; struct {int mclk,bclk,ws,dout,din;} gpio_cfg;} i2s_std_config_t;
esp_err_t i2s_new_channel(const i2s_chan_config_t *, i2s_chan_handle_t *, void *);
esp_err_t i2s_channel_init_std_mode(i2s_chan_handle_t, const i2s_std_config_t *);
esp_err_t i2s_channel_enable(i2s_chan_handle_t);
esp_err_t i2s_channel_write(i2s_chan_handle_t, const void *, size_t, size_t *, uint32_t);
esp_err_t i2s_channel_disable(i2s_chan_handle_t);
esp_err_t i2s_del_channel(i2s_chan_handle_t);
