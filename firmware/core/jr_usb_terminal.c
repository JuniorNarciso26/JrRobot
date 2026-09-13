#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "jr_brain.h"
#include "jr_commands.h"
#include "jr_usb_terminal.h"

#define JR_USB_EVENT_QUEUE_LENGTH 24
#define JR_USB_EVENT_MAX_BYTES 256

static const char *TAG = "jrbot_usb_terminal";
static bool started;
static QueueHandle_t event_queue;
static TaskHandle_t event_task_handle;
static TaskHandle_t terminal_task_handle;
static uint32_t event_dropped;

typedef struct {
    char line[JR_SERIAL_LINE_MAX_BYTES + 1];
    size_t size;
    bool discard;
} usb_parser_t;

typedef struct {
    char text[JR_USB_EVENT_MAX_BYTES];
} usb_event_t;

static void usb_write_all(const char *text) {
    if (!text || !usb_serial_jtag_is_driver_installed()) return;
    size_t len = strlen(text);
    size_t sent = 0;
    while (sent < len) {
        int n = usb_serial_jtag_write_bytes(text + sent, len - sent, pdMS_TO_TICKS(200));
        if (n <= 0) break;
        sent += (size_t)n;
    }
    (void)usb_serial_jtag_write_bytes("\n", 1, pdMS_TO_TICKS(200));
    (void)usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(300));
}

void jr_usb_terminal_emit(const char *text) {
    /*
     * Eventos do Brain sao telemetria: a tarefa de voz apenas enfileira.
     * Uma tarefa separada pode aguardar o USB sem aumentar a latencia da resposta.
     */
    if (!text || !event_queue) return;
    usb_event_t event = {0};
    snprintf(event.text, sizeof(event.text), "%s", text);
    if (xQueueSend(event_queue, &event, 0) != pdTRUE) event_dropped++;
}

static void usb_event_task(void *arg) {
    (void)arg;
    usb_event_t event;
    for (;;) {
        if (xQueueReceive(event_queue, &event, portMAX_DELAY) != pdTRUE) continue;
        usb_write_all(event.text);
        if (event_dropped) {
            uint32_t dropped = event_dropped;
            event_dropped = 0;
            char warning[96];
            snprintf(warning, sizeof(warning), "JR_WARN usb_event_queue dropped=%lu", (unsigned long)dropped);
            usb_write_all(warning);
        }
    }
}

static bool brain_command(const char *cmd, char *response, size_t cap) {
    if (!strcmp(cmd, "autonomo_on")) {
        esp_err_t err = jr_brain_set_enabled(true);
        snprintf(response, cap, err == ESP_OK ? "JR_OK autonomous=1 brain=listening wakeword=JrBot engine=bench_stub" : "JR_ERROR autonomous_on=%s", esp_err_to_name(err));
        return err == ESP_OK;
    }
    if (!strcmp(cmd, "autonomo_off")) {
        esp_err_t err = jr_brain_set_enabled(false);
        snprintf(response, cap, err == ESP_OK ? "JR_OK autonomous=0 brain=off" : "JR_ERROR autonomous_off=%s", esp_err_to_name(err));
        return err == ESP_OK;
    }
    if (!strcmp(cmd, "brain_test")) {
        esp_err_t err = jr_brain_trigger_test();
        snprintf(response, cap, err == ESP_OK ? "JR_OK brain_test=wakeword_simulated keyword=JrBot expression=feliz" : "JR_ERROR brain_test=%s", esp_err_to_name(err));
        return err == ESP_OK;
    }
    if (!strcmp(cmd, "brain_status")) {
        jr_brain_status_t s;
        jr_brain_get_status(&s);
        snprintf(response, cap,
                 "JR_OK autonomous=%d brain=%s listening=%d engine=%s triggers=%lu last_event=%s last_error=%s",
                 s.enabled ? 1 : 0, jr_brain_state_name(s.state), s.listening ? 1 : 0, s.engine,
                 (unsigned long)s.triggers, s.last_event, esp_err_to_name(s.last_error));
        return true;
    }
    return false;
}

