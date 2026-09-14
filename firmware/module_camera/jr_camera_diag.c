#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jr_board.h"
#include "jr_camera_diag.h"
#if JR_CAMERA_ENABLED
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG="jrbot_camera";
static bool cleanup_failed;
static bool camera_busy;
static bool runtime_initialized;
static jr_camera_size_t runtime_size=JR_CAMERA_SIZE_VGA;
static int runtime_quality=12;
static uint32_t capture_count;
static portMUX_TYPE camera_state_lock=portMUX_INITIALIZER_UNLOCKED;

static bool camera_acquire(void) {
    bool acquired=false;
    portENTER_CRITICAL(&camera_state_lock);
    if (!camera_busy) {
        camera_busy=true;
        acquired=true;
    }
    portEXIT_CRITICAL(&camera_state_lock);
    return acquired;
}

static void camera_release(void) {
    portENTER_CRITICAL(&camera_state_lock);
    camera_busy=false;
    portEXIT_CRITICAL(&camera_state_lock);
}

static const camera_config_t probe_config={
    .pin_pwdn=JR_CAM_PWDN,.pin_reset=JR_CAM_RESET,
    .pin_xclk=JR_CAM_XCLK,.pin_sccb_sda=JR_CAM_SDA,.pin_sccb_scl=JR_CAM_SCL,
    .pin_d0=JR_CAM_D0,.pin_d1=JR_CAM_D1,.pin_d2=JR_CAM_D2,.pin_d3=JR_CAM_D3,
    .pin_d4=JR_CAM_D4,.pin_d5=JR_CAM_D5,.pin_d6=JR_CAM_D6,.pin_d7=JR_CAM_D7,
    .pin_vsync=JR_CAM_VSYNC,.pin_href=JR_CAM_HREF,.pin_pclk=JR_CAM_PCLK,
    .xclk_freq_hz=20000000,
    .ledc_timer=LEDC_TIMER_0,.ledc_channel=LEDC_CHANNEL_0,
    .pixel_format=PIXFORMAT_JPEG,.frame_size=FRAMESIZE_QVGA,
    .jpeg_quality=12,.fb_count=1,
    .fb_location=CAMERA_FB_IN_PSRAM,.grab_mode=CAMERA_GRAB_WHEN_EMPTY,
};

static camera_config_t runtime_config(void) {
    camera_config_t c=probe_config;
    c.frame_size=FRAMESIZE_UXGA;
    c.jpeg_quality=12;
    c.fb_count=2;
    c.grab_mode=CAMERA_GRAB_LATEST;
    return c;
}

static framesize_t frame_size_for(jr_camera_size_t size) {
    switch (size) {
        case JR_CAMERA_SIZE_QVGA: return FRAMESIZE_QVGA;
        case JR_CAMERA_SIZE_VGA: return FRAMESIZE_VGA;
        case JR_CAMERA_SIZE_SVGA: return FRAMESIZE_SVGA;
        case JR_CAMERA_SIZE_XGA: return FRAMESIZE_XGA;
        case JR_CAMERA_SIZE_HD: return FRAMESIZE_HD;
        case JR_CAMERA_SIZE_UXGA: return FRAMESIZE_UXGA;
        default: return FRAMESIZE_VGA;
    }
}

static int clamp_quality(int q) {
    if (q<4) q=4;
    if (q>30) q=30;
    return q;
}

static esp_err_t ensure_runtime_camera(void) {
    if (runtime_initialized) return ESP_OK;
    if (!esp_psram_is_initialized()) return ESP_ERR_NOT_SUPPORTED;
    camera_config_t cfg=runtime_config();
    esp_err_t err=esp_camera_init(&cfg);
    if (err!=ESP_OK) return err;
    runtime_initialized=true;
    sensor_t *sensor=esp_camera_sensor_get();
    if (!sensor) return ESP_ERR_NOT_FOUND;
    if (sensor->set_framesize(sensor,frame_size_for(runtime_size))!=0) return ESP_FAIL;
    if (sensor->set_quality(sensor,runtime_quality)!=0) return ESP_FAIL;
    ESP_LOGI(TAG,"CAMERA_RUNTIME_START max=UXGA fb_count=%d grab=LATEST psram=1 pid=0x%04X",
             cfg.fb_count,sensor->id.PID);
    return ESP_OK;
}

