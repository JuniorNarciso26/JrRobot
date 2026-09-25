#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_peer.h"
#include "esp_peer_default.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "jr_audio.h"
#include "jr_board.h"
#include "jr_brain.h"
#include "jr_mode_manager.h"
#include "jr_resource_manager.h"
#include "jr_camera_diag.h"
#include "jr_webrtc_audio.h"
#include "jr_wifi.h"

#define JR_WA_I2S_RATE                  16000
#define JR_WA_WEBRTC_RATE               8000
#define JR_WA_I2S_FRAMES_20MS           320
#define JR_WA_G711_SAMPLES_20MS         160
#define JR_WA_RX_PACKET_MAX             512
#define JR_WA_AUDIO_QUEUE_DEPTH         12
#define JR_WA_SIGNAL_QUEUE_DEPTH        16
#define JR_WA_MIC_TASK_STACK            8192
#define JR_WA_PLAY_TASK_STACK           12288
#define JR_WA_PEER_TASK_STACK           (25 * 1024)
#define JR_WA_TASK_PRIORITY             7
#define JR_WA_PEER_TASK_PRIORITY        18
#define JR_WA_STOP_WAIT_MS              2500
#define JR_WA_SIGNAL_POST_MAX           (16 * 1024)
#define JR_WA_VIDEO_DC_CHUNK_SIZE        10000
#define JR_WA_VIDEO_DC_HEADER_SIZE       5
#define JR_WA_VIDEO_DC_END               0x80
#define JR_WA_VIDEO_DC_SEQ_MASK          0x7F
#define JR_WA_VIDEO_TASK_STACK           6144
#define JR_WA_VIDEO_DC_LABEL             "jrbot-video"

typedef enum {
    JR_WA_VIDEO_PROFILE_FAST = 0,
    JR_WA_VIDEO_PROFILE_BALANCED,
    JR_WA_VIDEO_PROFILE_QUALITY,
} jr_wa_video_profile_t;

static const char *TAG = "jrbot_webrtc_audio";

