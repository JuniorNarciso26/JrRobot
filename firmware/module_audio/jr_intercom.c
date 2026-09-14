#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "jr_audio.h"
#include "jr_intercom.h"
#include "jr_mic.h"

#define JR_INTERCOM_RATE 16000
#define JR_INTERCOM_DEFAULT_SAMPLES 2048
#define JR_INTERCOM_MIN_SAMPLES 512
#define JR_INTERCOM_MAX_SAMPLES 4096
#define JR_INTERCOM_MAX_TALK_BYTES (JR_INTERCOM_MAX_SAMPLES * 2)

static const char *TAG = "jrbot_intercom";
static uint32_t listen_chunks;
static uint32_t talk_chunks;
static uint32_t talk_samples;

static esp_err_t text_reply(httpd_req_t *req,const char *status,const char *text) {
    httpd_resp_set_status(req,status);
    httpd_resp_set_type(req,"text/plain; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    return httpd_resp_send(req,text,HTTPD_RESP_USE_STRLEN);
}

static bool authorized(httpd_req_t *req) {
    char flag[4];
    return httpd_req_get_hdr_value_str(req,"X-JrBot-Intercom",flag,sizeof(flag))==ESP_OK && !strcmp(flag,"1");
}

static int query_samples(httpd_req_t *req) {
    int samples=JR_INTERCOM_DEFAULT_SAMPLES;
    char query[64]={0};
    char value[16]={0};
    if (httpd_req_get_url_query_str(req,query,sizeof(query))==ESP_OK &&
        httpd_query_key_value(query,"samples",value,sizeof(value))==ESP_OK) {
        samples=atoi(value);
    }
    if (samples<JR_INTERCOM_MIN_SAMPLES) samples=JR_INTERCOM_MIN_SAMPLES;
    if (samples>JR_INTERCOM_MAX_SAMPLES) samples=JR_INTERCOM_MAX_SAMPLES;
    return samples;
}

esp_err_t jr_intercom_listen_start_handler(httpd_req_t *req) {
    if (!authorized(req)) return text_reply(req,"403 Forbidden","JR_INTERCOM_ERROR cabecalho_obrigatorio");
    if (jr_audio_talk_active()) jr_audio_talk_stop();
    esp_err_t err=jr_mic_live_start();
    if (err!=ESP_OK) {
        char message[96];
        snprintf(message,sizeof(message),"JR_INTERCOM_ERROR listen_start=%s",esp_err_to_name(err));
        return text_reply(req,"409 Conflict",message);
    }
    listen_chunks=0;
    ESP_LOGI(TAG,"LISTEN_START rate=%d",JR_INTERCOM_RATE);
    return text_reply(req,"200 OK","JR_OK intercom_listen=started rate=16000 format=pcm16_mono");
}

esp_err_t jr_intercom_listen_chunk_handler(httpd_req_t *req) {
    if (!authorized(req)) return text_reply(req,"403 Forbidden","JR_INTERCOM_ERROR cabecalho_obrigatorio");
    if (!jr_mic_live_active()) return text_reply(req,"409 Conflict","JR_INTERCOM_ERROR listen_not_active");

    int requested=query_samples(req);
    int16_t *pcm=(int16_t *)heap_caps_malloc((size_t)requested*sizeof(int16_t),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if (!pcm) pcm=(int16_t *)heap_caps_malloc((size_t)requested*sizeof(int16_t),MALLOC_CAP_8BIT);
    if (!pcm) return text_reply(req,"500 Internal Server Error","JR_INTERCOM_ERROR sem_memoria");

    size_t got=0;
    esp_err_t err=jr_mic_live_read_pcm16(pcm,(size_t)requested,&got,700);
    if (err!=ESP_OK || !got) {
        heap_caps_free(pcm);
        char message[96];
        snprintf(message,sizeof(message),"JR_INTERCOM_ERROR listen_read=%s samples=%u",esp_err_to_name(err),(unsigned)got);
        return text_reply(req,"409 Conflict",message);
    }

    char samples_text[16];
    snprintf(samples_text,sizeof(samples_text),"%u",(unsigned)got);
    httpd_resp_set_type(req,"application/octet-stream");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    httpd_resp_set_hdr(req,"X-JrBot-Audio-Format","pcm16_mono_le");
    httpd_resp_set_hdr(req,"X-JrBot-Sample-Rate","16000");
    httpd_resp_set_hdr(req,"X-JrBot-Samples",samples_text);
    err=httpd_resp_send(req,(const char *)pcm,(ssize_t)(got*sizeof(int16_t)));
    heap_caps_free(pcm);
    if (err==ESP_OK) {
        listen_chunks++;
        if ((listen_chunks%50u)==0u) ESP_LOGI(TAG,"LISTEN_PROGRESS chunks=%lu",(unsigned long)listen_chunks);
    }
    return err;
}

esp_err_t jr_intercom_listen_stop_handler(httpd_req_t *req) {
    if (!authorized(req)) return text_reply(req,"403 Forbidden","JR_INTERCOM_ERROR cabecalho_obrigatorio");
    jr_mic_live_stop();
    ESP_LOGI(TAG,"LISTEN_STOP chunks=%lu",(unsigned long)listen_chunks);
    char message[96];
    snprintf(message,sizeof(message),"JR_OK intercom_listen=stopped chunks=%lu",(unsigned long)listen_chunks);
    return text_reply(req,"200 OK",message);
}

esp_err_t jr_intercom_talk_start_handler(httpd_req_t *req) {
    if (!authorized(req)) return text_reply(req,"403 Forbidden","JR_INTERCOM_ERROR cabecalho_obrigatorio");
    if (jr_mic_live_active()) jr_mic_live_stop();
    esp_err_t err=jr_audio_talk_start();
    if (err!=ESP_OK) {
        char message[96];
        snprintf(message,sizeof(message),"JR_INTERCOM_ERROR talk_start=%s",esp_err_to_name(err));
        return text_reply(req,"409 Conflict",message);
    }
    talk_chunks=0;
    talk_samples=0;
    ESP_LOGI(TAG,"TALK_START rate=%d",JR_INTERCOM_RATE);
    return text_reply(req,"200 OK","JR_OK intercom_talk=started rate=16000 format=pcm16_mono");
}

esp_err_t jr_intercom_talk_chunk_handler(httpd_req_t *req) {
    if (!authorized(req)) return text_reply(req,"403 Forbidden","JR_INTERCOM_ERROR cabecalho_obrigatorio");
    if (!jr_audio_talk_active()) return text_reply(req,"409 Conflict","JR_INTERCOM_ERROR talk_not_active");
    if (req->content_len==0 || req->content_len>JR_INTERCOM_MAX_TALK_BYTES || (req->content_len&1u))
        return text_reply(req,"413 Payload Too Large","JR_INTERCOM_ERROR pcm_chunk_invalido");

    uint8_t *body=(uint8_t *)heap_caps_malloc(req->content_len,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if (!body) body=(uint8_t *)heap_caps_malloc(req->content_len,MALLOC_CAP_8BIT);
    if (!body) return text_reply(req,"500 Internal Server Error","JR_INTERCOM_ERROR sem_memoria");

    size_t read=0;
    while (read<req->content_len) {
        int n=httpd_req_recv(req,(char *)body+read,req->content_len-read);
        if (n<=0) {
            heap_caps_free(body);
            return text_reply(req,"408 Request Timeout","JR_INTERCOM_ERROR corpo_incompleto");
        }
        read+=(size_t)n;
    }

    size_t samples=read/2u;
    esp_err_t err=jr_audio_talk_write_pcm16_mono((const int16_t *)body,samples,JR_INTERCOM_RATE);
    heap_caps_free(body);
    if (err!=ESP_OK) {
        char message[96];
        snprintf(message,sizeof(message),"JR_INTERCOM_ERROR talk_write=%s",esp_err_to_name(err));
        return text_reply(req,"409 Conflict",message);
    }
    talk_chunks++;
    talk_samples+=(uint32_t)samples;
    return text_reply(req,"200 OK","JR_OK intercom_talk=chunk");
}

esp_err_t jr_intercom_talk_stop_handler(httpd_req_t *req) {
    if (!authorized(req)) return text_reply(req,"403 Forbidden","JR_INTERCOM_ERROR cabecalho_obrigatorio");
    jr_audio_talk_stop();
    ESP_LOGI(TAG,"TALK_STOP chunks=%lu samples=%lu",(unsigned long)talk_chunks,(unsigned long)talk_samples);
    char message[112];
    snprintf(message,sizeof(message),"JR_OK intercom_talk=stopped chunks=%lu samples=%lu",
             (unsigned long)talk_chunks,(unsigned long)talk_samples);
    return text_reply(req,"200 OK",message);
}

esp_err_t jr_intercom_status_handler(httpd_req_t *req) {
    char message[192];
    snprintf(message,sizeof(message),
             "JR_OK intercom listen=%d talk=%d listen_chunks=%lu talk_chunks=%lu talk_samples=%lu rate=%d",
             jr_mic_live_active()?1:0,jr_audio_talk_active()?1:0,
             (unsigned long)listen_chunks,(unsigned long)talk_chunks,(unsigned long)talk_samples,JR_INTERCOM_RATE);
    return text_reply(req,"200 OK",message);
}