static esp_err_t apply_profile(sensor_t *sensor,jr_camera_size_t size,int quality,bool *changed) {
    if (!sensor) return ESP_ERR_NOT_FOUND;
    quality=clamp_quality(quality);
    bool profile_changed=(runtime_size!=size)||(runtime_quality!=quality);
    if (runtime_size!=size) {
        if (sensor->set_framesize(sensor,frame_size_for(size))!=0) return ESP_FAIL;
        runtime_size=size;
    }
    if (runtime_quality!=quality) {
        if (sensor->set_quality(sensor,quality)!=0) return ESP_FAIL;
        runtime_quality=quality;
    }
    if (changed) *changed=profile_changed;
    if (profile_changed) {
        ESP_LOGI(TAG,"CAMERA_CONFIG size=%s quality=%d",jr_camera_size_name(size),quality);
    }
    return ESP_OK;
}

static void discard_one_frame(void) {
    camera_fb_t *warm=esp_camera_fb_get();
    if (warm) esp_camera_fb_return(warm);
}
#endif

const char *jr_camera_size_name(jr_camera_size_t size) {
    switch (size) {
        case JR_CAMERA_SIZE_QVGA: return "qvga";
        case JR_CAMERA_SIZE_VGA: return "vga";
        case JR_CAMERA_SIZE_SVGA: return "svga";
        case JR_CAMERA_SIZE_XGA: return "xga";
        case JR_CAMERA_SIZE_HD: return "hd";
        case JR_CAMERA_SIZE_UXGA: return "uxga";
        default: return "vga";
    }
}

bool jr_camera_parse_size(const char *name,jr_camera_size_t *out) {
    if (!name || !out) return false;
    if (!strcmp(name,"qvga") || !strcmp(name,"320x240")) *out=JR_CAMERA_SIZE_QVGA;
    else if (!strcmp(name,"vga") || !strcmp(name,"640x480")) *out=JR_CAMERA_SIZE_VGA;
    else if (!strcmp(name,"svga") || !strcmp(name,"800x600")) *out=JR_CAMERA_SIZE_SVGA;
    else if (!strcmp(name,"xga") || !strcmp(name,"1024x768")) *out=JR_CAMERA_SIZE_XGA;
    else if (!strcmp(name,"hd") || !strcmp(name,"1280x720")) *out=JR_CAMERA_SIZE_HD;
    else if (!strcmp(name,"uxga") || !strcmp(name,"1600x1200")) *out=JR_CAMERA_SIZE_UXGA;
    else return false;
    return true;
}

bool jr_camera_probe_once(unsigned *pid_out) {
    if (pid_out) *pid_out=0;
#if !JR_CAMERA_ENABLED
    return false;
#else
    if (cleanup_failed || !esp_psram_is_initialized() || !camera_acquire()) return false;
    if (runtime_initialized) {
        sensor_t *sensor=esp_camera_sensor_get();
        bool present=sensor!=NULL;
        if (present && pid_out) *pid_out=sensor->id.PID;
        camera_release();
        return present;
    }
    esp_err_t err=esp_camera_init(&probe_config);
    if (err!=ESP_OK) {
        camera_release();
        return false;
    }
    sensor_t *sensor=esp_camera_sensor_get();
    bool present=sensor!=NULL;
    if (present && pid_out) *pid_out=sensor->id.PID;
    err=esp_camera_deinit();
    if (err!=ESP_OK) {
        cleanup_failed=true;
        present=false;
    }
    camera_release();
    return present;
#endif
}

