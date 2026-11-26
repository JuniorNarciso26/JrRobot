// Serial-only training data collector for the isolated micro_speech experiment.
#include "serial_capture.h"
#include <cstdio>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"

namespace {
constexpr size_t kSamples = 16000;
int16_t recording[kSamples];
enum State { Idle, Capturing, Ready };
State state = Idle;
size_t captured = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void reset() {
    portENTER_CRITICAL(&mux);
    state = Idle;
    captured = 0;
    portEXIT_CRITICAL(&mux);
}

bool ready() {
    portENTER_CRITICAL(&mux);
    const bool result = state == Ready;
    portEXIT_CRITICAL(&mux);
    return result;
}

void record(const char* id) {
    // The reader task receives fresh PCM from the same input used by inference.
    std::printf("JR_KWS_BEGIN id=%s rate=16000 samples=16000 bytes=32000\n", id);
    std::fflush(stdout);
    portENTER_CRITICAL(&mux);
    captured = 0;
    state = Capturing;
    portEXIT_CRITICAL(&mux);
    const int64_t deadline = esp_timer_get_time() + 4000000;
    while (!ready()) {
        if (esp_timer_get_time() >= deadline) {
            reset();
            std::printf("JR_KWS_ERROR id=%s reason=mic_timeout\n", id);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    std::printf("JR_KWS_CAPTURED id=%s\n", id);
    const auto* bytes = reinterpret_cast<const uint8_t*>(recording);
    uint32_t hash = 2166136261u;
    for (size_t offset = 0, sequence = 0; offset < sizeof(recording); offset += 256, ++sequence) {
        const size_t count = sizeof(recording) - offset < 256 ? sizeof(recording) - offset : 256;
        char hex[513];
        constexpr char digits[] = "0123456789abcdef";
        for (size_t j = 0; j < count; ++j) {
            const uint8_t value = bytes[offset + j];
            hex[j * 2] = digits[value >> 4];
            hex[j * 2 + 1] = digits[value & 15];
            hash = (hash ^ value) * 16777619u;
        }
        hex[count * 2] = 0;
        // A single stdio write per frame prevents partial lines mixing with logs.
        std::printf("JR_KWS_DATA id=%s seq=%u hex=%s\n", id, (unsigned)sequence, hex);
        std::fflush(stdout);
        vTaskDelay(1);
    }
    std::printf("JR_KWS_END id=%s fnv=%08lx\n", id, (unsigned long)hash);
    std::fflush(stdout);
    reset();
}

void command_task(void*) {
    char command[96] = {};
    size_t used = 0;
    bool discard = false;
    // Console UART polling: EOF means no byte currently available, not shutdown.
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    while (true) {
        const int ch = std::getchar();
        if (ch == EOF) {
            std::clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (ch == '\r') continue;
        if (ch != '\n') {
            if (used + 1 < sizeof(command) && !discard) command[used++] = static_cast<char>(ch);
            else discard = true;
            continue;
        }
        command[used] = 0;
        if (!discard && std::strcmp(command, "kws_info") == 0) {
            std::printf("JR_KWS_INFO v=1 rate=16000 samples=16000\n");
            std::fflush(stdout);
        } else if (!discard) {
            char id[33] = {}, extra = 0;
            if (std::sscanf(command, "kws_record %32[0-9a-f] %c", id, &extra) == 1 && std::strlen(id) == 16) {
                record(id);
            }
        }
        used = 0;
        discard = false;
    }
}
}  // namespace

bool JrCaptureActive() {
    portENTER_CRITICAL(&mux);
    const bool active = state != Idle;
    portEXIT_CRITICAL(&mux);
    return active;
}

void JrCaptureFeed(const int16_t* pcm, size_t samples) {
    portENTER_CRITICAL(&mux);
    if (state == Capturing) {
        const size_t count = samples < kSamples - captured ? samples : kSamples - captured;
        std::memcpy(recording + captured, pcm, count * sizeof(int16_t));
        captured += count;
        if (captured == kSamples) state = Ready;
    }
    portEXIT_CRITICAL(&mux);
}

void JrCaptureStart() {
    ESP_ERROR_CHECK(xTaskCreate(command_task, "kws_serial", 4096, nullptr, 5, nullptr) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}
