#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// MVP JrBot Face OLED.
// Pinos provisórios do primeiro teste. Confirmar na placa ESP32-S3-CAM física.
#define OLED_SDA GPIO_NUM_8
#define OLED_SCL GPIO_NUM_9
#define OLED_ADDRESS_PRIMARY 0x3C
#define OLED_ADDRESS_SECONDARY 0x3D
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES 8

static const char *TAG = "jrbot_face";
static uint8_t frame[OLED_WIDTH * OLED_PAGES];
static uint8_t oled_address = OLED_ADDRESS_PRIMARY;
static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t oled_device;

typedef enum {
    FACE_NEUTRAL = 0,
    FACE_HAPPY,
    FACE_SAD,
    FACE_ANGRY,
    FACE_SLEEPY,
    FACE_LEFT,
    FACE_RIGHT,
    FACE_SURPRISED,
} face_expression_t;

static volatile face_expression_t current_expression = FACE_NEUTRAL;
static volatile uint32_t command_count = 0;
static volatile uint32_t frame_count = 0;

static const char *expression_name(face_expression_t expression) {
    switch (expression) {
        case FACE_HAPPY: return "happy";
        case FACE_SAD: return "sad";
        case FACE_ANGRY: return "angry";
        case FACE_SLEEPY: return "sleepy";
        case FACE_LEFT: return "left";
        case FACE_RIGHT: return "right";
        case FACE_SURPRISED: return "surprised";
        case FACE_NEUTRAL:
        default: return "neutral";
    }
}

static bool oled_send(uint8_t control, const uint8_t *bytes, size_t length) {
    uint8_t transfer[129];
    if (length > sizeof(transfer) - 1) return false;
    transfer[0] = control;
    memcpy(&transfer[1], bytes, length);
    return i2c_master_transmit(oled_device, transfer, length + 1, 1000) == ESP_OK;
}

static bool oled_command(uint8_t command) { return oled_send(0x00, &command, 1); }

static bool oled_select_address(void) {
    const uint8_t candidates[] = {OLED_ADDRESS_PRIMARY, OLED_ADDRESS_SECONDARY};
    for (size_t i = 0; i < sizeof(candidates); ++i) {
        if (i2c_master_probe(i2c_bus, candidates[i], 100) == ESP_OK) {
            oled_address = candidates[i];
            i2c_device_config_t config = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = oled_address,
                .scl_speed_hz = 400000,
            };
            return i2c_master_bus_add_device(i2c_bus, &config, &oled_device) == ESP_OK;
        }
    }
    return false;
}

static bool oled_init(void) {
    const uint8_t init[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF,
    };
    return oled_select_address() && oled_send(0x00, init, sizeof(init));
}

static void set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    uint8_t *cell = &frame[x + (y / 8) * OLED_WIDTH];
    uint8_t mask = 1U << (y % 8);
    if (on) *cell |= mask;
    else *cell &= (uint8_t)~mask;
}

static void fill_round_rect(int x, int y, int width, int height, int radius, bool on) {
    for (int py = y; py < y + height; ++py) {
        for (int px = x; px < x + width; ++px) {
            int left = px - x;
            int right = x + width - 1 - px;
            int top = py - y;
            int bottom = y + height - 1 - py;
            int dx = left < radius ? radius - left : (right < radius ? radius - right : 0);
            int dy = top < radius ? radius - top : (bottom < radius ? radius - bottom : 0);
            if (dx * dx + dy * dy <= radius * radius) set_pixel(px, py, on);
        }
    }
}

static void draw_line(int x0, int y0, int x1, int y1, bool on) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = -(y1 > y0 ? y1 - y0 : y0 - y1);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        set_pixel(x0, y0, on);
        set_pixel(x0, y0 + 1, on);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void draw_eye(int center_x, int center_y, face_expression_t expression, bool blink) {
    if (blink || expression == FACE_SLEEPY) {
        fill_round_rect(center_x - 22, center_y - 2, 44, 4, 2, true);
        return;
    }

    int eye_w = 38;
    int eye_h = 30;
    int pupil_dx = 0;
    int pupil_dy = 0;

    if (expression == FACE_LEFT) pupil_dx = -8;
    if (expression == FACE_RIGHT) pupil_dx = 8;
    if (expression == FACE_HAPPY) pupil_dy = -2;
    if (expression == FACE_SAD) pupil_dy = 4;

    if (expression == FACE_SURPRISED) {
        fill_round_rect(center_x - 17, center_y - 17, 34, 34, 16, true);
        fill_round_rect(center_x - 7, center_y - 7, 14, 14, 6, false);
        return;
    }

    fill_round_rect(center_x - eye_w / 2, center_y - eye_h / 2, eye_w, eye_h, 10, true);
    fill_round_rect(center_x - 6 + pupil_dx, center_y - 8 + pupil_dy, 12, 16, 5, false);

    if (expression == FACE_HAPPY) {
        draw_line(center_x - 16, center_y - 21, center_x + 16, center_y - 18, true);
    } else if (expression == FACE_SAD) {
        draw_line(center_x - 16, center_y - 19, center_x + 16, center_y - 23, true);
    } else if (expression == FACE_ANGRY) {
        // Inclina as sobrancelhas para dentro.
        if (center_x < 64) draw_line(center_x - 16, center_y - 24, center_x + 15, center_y - 17, true);
        else draw_line(center_x - 16, center_y - 17, center_x + 15, center_y - 24, true);
    }
}