void jr_camera_jpeg_release(jr_camera_jpeg_t *frame) {
    if (!frame) return;
    if (frame->data) heap_caps_free(frame->data);
    memset(frame,0,sizeof(*frame));
}

esp_err_t jr_camera_capture_jpeg(jr_camera_jpeg_t *out) {
    return jr_camera_capture_jpeg_ex(out,JR_CAMERA_SIZE_VGA,12);
}

esp_err_t jr_camera_capture_jpeg_ex(jr_camera_jpeg_t *out,jr_camera_size_t size,int jpeg_quality) {
    if (!out) return ESP_ERR_INVALID_ARG;
    memset(out,0,sizeof(*out));
#if !JR_CAMERA_ENABLED
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (cleanup_failed) return ESP_ERR_INVALID_STATE;
    if (!esp_psram_is_initialized()) return ESP_ERR_NOT_SUPPORTED;
    if (!camera_acquire()) return ESP_ERR_INVALID_STATE;

    esp_err_t result=ensure_runtime_camera();
    camera_fb_t *fb=NULL;
    if (result!=ESP_OK) goto done;

    sensor_t *sensor=esp_camera_sensor_get();
    if (!sensor) {
        result=ESP_ERR_NOT_FOUND;
        goto done;
    }
    bool changed=false;
    result=apply_profile(sensor,size,jpeg_quality,&changed);
    if (result!=ESP_OK) goto done;
    if (changed) discard_one_frame();

    fb=esp_camera_fb_get();
    if (!fb) {
        result=ESP_ERR_TIMEOUT;
        goto done;
    }
    if (!fb->buf || fb->len<4 || !fb->width || !fb->height ||
        fb->format!=PIXFORMAT_JPEG || fb->buf[0]!=0xff || fb->buf[1]!=0xd8) {
        result=ESP_FAIL;
        goto done;
    }

    out->data=heap_caps_malloc(fb->len,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if (!out->data) out->data=heap_caps_malloc(fb->len,MALLOC_CAP_8BIT);
    if (!out->data) {
        result=ESP_ERR_NO_MEM;
        goto done;
    }
    memcpy(out->data,fb->buf,fb->len);
    out->len=fb->len;
    out->pid=sensor->id.PID;
    out->width=(unsigned)fb->width;
    out->height=(unsigned)fb->height;
    out->jpeg_quality=runtime_quality;
    out->size=runtime_size;
    capture_count++;
    result=ESP_OK;

done:
    if (fb) esp_camera_fb_return(fb);
    if (result!=ESP_OK) jr_camera_jpeg_release(out);
    camera_release();
    return result;
#endif
}

bool jr_camera_test_once(char *response,size_t capacity) {
    if (!response || capacity<256) return false;
#if !JR_CAMERA_ENABLED
    snprintf(response,capacity,"JR_ERROR camera_test=disabled");
    return false;
#else
    if (cleanup_failed) {
        snprintf(response,capacity,"JR_ERROR camera_test=cleanup_failed restart_required=1");
        return false;
    }
    jr_camera_jpeg_t frame={0};
    esp_err_t err=jr_camera_capture_jpeg_ex(&frame,JR_CAMERA_SIZE_QVGA,12);
    if (err!=ESP_OK) {
        snprintf(response,capacity,"JR_ERROR camera_test=capture error=%s",esp_err_to_name(err));
        return false;
    }
    unsigned pid=frame.pid;
    unsigned width=frame.width;
    unsigned height=frame.height;
    unsigned bytes=(unsigned)frame.len;
    int quality=frame.jpeg_quality;
    jr_camera_jpeg_release(&frame);
    snprintf(response,capacity,
             "JR_OK camera_test=frame_received pid=0x%04X width=%u height=%u bytes=%u quality=%d released=1 optical_check=pending",
             pid,width,height,bytes,quality);
    return true;
#endif
}
