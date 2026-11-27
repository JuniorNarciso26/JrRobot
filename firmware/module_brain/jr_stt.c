/* Experimental fixed-window STT. Caller owns the mic and releases it before inference. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "jr_usb_terminal.h"
#include "jr_stt.h"
#include "jr_stt_aliases.h"
#include "jr_face.h"
#include "sram_pool.h"
#include "stt_engine.h"
#define RATE 16000
#define SAMPLES (5 * RATE)
static int16_t *pcm;
static bool captured;
static void emit(const char *line) {
    ESP_LOGI("jrbot_stt", "%s", line);
    jr_usb_terminal_emit(line);
}
esp_err_t jr_stt_reserve(void) {
    return sram_pool_reserve() ? ESP_ERR_NO_MEM : ESP_OK;
}
esp_err_t jr_stt_open(void) {
    if (pcm) return ESP_OK;
    int rc=stt_engine_open();
    if (rc) { stt_engine_close(); emit("JR_STT_ERROR stage=model"); return ESP_FAIL; }
    pcm=heap_caps_malloc(SAMPLES*sizeof(*pcm), MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if (!pcm) { stt_engine_close(); return ESP_ERR_NO_MEM; }
    emit("JR_STT_READY engine=babytalk language=en window_ms=5000 actions=happy_sad_aliases");
    return ESP_OK;
}
void jr_stt_close(void) {
    free(pcm); pcm=NULL; captured=false;
    stt_engine_close();
}
esp_err_t jr_stt_capture(i2s_chan_handle_t rx, const volatile bool *cancel) {
    captured=false;
    if (!pcm || !rx) return ESP_ERR_INVALID_STATE;
    int32_t raw[256];
    /* Flush microphone startup transient before the capture prompt. */
    for(int i=0;i<8;++i) {
        size_t bytes=0;
        if (*cancel) return ESP_ERR_INVALID_STATE;
        esp_err_t err=i2s_channel_read(rx,raw,sizeof(raw),&bytes,350);
        if(err!=ESP_OK || bytes!=sizeof(raw)) return ESP_FAIL;
    }
    emit("JR_STT_LISTENING: FALE AGORA em ingles | janela=5s");
    int peak=0; size_t count=0;
    const int64_t start=esp_timer_get_time();
    while(count<SAMPLES) {
        if (*cancel) { emit("JR_STT_CANCELLED"); return ESP_ERR_INVALID_STATE; }
        size_t bytes=0;
        esp_err_t err=i2s_channel_read(rx,raw,sizeof(raw),&bytes,350);
        if(err!=ESP_OK || !bytes || bytes>sizeof(raw) || bytes%sizeof(*raw) ||
           esp_timer_get_time()-start>8000000) {
            emit("JR_STT_ERROR stage=capture"); return ESP_FAIL;
        }
        for(size_t j=0;j<bytes/sizeof(*raw) && count<SAMPLES;++j) {
            int value=raw[j]>>16; /* Same 1x PCM conversion validated in isolated test. */
            pcm[count++]=(int16_t)value;
            int magnitude=value<0 ? -value : value;
            if(magnitude>peak) peak=magnitude;
        }
    }
    captured=true;
    char line[128];
    snprintf(line,sizeof(line),"JR_STT_CAPTURED samples=%u peak=%d gain=1",(unsigned)count,peak);
    emit(line);
    return ESP_OK;
}
void jr_stt_transcribe(const volatile bool *cancel) {
    if(!captured || *cancel) return;
    captured=false;
    emit("JR_STT_PROCESSING");
    const int64_t start=esp_timer_get_time();
    stt_result_t result;
    int rc=stt_engine_run(pcm,SAMPLES,&result);
    if(*cancel) { emit("JR_STT_CANCELLED"); return; }
    if(rc) { emit("JR_STT_ERROR stage=inference"); return; }
    char line[480];
    const char *text=result.text[0]?result.text:"(vazio)";
    for(size_t offset=0;offset<strlen(text);offset+=440) {
        snprintf(line,sizeof(line),"%s: %.440s",offset?"JR_STT_TEXT_CONT":"JR_STT_TEXT",text+offset);
        emit(line);
    }
    snprintf(line,sizeof(line),"JR_STT_TIME total_ms=%.1f frames=%d",(esp_timer_get_time()-start)/1000.0,result.frames);
    emit(line);
    const jr_stt_face_match_t *match=jr_stt_match_face(result.text);
    if (!match) {
        emit("JR_STT_COMMAND result=unmatched action=none");
        return;
    }
    if (*cancel) { emit("JR_STT_CANCELLED"); return; }
    bool applied=jr_face_set_expression(match->expression);
    if (applied) jr_face_increment_command_count();
    snprintf(line,sizeof(line),"JR_STT_COMMAND action=face expression=%s alias=\"%s\" result=%s",
             match->expression,match->alias,applied?"ok":"error");
    emit(line);
}