static void log_heap_state(const char *stage)
{
    ESP_LOGI(TAG,
             "HEAP stage=%s internal_free=%u internal_largest=%u psram_free=%u psram_largest=%u",
             stage,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

typedef struct {
    uint16_t size;
    uint32_t pts;
    uint8_t data[JR_WA_RX_PACKET_MAX];
} jr_wa_audio_packet_t;

typedef struct {
    bool active;
    bool peer_connected;
    bool bus_owned;
    bool autonomous_before_live;
    bool autonomous_suspended;
    bool autonomous_restore_pending;
    bool autonomous_restored;
    esp_err_t autonomous_suspend_error;
    esp_err_t autonomous_restore_error;
    int peer_state;
    uint32_t sessions;
    uint64_t mic_i2s_frames;
    uint32_t mic_i2s_errors;
    uint32_t webrtc_tx_packets;
    uint32_t webrtc_tx_bytes;
    uint32_t webrtc_tx_errors;
    uint32_t webrtc_rx_packets;
    uint32_t webrtc_rx_bytes;
    uint32_t webrtc_rx_drops;
    uint64_t speaker_i2s_frames;
    uint32_t speaker_i2s_errors;
    bool video_dc_open;
    uint16_t video_dc_stream_id;
    uint32_t video_frames;
    uint32_t video_bytes;
    uint32_t video_drops;
    uint32_t video_send_errors;
} jr_wa_status_t;

static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;
static jr_wa_status_t status_state;

static SemaphoreHandle_t session_mutex;
static QueueHandle_t signal_queue;
static QueueHandle_t audio_queue;

static i2s_chan_handle_t tx_chan;
static i2s_chan_handle_t rx_chan;
static esp_peer_handle_t peer;

static TaskHandle_t peer_task_handle;
static TaskHandle_t mic_task_handle;
static TaskHandle_t play_task_handle;
static TaskHandle_t video_task_handle;

static volatile bool session_running;
static volatile bool peer_loop_running;
static volatile bool video_dc_open;
static jr_mode_t live_previous_mode = JR_MODE_IDLE;
static bool live_mode_entered;
static uint16_t video_dc_stream_id;
static jr_wa_video_profile_t video_profile = JR_WA_VIDEO_PROFILE_BALANCED;

static void video_send_task(void *arg);

static void video_profile_params(jr_wa_video_profile_t profile,
                                 jr_camera_size_t *size,
                                 int *quality,
                                 int *fps,
                                 const char **name)
{
    switch (profile) {
        case JR_WA_VIDEO_PROFILE_FAST:
            if (size) *size = JR_CAMERA_SIZE_QVGA;
            if (quality) *quality = 16;
            if (fps) *fps = 15;
            if (name) *name = "fast";
            break;
        case JR_WA_VIDEO_PROFILE_QUALITY:
            if (size) *size = JR_CAMERA_SIZE_SVGA;
            if (quality) *quality = 16;
            if (fps) *fps = 7;
            if (name) *name = "quality";
            break;
        case JR_WA_VIDEO_PROFILE_BALANCED:
        default:
            if (size) *size = JR_CAMERA_SIZE_VGA;
            if (quality) *quality = 14;
            if (fps) *fps = 10;
            if (name) *name = "balanced";
            break;
    }
}

static httpd_req_t *event_stream_req;
static volatile bool event_stream_connected;
static volatile bool event_stream_stopping;

static esp_peer_default_cfg_t peer_extra_cfg = {
    .agent_recv_timeout = 200,
    .data_ch_cfg = {
        .cache_timeout = 1000,
        .send_cache_size = 24 * 1024,
        .recv_cache_size = 4 * 1024,
    },
    .rtp_cfg = {
        .audio_recv_jitter = {
            .cache_timeout = 120,
            .resend_delay = 20,
            .cache_size = 1024,
        },
        .send_pool_size = 1024,
        .send_queue_num = 10,
        .max_resend_count = 3,
    },
    .max_candidates = 4,
    .tcp_support = false,
};

static const char WEBRTC_AUDIO_HTML[] =
"<!doctype html><html lang='pt-BR'><head>"
"<meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>JrBot WebRTC Audio</title>"
"<style>body{font-family:system-ui,sans-serif;max-width:760px;margin:24px auto;padding:0 16px;background:#111;color:#eee}"
"button{font-size:18px;padding:12px 18px;margin:5px;border-radius:10px;border:0}"
"#connect{background:#2e7d32;color:white}#disconnect{background:#8e2424;color:white}"
"pre{white-space:pre-wrap;background:#1e1e1e;padding:12px;border-radius:10px;min-height:120px}"
".ok{color:#76ff76}.warn{color:#ffd166}</style></head><body>"
"<h2>JrBot - WebRTC Audio Local</h2>"
"<p>Audio WebRTC local full-duplex via PCMA. O video Live pode permanecer aberto em outra aba.</p>"
"<button id='connect'>Conectar audio</button><button id='disconnect' disabled>Desconectar</button>"
"<p id='state' class='warn'>desconectado</p>"
"<audio id='remoteAudio' autoplay playsinline controls></audio>"
"<pre id='diag'>Aguardando...</pre>"
"<script>"
"const state=document.getElementById('state'),diag=document.getElementById('diag'),remote=document.getElementById('remoteAudio');"
"let pc=null,es=null,localStream=null,answerSent=false,pendingLocal=[],pendingRemote=[];"
"function setState(s,ok=false){state.textContent=s;state.className=ok?'ok':'warn';}"
"async function post(o){const r=await fetch('/webrtc-audio/signal',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(o)});if(!r.ok)throw new Error(await r.text());return r.text();}"
"async function addRemoteCandidate(c){if(!pc||!pc.remoteDescription){pendingRemote.push(c);return;}const cand=c.startsWith('a=')?c.slice(2):c;await pc.addIceCandidate(new RTCIceCandidate({candidate:cand,sdpMid:'0',sdpMLineIndex:0}));}"
"async function handleSignal(m){"
" if(m.type==='offer'){"
"  await pc.setRemoteDescription({type:'offer',sdp:m.sdp});"
"  for(const c of pendingRemote.splice(0))await addRemoteCandidate(c);"
"  const ans=await pc.createAnswer();await pc.setLocalDescription(ans);"
"  await post({type:'answer',sdp:pc.localDescription.sdp});answerSent=true;"
"  for(const c of pendingLocal.splice(0))await post({type:'candidate',candidate:c});"
" }else if(m.type==='candidate'){await addRemoteCandidate(m.candidate);}"
" else if(m.type==='error'){throw new Error(m.message||'erro do JrBot');}"
"}"
"async function connect(){"
" document.getElementById('connect').disabled=true;setState('solicitando microfone...');"
" try{"
"  localStream=await navigator.mediaDevices.getUserMedia({audio:{echoCancellation:true,noiseSuppression:true,autoGainControl:true},video:false});"
"  pc=new RTCPeerConnection({iceServers:[]});"
"  localStream.getTracks().forEach(t=>pc.addTrack(t,localStream));"
"  pc.ontrack=e=>{remote.srcObject=e.streams[0];remote.play().catch(()=>{});};"
"  pc.onicecandidate=async e=>{if(!e.candidate)return;const c=e.candidate.candidate;if(answerSent){try{await post({type:'candidate',candidate:c});}catch(err){console.error(err);}}else pendingLocal.push(c);};"
"  pc.onconnectionstatechange=()=>{const s=pc.connectionState;setState('WebRTC: '+s,s==='connected');};"
"  es=new EventSource('/webrtc-audio/signal');"
"  es.onmessage=async e=>{try{await handleSignal(JSON.parse(e.data));}catch(err){console.error(err);setState('erro sinalizacao: '+err.message);}};"
"  es.onerror=()=>{if(es&&es.readyState===EventSource.CLOSED)setState('sinalizacao fechada');};"
"  document.getElementById('disconnect').disabled=false;"
" }catch(err){setState('erro: '+err.message);cleanupLocal();document.getElementById('connect').disabled=false;}"
"}"
"function cleanupLocal(){if(es){es.close();es=null;}if(pc){pc.close();pc=null;}if(localStream){localStream.getTracks().forEach(t=>t.stop());localStream=null;}remote.srcObject=null;answerSent=false;pendingLocal=[];pendingRemote=[];}"
"async function disconnect(){try{await post({type:'bye'});}catch(e){}cleanupLocal();setState('desconectado');document.getElementById('connect').disabled=false;document.getElementById('disconnect').disabled=true;}"
"document.getElementById('connect').onclick=connect;document.getElementById('disconnect').onclick=disconnect;"
"setInterval(async()=>{try{const r=await fetch('/webrtc-audio/status',{cache:'no-store'});diag.textContent=await r.text();}catch(e){diag.textContent='status indisponivel: '+e.message;}},5000);"
"</script></body></html>";

static bool get_active(void)
{
    bool value;
    portENTER_CRITICAL(&state_lock);
    value = status_state.active;
    portEXIT_CRITICAL(&state_lock);
    return value;
}

static bool get_peer_connected(void)
{
    bool value;
    portENTER_CRITICAL(&state_lock);
    value = status_state.peer_connected;
    portEXIT_CRITICAL(&state_lock);
    return value;
}

static void set_active(bool active)
{
    portENTER_CRITICAL(&state_lock);
    status_state.active = active;
    portEXIT_CRITICAL(&state_lock);
}

static void set_peer_connected(bool connected)
{
    portENTER_CRITICAL(&state_lock);
    status_state.peer_connected = connected;
    portEXIT_CRITICAL(&state_lock);
}

static void set_peer_state(int state)
{
    portENTER_CRITICAL(&state_lock);
    status_state.peer_state = state;
    portEXIT_CRITICAL(&state_lock);
}

static void reset_session_counters(void)
{
    portENTER_CRITICAL(&state_lock);
    status_state.mic_i2s_frames = 0;
    status_state.mic_i2s_errors = 0;
    status_state.webrtc_tx_packets = 0;
    status_state.webrtc_tx_bytes = 0;
    status_state.webrtc_tx_errors = 0;
    status_state.webrtc_rx_packets = 0;
    status_state.webrtc_rx_bytes = 0;
    status_state.webrtc_rx_drops = 0;
    status_state.speaker_i2s_frames = 0;
    status_state.speaker_i2s_errors = 0;
    status_state.video_dc_open = false;
    status_state.video_dc_stream_id = 0;
    status_state.video_frames = 0;
    status_state.video_bytes = 0;
    status_state.video_drops = 0;
    status_state.video_send_errors = 0;
    status_state.autonomous_before_live = false;
    status_state.autonomous_suspended = false;
    status_state.autonomous_restore_pending = false;
    status_state.autonomous_restored = false;
    status_state.autonomous_suspend_error = ESP_OK;
    status_state.autonomous_restore_error = ESP_OK;
    status_state.peer_state = ESP_PEER_STATE_CLOSED;
    portEXIT_CRITICAL(&state_lock);
}

static esp_err_t suspend_autonomous_for_live(void)
{
    bool previous = jr_brain_enabled();

    portENTER_CRITICAL(&state_lock);
    status_state.autonomous_before_live = previous;
    status_state.autonomous_suspended = false;
    status_state.autonomous_restore_pending = false;
    status_state.autonomous_restored = false;
    status_state.autonomous_suspend_error = ESP_OK;
    status_state.autonomous_restore_error = ESP_OK;
    portEXIT_CRITICAL(&state_lock);

    ESP_LOGI(TAG, "JR_MODE autonomous_suspend previous=%d reason=live", previous ? 1 : 0);

    if (!previous) {
        ESP_LOGI(TAG, "JR_MODE autonomous_suspend previous=0 suspended=0 result=not_needed");
        return ESP_OK;
    }

    esp_err_t err = jr_brain_set_enabled(false);

    portENTER_CRITICAL(&state_lock);
    status_state.autonomous_suspended = (err == ESP_OK);
    status_state.autonomous_restore_pending = (err == ESP_OK);
    status_state.autonomous_suspend_error = err;
    portEXIT_CRITICAL(&state_lock);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "JR_MODE autonomous_suspend previous=1 suspended=1 result=ESP_OK");
    } else {
        ESP_LOGE(TAG, "JR_MODE autonomous_suspend previous=1 suspended=0 result=%s",
                 esp_err_to_name(err));
    }
    return err;
}