static bool oled_show(void) {
    for (uint8_t page = 0; page < OLED_PAGES; ++page) {
        if (!oled_command(0xB0 | page) || !oled_command(0x00) || !oled_command(0x10) ||
            !oled_send(0x40, &frame[page * OLED_WIDTH], OLED_WIDTH)) return false;
    }
    return true;
}

static void draw_face(face_expression_t expression, bool blink) {
    memset(frame, 0, sizeof(frame));
    draw_eye(35, 33, expression, blink);
    draw_eye(93, 33, expression, blink);
    oled_show();
}

static bool set_expression_from_command(const char *command) {
    if (strcmp(command, "neutro") == 0 || strcmp(command, "neutral") == 0) current_expression = FACE_NEUTRAL;
    else if (strcmp(command, "feliz") == 0 || strcmp(command, "happy") == 0) current_expression = FACE_HAPPY;
    else if (strcmp(command, "triste") == 0 || strcmp(command, "sad") == 0) current_expression = FACE_SAD;
    else if (strcmp(command, "bravo") == 0 || strcmp(command, "angry") == 0) current_expression = FACE_ANGRY;
    else if (strcmp(command, "sono") == 0 || strcmp(command, "sleepy") == 0) current_expression = FACE_SLEEPY;
    else if (strcmp(command, "esquerda") == 0 || strcmp(command, "left") == 0) current_expression = FACE_LEFT;
    else if (strcmp(command, "direita") == 0 || strcmp(command, "right") == 0) current_expression = FACE_RIGHT;
    else if (strcmp(command, "surpreso") == 0 || strcmp(command, "surprised") == 0) current_expression = FACE_SURPRISED;
    else return false;
    return true;
}

static void print_help(void) {
    printf("\nJrBot Face OLED pronto. Comandos:\n");
    printf("  neutro | feliz | triste | bravo | sono | esquerda | direita | surpreso\n");
    printf("  status | help\n\n");
}

static void terminal_task(void *parameter) {
    uint8_t received[64];
    char command[32] = {0};
    size_t command_length = 0;

    print_help();
    while (true) {
        int count = uart_read_bytes(UART_NUM_0, received, sizeof(received), pdMS_TO_TICKS(100));
        for (int i = 0; i < count; ++i) {
            char letter = (char)received[i];
            if (letter == '\r' || letter == '\n') {
                if (command_length == 0) continue;
                command[command_length] = '\0';
                if (strcmp(command, "status") == 0) {
                    printf("JR_STATUS expression=%s oled=0x%02X sda=%d scl=%d commands=%lu frames=%lu\n", expression_name(current_expression), oled_address, OLED_SDA, OLED_SCL, (unsigned long)command_count, (unsigned long)frame_count);
                } else if (strcmp(command, "help") == 0 || strcmp(command, "ajuda") == 0) {
                    print_help();
                } else if (set_expression_from_command(command)) {
                    command_count++;
                    printf("JR_OK command=%lu expression=%s\n", (unsigned long)command_count, expression_name(current_expression));
                    ESP_LOGI(TAG, "Expressao alterada: %s command=%lu", expression_name(current_expression), (unsigned long)command_count);
                } else {
                    printf("JR_ERROR comando_desconhecido=%s\n", command);
                    ESP_LOGW(TAG, "Comando desconhecido: %s", command);
                }
                command_length = 0;
            } else if (command_length < sizeof(command) - 1) {
                command[command_length++] = (char)tolower((unsigned char)letter);
            }
        }
    }
}

void app_main(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = OLED_SDA,
        .scl_io_num = OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    if (!oled_init()) {
        ESP_LOGE(TAG, "OLED nao respondeu em I2C 0x3C nem 0x3D. Verifique SDA, SCL, VCC e GND.");
        return;
    }

    ESP_LOGI(TAG, "OLED detectado em I2C 0x%02X. Iniciando rosto JrBot.", oled_address);
    esp_err_t uart_result = uart_driver_install(UART_NUM_0, 2048, 0, 0, NULL, 0);
    if (uart_result != ESP_OK && uart_result != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(uart_result);
    xTaskCreate(terminal_task, "terminal", 4096, NULL, 5, NULL);

    while (true) {
        uint32_t elapsed_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        bool blinking = current_expression != FACE_SLEEPY && (elapsed_ms % 2400 < 140);
        draw_face(current_expression, blinking);
        frame_count++;
        if (frame_count % 25 == 0) {
            printf("JR_ALIVE expression=%s commands=%lu frames=%lu blink=%d\n", expression_name(current_expression), (unsigned long)command_count, (unsigned long)frame_count, blinking ? 1 : 0);
        }
        vTaskDelay(pdMS_TO_TICKS(120));
    }
}