static void usb_process_line(const char *line) {
    char id[33] = "";
    const char *cmd = line;

    if (*cmd == '@') {
        const char *space = strchr(cmd, ' ');
        if (!space || space - cmd < 2 || space - cmd > 33) {
            usb_write_all("JR_ERROR invalid_id");
            return;
        }
        size_t n = (size_t)(space - cmd - 1);
        for (size_t i = 0; i < n; i++) {
            unsigned char c = (unsigned char)cmd[i + 1];
            if (!isalnum(c) && c != '-' && c != '_') {
                usb_write_all("JR_ERROR invalid_id");
                return;
            }
        }
        memcpy(id, cmd + 1, n);
        id[n] = '\0';
        cmd = space + 1;
    }

    char response[JR_RESPONSE_MAX_BYTES];
    bool ok;
    if (!strcmp(cmd, "autonomo_on") || !strcmp(cmd, "autonomo_off") || !strcmp(cmd, "brain_test") || !strcmp(cmd, "brain_status")) {
        ok = brain_command(cmd, response, sizeof(response));
    } else {
        ok = jr_handle_command(cmd, response, sizeof(response));
    }

    if (*id) {
        char wire[JR_RESPONSE_MAX_BYTES + 64];
        int n = snprintf(wire, sizeof(wire), "JR_REPLY id=%s ok=%d %s", id, ok ? 1 : 0, response);
        if (n > 0 && (size_t)n < sizeof(wire)) usb_write_all(wire);
        else usb_write_all("JR_ERROR response_too_long");
    } else {
        usb_write_all(response);
    }
}

static void usb_feed(usb_parser_t *parser, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        unsigned char c = data[i];
        if (c == '\r' || c == '\n') {
            if (!parser->discard && parser->size) {
                parser->line[parser->size] = '\0';
                unsigned char first = (unsigned char)parser->line[0];
                if (first == '@' || isalpha(first)) usb_process_line(parser->line);
            }
            parser->size = 0;
            parser->discard = false;
        } else if (!parser->discard) {
            if (c < 32 || c == 127) {
                if (parser->size) parser->discard = true;
            } else if (parser->size >= JR_SERIAL_LINE_MAX_BYTES) {
                parser->discard = true;
            } else {
                parser->line[parser->size++] = (char)c;
            }
        }
    }
}

static void usb_terminal_task(void *arg) {
    (void)arg;
    uint8_t rx[256];
    usb_parser_t parser = {0};

    for (;;) {
        int n = usb_serial_jtag_read_bytes(rx, sizeof(rx), pdMS_TO_TICKS(100));
        if (n > 0) usb_feed(&parser, rx, (size_t)n);
    }
}

void jr_usb_terminal_start(void) {
    if (started) return;

    if (!usb_serial_jtag_is_driver_installed()) {
        usb_serial_jtag_driver_config_t config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
        config.rx_buffer_size = 2048;
        config.tx_buffer_size = 4096;
        esp_err_t err = usb_serial_jtag_driver_install(&config);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "USB Serial/JTAG input indisponivel: %s", esp_err_to_name(err));
            return;
        }
    }

    event_queue = xQueueCreate(JR_USB_EVENT_QUEUE_LENGTH, sizeof(usb_event_t));
    if (!event_queue) {
        ESP_LOGW(TAG, "Fila de telemetria USB indisponivel");
        return;
    }
    if (xTaskCreate(usb_event_task, "usb_event_tx", 4096, NULL, 3, &event_task_handle) != pdPASS) {
        vQueueDelete(event_queue);
        event_queue = NULL;
        ESP_LOGW(TAG, "Falha criando tarefa de telemetria USB");
        return;
    }
    if (xTaskCreate(usb_terminal_task, "usb_terminal", 6144, NULL, 5, &terminal_task_handle) != pdPASS) {
        vTaskDelete(event_task_handle);
        event_task_handle = NULL;
        vQueueDelete(event_queue);
        event_queue = NULL;
        ESP_LOGW(TAG, "Falha criando tarefa USB Serial/JTAG");
        return;
    }

    started = true;
    ESP_LOGI(TAG, "USB Serial/JTAG pronto para comandos bidirecionais + telemetria enfileirada");
}