static void restore_mode_after_live(const char *reason)
{
    if (!live_mode_entered) return;

    jr_mode_t target = live_previous_mode;
    if (target == JR_MODE_AUTONOMOUS && !jr_brain_enabled()) {
        target = JR_MODE_IDLE;
    }

    (void)jr_mode_set(target, reason ? reason : "live_end");
    live_mode_entered = false;
}

static void restore_autonomous_after_live(const char *reason)
{
    bool previous;
    bool suspended;
    bool pending;

    portENTER_CRITICAL(&state_lock);
    previous = status_state.autonomous_before_live;
    suspended = status_state.autonomous_suspended;
    pending = status_state.autonomous_restore_pending;
    portEXIT_CRITICAL(&state_lock);

    if (!pending) {
        ESP_LOGI(TAG,
                 "JR_MODE autonomous_resume previous=%d suspended=%d restored=0 pending=0 reason=%s",
                 previous ? 1 : 0, suspended ? 1 : 0, reason ? reason : "unknown");
        restore_mode_after_live(reason);
        return;
    }

    esp_err_t err = jr_brain_set_enabled(true);

    portENTER_CRITICAL(&state_lock);
    status_state.autonomous_restore_pending = false;
    status_state.autonomous_restored = (err == ESP_OK);
    status_state.autonomous_restore_error = err;
    portEXIT_CRITICAL(&state_lock);

    if (err == ESP_OK) {
        ESP_LOGI(TAG,
                 "JR_MODE autonomous_resume previous=1 suspended=1 restored=1 pending=0 reason=%s result=ESP_OK",
                 reason ? reason : "unknown");
    } else {
        ESP_LOGE(TAG,
                 "JR_MODE autonomous_resume previous=1 suspended=1 restored=0 pending=0 reason=%s result=%s",
                 reason ? reason : "unknown", esp_err_to_name(err));
    }

    restore_mode_after_live(err == ESP_OK ? reason : "live_resume_failed");
}

static int16_t clamp16(int32_t v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (int16_t)v;
}

static uint8_t linear_to_alaw(int16_t pcm)
{
    static const int16_t seg_end[8] = {
        0x00FF, 0x01FF, 0x03FF, 0x07FF,
        0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF
    };
    int pcm_val = pcm;
    int mask;
    if (pcm_val >= 0) {
        mask = 0xD5;
    } else {
        mask = 0x55;
        pcm_val = -pcm_val - 1;
        if (pcm_val < 0) pcm_val = 32767;
    }
    if (pcm_val > 32635) pcm_val = 32635;

    int seg = 0;
    while (seg < 8 && pcm_val > seg_end[seg]) seg++;

    uint8_t aval;
    if (seg >= 8) {
        aval = 0x7F;
    } else {
        aval = (uint8_t)(seg << 4);
        if (seg < 2) aval |= (uint8_t)((pcm_val >> 4) & 0x0F);
        else aval |= (uint8_t)((pcm_val >> (seg + 3)) & 0x0F);
    }
    return (uint8_t)(aval ^ mask);
}

static int16_t alaw_to_linear(uint8_t a)
{
    a ^= 0x55;
    int t = (a & 0x0F) << 4;
    int seg = (a & 0x70) >> 4;
    switch (seg) {
        case 0: t += 8; break;
        case 1: t += 0x108; break;
        default:
            t += 0x108;
            t <<= (seg - 1);
            break;
    }
    return (a & 0x80) ? (int16_t)t : (int16_t)-t;
}

static void release_i2s_pins(void)
{
    (void)gpio_reset_pin((gpio_num_t)JR_AUDIO_BCLK_GPIO);
    (void)gpio_set_pull_mode((gpio_num_t)JR_AUDIO_BCLK_GPIO, GPIO_FLOATING);
    (void)gpio_set_direction((gpio_num_t)JR_AUDIO_BCLK_GPIO, GPIO_MODE_INPUT);
    (void)gpio_reset_pin((gpio_num_t)JR_AUDIO_LRC_GPIO);
    (void)gpio_set_pull_mode((gpio_num_t)JR_AUDIO_LRC_GPIO, GPIO_FLOATING);
    (void)gpio_set_direction((gpio_num_t)JR_AUDIO_LRC_GPIO, GPIO_MODE_INPUT);
    (void)gpio_set_pull_mode((gpio_num_t)JR_MIC_SD_GPIO, GPIO_PULLDOWN_ONLY);
    (void)gpio_set_direction((gpio_num_t)JR_MIC_SD_GPIO, GPIO_MODE_INPUT);
    (void)gpio_set_direction((gpio_num_t)JR_AUDIO_DIN_GPIO, GPIO_MODE_OUTPUT);
    (void)gpio_set_level((gpio_num_t)JR_AUDIO_DIN_GPIO, 0);
}

static void close_i2s_pair(void)
{
    if (rx_chan) {
        (void)i2s_channel_disable(rx_chan);
        (void)i2s_del_channel(rx_chan);
        rx_chan = NULL;
    }
    if (tx_chan) {
        (void)i2s_channel_disable(tx_chan);
        (void)i2s_del_channel(tx_chan);
        tx_chan = NULL;
    }
    release_i2s_pins();
}

static esp_err_t open_i2s_pair(void)
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;

    esp_err_t err = i2s_new_channel(&chan_cfg, &tx_chan, &rx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S new pair failed: %s", esp_err_to_name(err));
        return err;
    }

    i2s_std_config_t cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(JR_WA_I2S_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = JR_AUDIO_BCLK_GPIO,
            .ws = JR_AUDIO_LRC_GPIO,
            .dout = JR_AUDIO_DIN_GPIO,
            .din = JR_MIC_SD_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(tx_chan, &cfg);
    if (err != ESP_OK) goto fail;
    err = i2s_channel_init_std_mode(rx_chan, &cfg);
    if (err != ESP_OK) goto fail;
    err = i2s_channel_enable(tx_chan);
    if (err != ESP_OK) goto fail;
    err = i2s_channel_enable(rx_chan);
    if (err != ESP_OK) goto fail;

    ESP_LOGI(TAG,
             "I2S_FDX_READY rate=16000 format=PHILIPS slot=32x2 bclk=%d ws=%d rx=%d tx=%d",
             JR_AUDIO_BCLK_GPIO, JR_AUDIO_LRC_GPIO, JR_MIC_SD_GPIO, JR_AUDIO_DIN_GPIO);
    return ESP_OK;

fail:
    ESP_LOGE(TAG, "I2S pair init failed: %s", esp_err_to_name(err));
    close_i2s_pair();
    return err;
}

static void flush_signal_queue(void)
{
    if (!signal_queue) return;
    char *msg = NULL;
    while (xQueueReceive(signal_queue, &msg, 0) == pdTRUE) {
        free(msg);
        msg = NULL;
    }
}

static void flush_audio_queue(void)
{
    if (!audio_queue) return;
    jr_wa_audio_packet_t packet;
    while (xQueueReceive(audio_queue, &packet, 0) == pdTRUE) {
    }
}

