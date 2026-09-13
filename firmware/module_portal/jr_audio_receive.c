#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "jr_audio.h"
#include "jr_audio_receive.h"

#define JR_AUDIO_RX_RATE 16000
#define JR_AUDIO_RX_MAX_SECONDS 10
#define JR_AUDIO_RX_MAX_DATA (JR_AUDIO_RX_RATE * 2 * JR_AUDIO_RX_MAX_SECONDS)
#define JR_AUDIO_RX_MAX_BODY (44 + JR_AUDIO_RX_MAX_DATA)

static uint16_t le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static esp_err_t text_reply(httpd_req_t *req,const char *status,const char *text) {
    httpd_resp_set_status(req,status);
    httpd_resp_set_type(req,"text/plain; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    return httpd_resp_send(req,text,HTTPD_RESP_USE_STRLEN);
}

static bool valid_wav(const uint8_t *data,size_t len,size_t *samples) {
    if (!data || len < 44) return false;
    if (memcmp(data,"RIFF",4) || memcmp(data+8,"WAVE",4)) return false;
    if (memcmp(data+12,"fmt ",4) || le32(data+16) != 16) return false;
    if (le16(data+20) != 1 || le16(data+22) != 1) return false;
    if (le32(data+24) != JR_AUDIO_RX_RATE) return false;
    if (le16(data+32) != 2 || le16(data+34) != 16) return false;
    if (memcmp(data+36,"data",4)) return false;
    uint32_t bytes=le32(data+40);
    if (!bytes || bytes > JR_AUDIO_RX_MAX_DATA || (bytes & 1u)) return false;
    if ((size_t)bytes + 44u != len) return false;
    if (samples) *samples=bytes/2u;
    return true;
}

esp_err_t jr_audio_receive_wav_handler(httpd_req_t *req) {
    char flag[4];
    if (httpd_req_get_hdr_value_str(req,"X-JrBot-Audio",flag,sizeof(flag))!=ESP_OK || strcmp(flag,"1"))
        return text_reply(req,"403 Forbidden","JR_AUDIO_ERROR cabecalho_obrigatorio");
    if (req->content_len < 44 || req->content_len > JR_AUDIO_RX_MAX_BODY)
        return text_reply(req,"413 Payload Too Large","JR_AUDIO_ERROR limite_10s");

    uint8_t *body=(uint8_t *)heap_caps_malloc(req->content_len,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if (!body) body=(uint8_t *)heap_caps_malloc(req->content_len,MALLOC_CAP_8BIT);
    if (!body) return text_reply(req,"500 Internal Server Error","JR_AUDIO_ERROR sem_memoria");

    size_t read=0;
    while (read < req->content_len) {
        int n=httpd_req_recv(req,(char *)body+read,req->content_len-read);
        if (n<=0) {
            heap_caps_free(body);
            return text_reply(req,"408 Request Timeout","JR_AUDIO_ERROR corpo_incompleto");
        }
        read+=(size_t)n;
    }

    size_t samples=0;
    if (!valid_wav(body,read,&samples)) {
        heap_caps_free(body);
        return text_reply(req,"415 Unsupported Media Type","JR_AUDIO_ERROR esperado_wav_pcm16_mono_16000hz");
    }

    esp_err_t err=jr_audio_play_pcm16_mono((const int16_t *)(body+44),samples,JR_AUDIO_RX_RATE);
    heap_caps_free(body);
    if (err!=ESP_OK) {
        char message[96];
        snprintf(message,sizeof(message),"JR_AUDIO_ERROR playback=%s",esp_err_to_name(err));
        return text_reply(req,"409 Conflict",message);
    }

    char message[96];
    snprintf(message,sizeof(message),"JR_OK audio_received samples=%u rate=%d",(unsigned)samples,JR_AUDIO_RX_RATE);
    return text_reply(req,"200 OK",message);
}
