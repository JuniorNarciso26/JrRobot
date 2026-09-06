#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jr_commands.h"
#include "jr_usb_terminal.h"

static const char *TAG = "jrbot_usb_terminal";
static bool started;

typedef struct {
    char line[JR_SERIAL_LINE_MAX_BYTES + 1];
    size_t size;
    bool discard;
} usb_parser_t;

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
    bool ok = jr_handle_command(cmd, response, sizeof(response));

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

    if (xTaskCreate(usb_terminal_task, "usb_terminal", 6144, NULL, 5, NULL) != pdPASS) {
        ESP_LOGW(TAG, "Falha criando tarefa USB Serial/JTAG");
        return;
    }

    started = true;
    ESP_LOGI(TAG, "USB Serial/JTAG pronto para comandos bidirecionais");
}