static int queue_signal_object(const char *type, const uint8_t *data, int size)
{
    if (!signal_queue || !type) return -1;

    cJSON *root = cJSON_CreateObject();
    if (!root) return -1;
    cJSON_AddStringToObject(root, "type", type);

    if (data && size > 0) {
        char *copy = malloc((size_t)size + 1);
        if (!copy) {
            cJSON_Delete(root);
            return -1;
        }
        memcpy(copy, data, (size_t)size);
        copy[size] = 0;
        if (strcmp(type, "offer") == 0) {
            cJSON_AddStringToObject(root, "sdp", copy);
        } else if (strcmp(type, "candidate") == 0) {
            cJSON_AddStringToObject(root, "candidate", copy);
        } else {
            cJSON_AddStringToObject(root, "message", copy);
        }
        free(copy);
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return -1;

    if (xQueueSend(signal_queue, &json, pdMS_TO_TICKS(100)) != pdTRUE) {
        free(json);
        return -1;
    }
    return 0;
}

static int peer_state_handler(esp_peer_state_t state, void *ctx)
{
    (void)ctx;
    set_peer_state((int)state);
    ESP_LOGI(TAG, "PEER_STATE state=%d", (int)state);

    if (state == ESP_PEER_STATE_CONNECTED) {
        set_peer_connected(true);
        ESP_LOGI(TAG, "WEBRTC_CONNECTED codec=PCMA rate=8000 mono=1 video=JPEG_DATA_CHANNEL");
    } else if (state == ESP_PEER_STATE_DISCONNECTED ||
               state == ESP_PEER_STATE_CONNECT_FAILED ||
               state == ESP_PEER_STATE_CLOSED) {
        set_peer_connected(false);
    }
    return 0;
}

static int peer_msg_handler(esp_peer_msg_t *msg, void *ctx)
{
    (void)ctx;
    if (!msg) return -1;

    if (msg->type == ESP_PEER_MSG_TYPE_SDP) {
        ESP_LOGI(TAG, "SIGNAL_LOCAL offer_bytes=%d", msg->size);
        return queue_signal_object("offer", msg->data, msg->size);
    }
    if (msg->type == ESP_PEER_MSG_TYPE_CANDIDATE) {
        ESP_LOGI(TAG, "SIGNAL_LOCAL candidate_bytes=%d", msg->size);
        return queue_signal_object("candidate", msg->data, msg->size);
    }
    return 0;
}

static int peer_audio_info_handler(esp_peer_audio_stream_info_t *info, void *ctx)
{
    (void)ctx;
    if (!info) return 0;
    ESP_LOGI(TAG, "REMOTE_AUDIO_INFO codec=%d rate=%lu channel=%u",
             (int)info->codec, (unsigned long)info->sample_rate, info->channel);
    return 0;
}

static int peer_audio_data_handler(esp_peer_audio_frame_t *frame, void *ctx)
{
    (void)ctx;
    if (!frame || !frame->data || frame->size <= 0 || !audio_queue || !get_active()) return 0;

    if (frame->size > JR_WA_RX_PACKET_MAX) {
        portENTER_CRITICAL(&state_lock);
        status_state.webrtc_rx_drops++;
        portEXIT_CRITICAL(&state_lock);
        ESP_LOGW(TAG, "RX_DROP oversize=%d", frame->size);
        return 0;
    }

    jr_wa_audio_packet_t packet = {
        .size = (uint16_t)frame->size,
        .pts = frame->pts,
    };
    memcpy(packet.data, frame->data, (size_t)frame->size);

    if (xQueueSend(audio_queue, &packet, 0) != pdTRUE) {
        portENTER_CRITICAL(&state_lock);
        status_state.webrtc_rx_drops++;
        portEXIT_CRITICAL(&state_lock);
        return 0;
    }

    portENTER_CRITICAL(&state_lock);
    status_state.webrtc_rx_packets++;
    status_state.webrtc_rx_bytes += (uint32_t)frame->size;
    portEXIT_CRITICAL(&state_lock);
    return 0;
}

static int peer_video_info_handler(esp_peer_video_stream_info_t *info, void *ctx)
{
    (void)info;
    (void)ctx;
    return 0;
}

static int peer_video_data_handler(esp_peer_video_frame_t *frame, void *ctx)
{
    (void)frame;
    (void)ctx;
    return 0;
}

static int peer_channel_open_handler(esp_peer_data_channel_info_t *ch, void *ctx)
{
    (void)ctx;
    if (!ch || !ch->label) return 0;
    ESP_LOGI(TAG, "DATA_CHANNEL_OPEN label=%s stream_id=%u", ch->label, ch->stream_id);
    if (strcmp(ch->label, JR_WA_VIDEO_DC_LABEL) == 0) {
        video_dc_stream_id = ch->stream_id;
        video_dc_open = true;
        portENTER_CRITICAL(&state_lock);
        status_state.video_dc_open = true;
        status_state.video_dc_stream_id = ch->stream_id;
        portEXIT_CRITICAL(&state_lock);
        log_heap_state("video_dc_open");
        if (!video_task_handle) {
            BaseType_t created = xTaskCreate(
                video_send_task, "jr_wa_video", JR_WA_VIDEO_TASK_STACK,
                NULL, JR_WA_TASK_PRIORITY, &video_task_handle);
            if (created != pdPASS) {
                ESP_LOGE(TAG, "VIDEO_DC task allocation failed");
                video_task_handle = NULL;
            }
        }
    }
    return 0;
}

static int peer_data_handler(esp_peer_data_frame_t *frame, void *ctx)
{
    (void)frame;
    (void)ctx;
    return 0;
}

static int peer_channel_close_handler(esp_peer_data_channel_info_t *ch, void *ctx)
{
    (void)ctx;
    if (ch) {
        ESP_LOGI(TAG, "DATA_CHANNEL_CLOSE label=%s stream_id=%u",
                 ch->label ? ch->label : "?", ch->stream_id);
    }
    video_dc_open = false;
    portENTER_CRITICAL(&state_lock);
    status_state.video_dc_open = false;
    portEXIT_CRITICAL(&state_lock);
    return 0;
}

static int send_jpeg_data_channel(const uint8_t *jpeg, size_t jpeg_size, uint8_t *chunk_buf)
{
    if (!peer || !video_dc_open || !jpeg || !jpeg_size || !chunk_buf) {
        return ESP_PEER_ERR_WRONG_STATE;
    }

    uint32_t chunk_count = (uint32_t)((jpeg_size + JR_WA_VIDEO_DC_CHUNK_SIZE - 1U) /
                                      JR_WA_VIDEO_DC_CHUNK_SIZE);
    if (chunk_count > (JR_WA_VIDEO_DC_SEQ_MASK + 1U)) {
        return ESP_PEER_ERR_INVALID_ARG;
    }

    size_t offset = 0;
    uint8_t seq = 0;
    while (offset < jpeg_size && session_running && video_dc_open) {
        size_t payload = jpeg_size - offset;
        if (payload > JR_WA_VIDEO_DC_CHUNK_SIZE) payload = JR_WA_VIDEO_DC_CHUNK_SIZE;

        uint8_t chunk_id = seq & JR_WA_VIDEO_DC_SEQ_MASK;
        if (offset + payload >= jpeg_size) chunk_id |= JR_WA_VIDEO_DC_END;

        chunk_buf[0] = chunk_id;
        chunk_buf[1] = (uint8_t)((payload >> 24) & 0xFF);
        chunk_buf[2] = (uint8_t)((payload >> 16) & 0xFF);
        chunk_buf[3] = (uint8_t)((payload >> 8) & 0xFF);
        chunk_buf[4] = (uint8_t)(payload & 0xFF);
        memcpy(chunk_buf + JR_WA_VIDEO_DC_HEADER_SIZE, jpeg + offset, payload);

        esp_peer_data_frame_t data_frame = {
            .type = ESP_PEER_DATA_CHANNEL_DATA,
            .stream_id = video_dc_stream_id,
            .data = chunk_buf,
            .size = (int)(JR_WA_VIDEO_DC_HEADER_SIZE + payload),
        };

        int ret = esp_peer_send_data(peer, &data_frame);
        if (ret != ESP_PEER_ERR_NONE) return ret;

        offset += payload;
        seq++;
    }
    return ESP_PEER_ERR_NONE;
}

static void video_send_task(void *arg)
{
    (void)arg;
    uint8_t *chunk_buf = heap_caps_malloc(JR_WA_VIDEO_DC_HEADER_SIZE + JR_WA_VIDEO_DC_CHUNK_SIZE,
                                          MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!chunk_buf) {
        ESP_LOGE(TAG, "VIDEO_DC chunk buffer allocation failed");
        video_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    jr_camera_size_t initial_size;
    int initial_quality = 14;
    int initial_fps = 10;
    const char *initial_name = "balanced";
    portENTER_CRITICAL(&state_lock);
    jr_wa_video_profile_t initial_profile = video_profile;
    portEXIT_CRITICAL(&state_lock);
    video_profile_params(initial_profile, &initial_size, &initial_quality, &initial_fps, &initial_name);

    ESP_LOGI(TAG, "VIDEO_DC_TASK_READY profile=%s size=%s quality=%d fps=%d chunk=%d",
             initial_name, jr_camera_size_name(initial_size), initial_quality,
             initial_fps, JR_WA_VIDEO_DC_CHUNK_SIZE);

    while (session_running) {
        if (!get_peer_connected() || !video_dc_open || !peer) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        jr_wa_video_profile_t profile;
        portENTER_CRITICAL(&state_lock);
        profile = video_profile;
        portEXIT_CRITICAL(&state_lock);

        jr_camera_size_t frame_size;
        int frame_quality = 14;
        int frame_fps = 10;
        const char *profile_name = "balanced";
        video_profile_params(profile, &frame_size, &frame_quality, &frame_fps, &profile_name);

        int64_t started_us = esp_timer_get_time();
        jr_camera_jpeg_t frame = {0};
        esp_err_t cam_err = jr_camera_capture_jpeg_ex(&frame, frame_size, frame_quality);
        if (cam_err != ESP_OK) {
            portENTER_CRITICAL(&state_lock);
            status_state.video_drops++;
            portEXIT_CRITICAL(&state_lock);
            ESP_LOGW(TAG, "VIDEO_CAPTURE_DROP err=%s", esp_err_to_name(cam_err));
            int retry_ms = frame_fps > 0 ? (1000 / frame_fps) : 100;
            vTaskDelay(pdMS_TO_TICKS(retry_ms));
            continue;
        }

        int ret = send_jpeg_data_channel(frame.data, frame.len, chunk_buf);
        portENTER_CRITICAL(&state_lock);
        if (ret == ESP_PEER_ERR_NONE) {
            status_state.video_frames++;
            status_state.video_bytes += (uint32_t)frame.len;
        } else {
            status_state.video_drops++;
            status_state.video_send_errors++;
        }
        uint32_t sent_frames = status_state.video_frames;
        portEXIT_CRITICAL(&state_lock);

        if (ret == ESP_PEER_ERR_WOULD_BLOCK) {
            ESP_LOGW(TAG, "VIDEO_DC_DROP buffer_full bytes=%u", (unsigned)frame.len);
        } else if (ret != ESP_PEER_ERR_NONE) {
            ESP_LOGW(TAG, "VIDEO_DC_SEND_ERROR ret=%d bytes=%u", ret, (unsigned)frame.len);
        } else if ((sent_frames % 25U) == 0U) {
            ESP_LOGI(TAG, "WEBRTC_JPEG frames=%lu profile=%s size=%ux%u quality=%d bytes=%u",
                     (unsigned long)sent_frames, profile_name, frame.width, frame.height,
                     frame.jpeg_quality, (unsigned)frame.len);
        }

        jr_camera_jpeg_release(&frame);

        int frame_ms = frame_fps > 0 ? (1000 / frame_fps) : 100;
        int64_t elapsed_ms = (esp_timer_get_time() - started_us) / 1000;
        if (elapsed_ms < frame_ms) {
            vTaskDelay(pdMS_TO_TICKS(frame_ms - (int)elapsed_ms));
        } else {
            taskYIELD();
        }
    }

    heap_caps_free(chunk_buf);
    video_task_handle = NULL;
    vTaskDelete(NULL);
}

static void peer_loop_task(void *arg)
{
    (void)arg;
    while (peer_loop_running && peer) {
        int ret = esp_peer_main_loop(peer);
        if (ret != ESP_PEER_ERR_NONE) {
            ESP_LOGW(TAG, "PEER_LOOP ret=%d", ret);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    peer_task_handle = NULL;
    vTaskDelete(NULL);
}

static void mic_send_task(void *arg)
{
    (void)arg;
    int32_t raw[JR_WA_I2S_FRAMES_20MS * 2];
    uint8_t g711[JR_WA_G711_SAMPLES_20MS];

    while (session_running && rx_chan) {
        size_t bytes_read = 0;
        esp_err_t err = i2s_channel_read(
            rx_chan, raw, sizeof(raw), &bytes_read, pdMS_TO_TICKS(250));

        if (err == ESP_ERR_TIMEOUT) continue;
        if (err != ESP_OK) {
            portENTER_CRITICAL(&state_lock);
            status_state.mic_i2s_errors++;
            portEXIT_CRITICAL(&state_lock);
            ESP_LOGE(TAG, "MIC_I2S_ERROR %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        size_t frames = bytes_read / (sizeof(int32_t) * 2U);
        portENTER_CRITICAL(&state_lock);
        status_state.mic_i2s_frames += frames;
        portEXIT_CRITICAL(&state_lock);

        if (!get_peer_connected() || !peer || frames < JR_WA_I2S_FRAMES_20MS) {
            continue;
        }

        for (size_t i = 0; i < JR_WA_G711_SAMPLES_20MS; ++i) {
            size_t f0 = i * 2U;
            size_t f1 = f0 + 1U;
            int16_t s0 = clamp16(raw[f0 * 2U] >> 14);
            int16_t s1 = clamp16(raw[f1 * 2U] >> 14);
            int16_t filtered = (int16_t)(((int32_t)s0 + (int32_t)s1) / 2);
            g711[i] = linear_to_alaw(filtered);
        }

        esp_peer_audio_frame_t frame = {
            .pts = (uint32_t)(esp_timer_get_time() / 1000ULL),
            .data = g711,
            .size = sizeof(g711),
        };
        int ret = esp_peer_send_audio(peer, &frame);
        portENTER_CRITICAL(&state_lock);
        if (ret == ESP_PEER_ERR_NONE) {
            status_state.webrtc_tx_packets++;
            status_state.webrtc_tx_bytes += sizeof(g711);
        } else {
            status_state.webrtc_tx_errors++;
        }
        portEXIT_CRITICAL(&state_lock);
        if (ret != ESP_PEER_ERR_NONE) {
            ESP_LOGW(TAG, "WEBRTC_TX_ERROR ret=%d", ret);
        }
    }

    mic_task_handle = NULL;
    vTaskDelete(NULL);
}

static void playback_task(void *arg)
{
    (void)arg;
    jr_wa_audio_packet_t packet;
    int32_t out[JR_WA_RX_PACKET_MAX * 4];

    while (session_running && tx_chan) {
        size_t frames_out = 0;

        if (audio_queue && xQueueReceive(audio_queue, &packet, 0) == pdTRUE) {
            size_t samples = packet.size;
            if (samples > JR_WA_RX_PACKET_MAX) samples = JR_WA_RX_PACKET_MAX;

            for (size_t i = 0; i < samples; ++i) {
                int16_t pcm = alaw_to_linear(packet.data[i]);
                int32_t slot = (int32_t)pcm * 65536;
                for (int up = 0; up < 2; ++up) {
                    out[frames_out * 2U] = slot;
                    out[frames_out * 2U + 1U] = slot;
                    frames_out++;
                }
            }
        } else {
            frames_out = JR_WA_I2S_FRAMES_20MS;
            memset(out, 0, frames_out * 2U * sizeof(int32_t));
        }

        size_t requested = frames_out * 2U * sizeof(int32_t);
        size_t bytes_written = 0;
        esp_err_t err = i2s_channel_write(
            tx_chan, out, requested, &bytes_written, pdMS_TO_TICKS(500));

        portENTER_CRITICAL(&state_lock);
        if (err == ESP_OK && bytes_written == requested) {
            status_state.speaker_i2s_frames += frames_out;
        } else {
            status_state.speaker_i2s_errors++;
        }
        portEXIT_CRITICAL(&state_lock);

        if (err != ESP_OK || bytes_written != requested) {
            ESP_LOGW(TAG, "SPK_I2S_ERROR err=%s bytes=%u requested=%u",
                     esp_err_to_name(err), (unsigned)bytes_written, (unsigned)requested);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    play_task_handle = NULL;
    vTaskDelete(NULL);
}

static void wait_worker_tasks(void)
{
    TickType_t start = xTaskGetTickCount();
    TickType_t limit = pdMS_TO_TICKS(JR_WA_STOP_WAIT_MS);
    while ((peer_task_handle || mic_task_handle || play_task_handle || video_task_handle) &&
           (xTaskGetTickCount() - start) < limit) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void stop_session_unlocked(void)
{
    bool has_resources;
    bool restore_pending;
    portENTER_CRITICAL(&state_lock);
    has_resources = status_state.bus_owned || status_state.active;
    restore_pending = status_state.autonomous_restore_pending;
    portEXIT_CRITICAL(&state_lock);
    has_resources = has_resources || peer || tx_chan || rx_chan ||
                    peer_task_handle || mic_task_handle || play_task_handle || video_task_handle;
    if (!has_resources && !restore_pending && !live_mode_entered) return;

    if (has_resources) {
        ESP_LOGI(TAG, "JR_MODE live_stop begin");
        ESP_LOGI(TAG, "SESSION_STOP begin");
        session_running = false;
        peer_loop_running = false;
        video_dc_open = false;
        set_peer_connected(false);
        portENTER_CRITICAL(&state_lock);
        status_state.video_dc_open = false;
        portEXIT_CRITICAL(&state_lock);
        set_active(false);

        wait_worker_tasks();

        if (peer) {
            esp_peer_close(peer);
            peer = NULL;
        }

        close_i2s_pair();
        flush_audio_queue();

        bool release_bus = false;
        portENTER_CRITICAL(&state_lock);
        release_bus = status_state.bus_owned;
        status_state.bus_owned = false;
        portEXIT_CRITICAL(&state_lock);
        if (release_bus) {
            jr_resource_i2s_release(JR_I2S_OWNER_LIVE, "webrtc_live");
        }

        ESP_LOGI(TAG, "SESSION_STOP end");
        ESP_LOGI(TAG, "JR_MODE live_stop end");
    }

    restore_autonomous_after_live(has_resources ? "live_end" : "start_failed");
}

static esp_err_t start_session_unlocked(void)
{
    stop_session_unlocked();
    flush_signal_queue();
    flush_audio_queue();
    reset_session_counters();

    live_previous_mode = jr_mode_set(JR_MODE_LIVE, "webrtc_start");
    live_mode_entered = true;

    esp_err_t err = suspend_autonomous_for_live();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "JR_MODE live_start result=error stage=autonomous_suspend error=%s",
                 esp_err_to_name(err));
        restore_autonomous_after_live("suspend_failed");
        return err;
    }

    if (!jr_resource_i2s_acquire(JR_I2S_OWNER_LIVE, 4000, "webrtc_live")) {
        ESP_LOGE(TAG, "audio bus busy");
        ESP_LOGE(TAG, "JR_MODE live_start result=error stage=audio_bus error=%s",
                 esp_err_to_name(ESP_ERR_TIMEOUT));
        restore_autonomous_after_live("audio_bus_busy");
        return ESP_ERR_TIMEOUT;
    }
    portENTER_CRITICAL(&state_lock);
    status_state.bus_owned = true;
    portEXIT_CRITICAL(&state_lock);

    jr_audio_stop();

    err = open_i2s_pair();
    if (err != ESP_OK) goto fail;

    esp_peer_cfg_t cfg = {
        .role = ESP_PEER_ROLE_CONTROLLING,
        .audio_info = {
            .codec = ESP_PEER_AUDIO_CODEC_G711A,
            .sample_rate = JR_WA_WEBRTC_RATE,
            .channel = 1,
        },
        .audio_dir = ESP_PEER_MEDIA_DIR_SEND_RECV,
        .video_dir = ESP_PEER_MEDIA_DIR_NONE,
        .enable_data_channel = true,
        .manual_ch_create = true,
        .no_auto_reconnect = true,
        .on_state = peer_state_handler,
        .on_msg = peer_msg_handler,
        .on_audio_info = peer_audio_info_handler,
        .on_audio_data = peer_audio_data_handler,
        .on_video_info = peer_video_info_handler,
        .on_video_data = peer_video_data_handler,
        .on_channel_open = peer_channel_open_handler,
        .on_data = peer_data_handler,
        .on_channel_close = peer_channel_close_handler,
        .extra_cfg = &peer_extra_cfg,
        .extra_size = sizeof(peer_extra_cfg),
    };

    log_heap_state("before_cert");
    int cert_ret = esp_peer_pre_generate_cert();
    if (cert_ret != ESP_PEER_ERR_NONE) {
        ESP_LOGW(TAG, "DTLS certificate pre-generation ret=%d", cert_ret);
    }
    log_heap_state("before_peer_open");

    int ret = esp_peer_open(&cfg, esp_peer_get_default_impl(), &peer);
    log_heap_state("after_peer_open");
    if (ret != ESP_PEER_ERR_NONE || !peer) {
        ESP_LOGE(TAG, "esp_peer_open failed ret=%d", ret);
        err = ESP_FAIL;
        goto fail;
    }

    session_running = true;
    peer_loop_running = true;
    set_active(true);
    portENTER_CRITICAL(&state_lock);
    status_state.sessions++;
    portEXIT_CRITICAL(&state_lock);

    if (xTaskCreatePinnedToCore(peer_loop_task, "jr_wa_peer", JR_WA_PEER_TASK_STACK,
                                NULL, JR_WA_PEER_TASK_PRIORITY, &peer_task_handle, 1) != pdPASS) {
        err = ESP_ERR_NO_MEM;
        goto fail;
    }
    if (xTaskCreatePinnedToCore(mic_send_task, "jr_wa_mic", JR_WA_MIC_TASK_STACK,
                                NULL, JR_WA_TASK_PRIORITY, &mic_task_handle, 0) != pdPASS) {
        err = ESP_ERR_NO_MEM;
        goto fail;
    }
    if (xTaskCreatePinnedToCore(playback_task, "jr_wa_play", JR_WA_PLAY_TASK_STACK,
                                NULL, JR_WA_TASK_PRIORITY, &play_task_handle, 0) != pdPASS) {
        err = ESP_ERR_NO_MEM;
        goto fail;
    }

    ret = esp_peer_new_connection(peer);
    if (ret != ESP_PEER_ERR_NONE) {
        ESP_LOGE(TAG, "esp_peer_new_connection failed ret=%d", ret);
        err = ESP_FAIL;
        goto fail;
    }

    jr_camera_size_t session_video_size;
    int session_video_quality = 14;
    int session_video_fps = 10;
    const char *session_video_name = "balanced";
    portENTER_CRITICAL(&state_lock);
    jr_wa_video_profile_t session_profile = video_profile;
    portEXIT_CRITICAL(&state_lock);
    video_profile_params(session_profile, &session_video_size, &session_video_quality,
                         &session_video_fps, &session_video_name);

    ESP_LOGI(TAG,
             "SESSION_START WebRTC=PCMA/8000/mono video=JPEG_DC profile=%s size=%s quality=%d fps=%d I2S=PHILIPS/16000/32x2 ip=%s",
             session_video_name, jr_camera_size_name(session_video_size),
             session_video_quality, session_video_fps, jr_wifi_ip());

    jr_wa_status_t mode_status;
    portENTER_CRITICAL(&state_lock);
    mode_status = status_state;
    portEXIT_CRITICAL(&state_lock);
    ESP_LOGI(TAG,
             "JR_MODE live_start result=ok autonomous_before=%d autonomous_suspended=%d restore_pending=%d",
             mode_status.autonomous_before_live ? 1 : 0,
             mode_status.autonomous_suspended ? 1 : 0,
             mode_status.autonomous_restore_pending ? 1 : 0);
    return ESP_OK;

fail:
    ESP_LOGE(TAG, "JR_MODE live_start result=error stage=session_setup error=%s",
             esp_err_to_name(err));
    stop_session_unlocked();
    return err;
}

static esp_err_t start_session(void)
{
    if (!session_mutex ||
        xSemaphoreTake(session_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = start_session_unlocked();
    xSemaphoreGive(session_mutex);
    return err;
}

static void stop_session(void)
{
    if (!session_mutex ||
        xSemaphoreTake(session_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        return;
    }
    stop_session_unlocked();
    xSemaphoreGive(session_mutex);
}

static int send_remote_sdp(const char *sdp)
{
    if (!peer || !sdp) return -1;
    esp_peer_msg_t msg = {
        .type = ESP_PEER_MSG_TYPE_SDP,
        .data = (uint8_t *)sdp,
        .size = (int)strlen(sdp),
    };
    int ret = esp_peer_send_msg(peer, &msg);
    ESP_LOGI(TAG, "SIGNAL_REMOTE answer ret=%d bytes=%d", ret, msg.size);
    return ret;
}

static int send_remote_candidate(const char *candidate)
{
    if (!peer || !candidate || !candidate[0]) return -1;

    ESP_LOGI(TAG, "REMOTE_CANDIDATE raw=\"%s\"", candidate);

    esp_peer_msg_t msg = {
        .type = ESP_PEER_MSG_TYPE_CANDIDATE,
        .data = (uint8_t *)candidate,
        .size = (int)strlen(candidate),
    };
    int ret = esp_peer_send_msg(peer, &msg);
    ESP_LOGI(TAG, "SIGNAL_REMOTE candidate ret=%d bytes=%d", ret, msg.size);
    return ret;
}

static int send_sse(httpd_req_t *req, const char *data)
{
    size_t len = strlen(data) + 9;
    char *buf = malloc(len);
    if (!buf) return ESP_ERR_NO_MEM;
    int n = snprintf(buf, len, "data: %s\n\n", data);
    esp_err_t ret = httpd_resp_send_chunk(req, buf, n);
    free(buf);
    return ret;
}

static void signal_sender_task(void *arg)
{
    (void)arg;
    uint32_t last_heartbeat = (uint32_t)(esp_timer_get_time() / 1000ULL);

    while (!event_stream_stopping && event_stream_req) {
        char *msg = NULL;
        if (xQueueReceive(signal_queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (msg) {
                int ret = send_sse(event_stream_req, msg);
                free(msg);
                if (ret != ESP_OK) break;
            }
        }

        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        if ((uint32_t)(now - last_heartbeat) >= 5000U) {
            last_heartbeat = now;
            jr_wa_status_t s;
            portENTER_CRITICAL(&state_lock);
            s = status_state;
            portEXIT_CRITICAL(&state_lock);
            char heartbeat[384];
            snprintf(heartbeat, sizeof(heartbeat),
                     "{\"type\":\"heartbeat\",\"heap_internal_free\":%u,"
                     "\"heap_internal_largest\":%u,\"heap_internal_min\":%u,"
                     "\"heap_psram_free\":%u,\"heap_psram_largest\":%u,"
                     "\"heap_psram_min\":%u,\"video_frames\":%lu,\"video_drops\":%lu,"
                     "\"video_dc\":%d}",
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                     (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                     (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                     (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
                     (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM),
                     (unsigned long)s.video_frames,
                     (unsigned long)s.video_drops,
                     s.video_dc_open ? 1 : 0);
            if (send_sse(event_stream_req, heartbeat) != ESP_OK) {
                break;
            }
        }
    }

    if (event_stream_req) {
        httpd_req_async_handler_complete(event_stream_req);
        event_stream_req = NULL;
    }
    event_stream_connected = false;
    event_stream_stopping = false;
    ESP_LOGI(TAG, "SSE closed");
    vTaskDelete(NULL);
}

static esp_err_t page_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, WEBRTC_AUDIO_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t signal_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    if (event_stream_connected) {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(req, "WebRTC audio lab already has a signaling client");
    }

    event_stream_connected = true;
    event_stream_stopping = false;

    esp_err_t err = send_sse(req, "{\"type\":\"connected\"}");
    if (err != ESP_OK) {
        event_stream_connected = false;
        return err;
    }

    err = httpd_req_async_handler_begin(req, &event_stream_req);
    if (err != ESP_OK || !event_stream_req) {
        event_stream_connected = false;
        return err;
    }

    if (xTaskCreate(signal_sender_task, "jr_wa_signal", 6144, NULL, 6, NULL) != pdPASS) {
        httpd_req_async_handler_complete(event_stream_req);
        event_stream_req = NULL;
        event_stream_connected = false;
        return ESP_ERR_NO_MEM;
    }

    err = start_session();
    if (err != ESP_OK) {
        queue_signal_object("error", (const uint8_t *)"session_start_failed", 20);
        ESP_LOGE(TAG, "SESSION_START failed: %s", esp_err_to_name(err));
    }
    return ESP_OK;
}

static esp_err_t signal_post_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len > JR_WA_SIGNAL_POST_MAX) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid content length");
    }

    char *buf = malloc((size_t)req->content_len + 1);
    if (!buf) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no memory");

    int got = 0;
    while (got < req->content_len) {
        int n = httpd_req_recv(req, buf + got, req->content_len - got);
        if (n == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (n <= 0) {
            free(buf);
            return ESP_FAIL;
        }
        got += n;
    }
    buf[got] = 0;

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");

    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (!cJSON_IsString(type)) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing type");
    }

    int ret = 0;
    if (strcmp(type->valuestring, "answer") == 0) {
        cJSON *sdp = cJSON_GetObjectItem(root, "sdp");
        if (!cJSON_IsString(sdp)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing sdp");
        }
        ret = send_remote_sdp(sdp->valuestring);
    } else if (strcmp(type->valuestring, "candidate") == 0) {
        cJSON *cand = cJSON_GetObjectItem(root, "candidate");
        if (!cJSON_IsString(cand)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing candidate");
        }
        ret = send_remote_candidate(cand->valuestring);
    } else if (strcmp(type->valuestring, "video_profile") == 0) {
        cJSON *profile = cJSON_GetObjectItem(root, "profile");
        if (!cJSON_IsString(profile)) {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing profile");
        }

        jr_wa_video_profile_t next_profile;
        if (strcmp(profile->valuestring, "fast") == 0) {
            next_profile = JR_WA_VIDEO_PROFILE_FAST;
        } else if (strcmp(profile->valuestring, "balanced") == 0) {
            next_profile = JR_WA_VIDEO_PROFILE_BALANCED;
        } else if (strcmp(profile->valuestring, "quality") == 0) {
            next_profile = JR_WA_VIDEO_PROFILE_QUALITY;
        } else {
            cJSON_Delete(root);
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid video profile");
        }

        portENTER_CRITICAL(&state_lock);
        video_profile = next_profile;
        portEXIT_CRITICAL(&state_lock);

        jr_camera_size_t size;
        int quality = 14;
        int fps = 10;
        const char *name = "balanced";
        video_profile_params(next_profile, &size, &quality, &fps, &name);
        ESP_LOGI(TAG, "VIDEO_PROFILE profile=%s size=%s quality=%d fps=%d",
                 name, jr_camera_size_name(size), quality, fps);
        ret = 0;
    } else if (strcmp(type->valuestring, "bye") == 0) {
        event_stream_stopping = true;
        stop_session();
        ret = 0;
    } else {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "unknown type");
    }

    cJSON_Delete(root);
    if (ret != 0) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "peer signaling failed");
    }
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t status_handler(httpd_req_t *req)
{
    jr_wa_status_t s;
    portENTER_CRITICAL(&state_lock);
    s = status_state;
    portEXIT_CRITICAL(&state_lock);

    char text[1280];
    snprintf(text, sizeof(text),
             "JR_WEBRTC_AUDIO_TEST active=%d peer_connected=%d peer_state=%d sessions=%lu "
             "autonomous_before=%d autonomous_suspended=%d autonomous_restore_pending=%d autonomous_restored=%d "
             "autonomous_suspend_error=%s autonomous_restore_error=%s "
             "codec=PCMA rate=8000 channel=1 "
             "i2s_rate=16000 i2s_format=PHILIPS i2s_slots=2x32 "
             "bclk=%d ws=%d mic_sd=%d amp_din=%d "
             "mic_frames=%llu mic_errors=%lu "
             "tx_packets=%lu tx_bytes=%lu tx_errors=%lu "
             "rx_packets=%lu rx_bytes=%lu rx_drops=%lu "
             "speaker_frames=%llu speaker_errors=%lu "
             "video_dc=%d video_stream_id=%u video_frames=%lu video_bytes=%lu video_drops=%lu video_errors=%lu "
             "heap_internal_free=%u heap_internal_largest=%u heap_internal_min=%u "
             "heap_psram_free=%u heap_psram_largest=%u heap_psram_min=%u",
             s.active ? 1 : 0,
             s.peer_connected ? 1 : 0,
             s.peer_state,
             (unsigned long)s.sessions,
             s.autonomous_before_live ? 1 : 0,
             s.autonomous_suspended ? 1 : 0,
             s.autonomous_restore_pending ? 1 : 0,
             s.autonomous_restored ? 1 : 0,
             esp_err_to_name(s.autonomous_suspend_error),
             esp_err_to_name(s.autonomous_restore_error),
             JR_AUDIO_BCLK_GPIO, JR_AUDIO_LRC_GPIO, JR_MIC_SD_GPIO, JR_AUDIO_DIN_GPIO,
             (unsigned long long)s.mic_i2s_frames,
             (unsigned long)s.mic_i2s_errors,
             (unsigned long)s.webrtc_tx_packets,
             (unsigned long)s.webrtc_tx_bytes,
             (unsigned long)s.webrtc_tx_errors,
             (unsigned long)s.webrtc_rx_packets,
             (unsigned long)s.webrtc_rx_bytes,
             (unsigned long)s.webrtc_rx_drops,
             (unsigned long long)s.speaker_i2s_frames,
             (unsigned long)s.speaker_i2s_errors,
             s.video_dc_open ? 1 : 0,
             (unsigned)s.video_dc_stream_id,
             (unsigned long)s.video_frames,
             (unsigned long)s.video_bytes,
             (unsigned long)s.video_drops,
             (unsigned long)s.video_send_errors,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));

    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, text);
}

esp_err_t jr_webrtc_audio_register_routes(httpd_handle_t server)
{
    if (!server) return ESP_ERR_INVALID_ARG;

    if (!session_mutex) session_mutex = xSemaphoreCreateMutex();
    if (!signal_queue) signal_queue = xQueueCreate(JR_WA_SIGNAL_QUEUE_DEPTH, sizeof(char *));
    if (!audio_queue) audio_queue = xQueueCreate(JR_WA_AUDIO_QUEUE_DEPTH, sizeof(jr_wa_audio_packet_t));
    if (!session_mutex || !signal_queue || !audio_queue) return ESP_ERR_NO_MEM;

    const httpd_uri_t routes[] = {
        {.uri = "/webrtc-audio", .method = HTTP_GET, .handler = page_handler, .user_ctx = NULL},
        {.uri = "/webrtc-audio/signal", .method = HTTP_GET, .handler = signal_get_handler, .user_ctx = NULL},
        {.uri = "/webrtc-audio/signal", .method = HTTP_POST, .handler = signal_post_handler, .user_ctx = NULL},
        {.uri = "/webrtc-audio/status", .method = HTTP_GET, .handler = status_handler, .user_ctx = NULL},
    };

    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); ++i) {
        esp_err_t err = httpd_register_uri_handler(server, &routes[i]);
        if (err != ESP_OK) return err;
    }

    ESP_LOGI(TAG,
             "WEBRTC_JPEG_READY url=https://%s/webrtc-audio audio=PCMA video=JPEG_DC",
             jr_wifi_ip());
    return ESP_OK;
}
