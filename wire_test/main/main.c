#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define WIRE_UART UART_NUM_0
#define WIRE_BAUD 115200
#define WIRE_LINE_MAX 96

static const int TEST_PINS[] = {1, 2, 41, 42, 47, 21};
static SemaphoreHandle_t pin_lock;
static int active_pin = -1;

typedef void (*wire_write_fn)(const char *text);

typedef struct {
    char line[WIRE_LINE_MAX + 1];
    size_t size;
    bool discard;
} wire_parser_t;

static bool pin_allowed(int pin) {
    for (size_t i = 0; i < sizeof(TEST_PINS) / sizeof(TEST_PINS[0]); ++i) {
        if (TEST_PINS[i] == pin) return true;
    }
    return false;
}

/* OFF seguro: ESP32 nao dirige nivel alto nem baixo e nao habilita pull interno. */
static void release_all_pins_locked(void) {
    for (size_t i = 0; i < sizeof(TEST_PINS) / sizeof(TEST_PINS[0]); ++i) {
        gpio_num_t pin = (gpio_num_t)TEST_PINS[i];
        gpio_reset_pin(pin);
        gpio_set_pull_mode(pin, GPIO_FLOATING);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
    }
    active_pin = -1;
}

static esp_err_t activate_pin_locked(int pin) {
    if (!pin_allowed(pin)) return ESP_ERR_INVALID_ARG;

    release_all_pins_locked();

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) return err;

    err = gpio_set_level((gpio_num_t)pin, 1);
    if (err == ESP_OK) active_pin = pin;
    return err;
}

static void uart_write_line(const char *text) {
    if (!text) return;
    uart_write_bytes(WIRE_UART, text, strlen(text));
    uart_write_bytes(WIRE_UART, "\r\n", 2);
    uart_wait_tx_done(WIRE_UART, pdMS_TO_TICKS(300));
}

static void usb_write_line(const char *text) {
    if (!text || !usb_serial_jtag_is_driver_installed()) return;
    size_t len = strlen(text);
    size_t sent = 0;
    while (sent < len) {
        int n = usb_serial_jtag_write_bytes(text + sent, len - sent, pdMS_TO_TICKS(200));
        if (n <= 0) break;
        sent += (size_t)n;
    }
    (void)usb_serial_jtag_write_bytes("\r\n", 2, pdMS_TO_TICKS(200));
    (void)usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(300));
}

static void process_command(const char *line, wire_write_fn write_line) {
    char cmd[WIRE_LINE_MAX + 1];
    snprintf(cmd, sizeof(cmd), "%s", line ? line : "");

    char *p = cmd;
    while (*p && isspace((unsigned char)*p)) ++p;
    char *end = p + strlen(p);
    while (end > p && isspace((unsigned char)end[-1])) *--end = '\0';

    for (char *q = p; *q; ++q) *q = (char)tolower((unsigned char)*q);

    if (!strcmp(p, "status")) {
        char response[160];
        int current;
        xSemaphoreTake(pin_lock, portMAX_DELAY);
        current = active_pin;
        xSemaphoreGive(pin_lock);
        snprintf(response, sizeof(response),
                 "WIRE_STATUS ready=1 active_pin=%d idle_mode=high_z pins=1,2,41,42,47,21", current);
        write_line(response);
        return;
    }

    if (!strcmp(p, "off")) {
        xSemaphoreTake(pin_lock, portMAX_DELAY);
        release_all_pins_locked();
        xSemaphoreGive(pin_lock);
        write_line("WIRE_OK off=1 active_pin=-1 idle_mode=high_z");
        return;
    }

    if (!strncmp(p, "pin ", 4)) {
        char *num_end = NULL;
        long value = strtol(p + 4, &num_end, 10);
        while (num_end && *num_end && isspace((unsigned char)*num_end)) ++num_end;
        if (!num_end || *num_end || !pin_allowed((int)value)) {
            write_line("WIRE_ERROR invalid_pin allowed=1,2,41,42,47,21");
            return;
        }

        xSemaphoreTake(pin_lock, portMAX_DELAY);
        esp_err_t err = activate_pin_locked((int)value);
        xSemaphoreGive(pin_lock);

        if (err == ESP_OK) {
            char response[128];
            snprintf(response, sizeof(response),
                     "WIRE_OK pin=%ld voltage=3.3V mode=open_drain_pullup measure_to=GND", value);
            write_line(response);
        } else {
            char response[128];
            snprintf(response, sizeof(response), "WIRE_ERROR pin=%ld err=%s", value, esp_err_to_name(err));
            write_line(response);
        }
        return;
    }

    if (!strcmp(p, "help")) {
        write_line("WIRE_HELP status | pin 1|2|41|42|47|21 | off");
        return;
    }

    write_line("WIRE_ERROR unknown_command");
}

static void feed_parser(wire_parser_t *parser, const uint8_t *data, size_t len, wire_write_fn write_line) {
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = data[i];
        if (c == '\r' || c == '\n') {
            if (!parser->discard && parser->size) {
                parser->line[parser->size] = '\0';
                process_command(parser->line, write_line);
            }
            parser->size = 0;
            parser->discard = false;
        } else if (!parser->discard) {
            if (c < 32 || c == 127 || parser->size >= WIRE_LINE_MAX) {
                parser->discard = true;
            } else {
                parser->line[parser->size++] = (char)c;
            }
        }
    }
}

static void uart_task(void *arg) {
    (void)arg;
    uint8_t rx[128];
    wire_parser_t parser = {0};
    uart_write_line("WIRE_TEST_READY transport=UART pins=1,2,41,42,47,21 idle_mode=high_z");
    for (;;) {
        int n = uart_read_bytes(WIRE_UART, rx, sizeof(rx), pdMS_TO_TICKS(100));
        if (n > 0) feed_parser(&parser, rx, (size_t)n, uart_write_line);
    }
}

static void usb_task(void *arg) {
    (void)arg;
    uint8_t rx[128];
    wire_parser_t parser = {0};
    usb_write_line("WIRE_TEST_READY transport=USB_SERIAL_JTAG pins=1,2,41,42,47,21 idle_mode=high_z");
    for (;;) {
        int n = usb_serial_jtag_read_bytes(rx, sizeof(rx), pdMS_TO_TICKS(100));
        if (n > 0) feed_parser(&parser, rx, (size_t)n, usb_write_line);
    }
}

void app_main(void) {
    pin_lock = xSemaphoreCreateMutex();
    if (!pin_lock) return;

    xSemaphoreTake(pin_lock, portMAX_DELAY);
    release_all_pins_locked();
    xSemaphoreGive(pin_lock);

    esp_err_t uart_err = uart_driver_install(WIRE_UART, 2048, 0, 0, NULL, 0);
    if (uart_err != ESP_OK && uart_err != ESP_ERR_INVALID_STATE) return;
    (void)uart_set_baudrate(WIRE_UART, WIRE_BAUD);

    if (!usb_serial_jtag_is_driver_installed()) {
        usb_serial_jtag_driver_config_t usb_cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
        usb_cfg.rx_buffer_size = 2048;
        usb_cfg.tx_buffer_size = 2048;
        if (usb_serial_jtag_driver_install(&usb_cfg) != ESP_OK) return;
    }

    if (xTaskCreate(uart_task, "wire_uart", 4096, NULL, 5, NULL) != pdPASS) return;
    if (xTaskCreate(usb_task, "wire_usb", 4096, NULL, 5, NULL) != pdPASS) return;

    for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}
