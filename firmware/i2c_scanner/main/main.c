#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "jrbot_i2c_scan";

typedef struct {
    gpio_num_t sda;
    gpio_num_t scl;
    const char *name;
} i2c_pair_t;

static const i2c_pair_t pairs[] = {
    {GPIO_NUM_8, GPIO_NUM_9, "SDA=8 SCL=9 atual"},
    {GPIO_NUM_9, GPIO_NUM_8, "SDA=9 SCL=8 invertido"},
    // ESP32-S3 desta placa nao tem GPIO22 no header/SDK; por isso nao testamos 21/22.
    {GPIO_NUM_4, GPIO_NUM_5, "SDA=4 SCL=5"},
    {GPIO_NUM_6, GPIO_NUM_7, "SDA=6 SCL=7"},
};

static void scan_pair(i2c_pair_t pair) {
    i2c_master_bus_handle_t bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = pair.sda,
        .scl_io_num = pair.scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_config, &bus);
    if (err != ESP_OK) {
        printf("PAIR_FAIL %s err=%s\n", pair.name, esp_err_to_name(err));
        return;
    }

    printf("\nSCAN %s\n", pair.name);
    int found = 0;
    for (uint8_t addr = 0x03; addr <= 0x77; ++addr) {
        err = i2c_master_probe(bus, addr, 100);
        if (err == ESP_OK) {
            printf("FOUND address=0x%02X on %s\n", addr, pair.name);
            found++;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    if (!found) printf("NOT_FOUND %s\n", pair.name);
    i2c_del_master_bus(bus);
}

void app_main(void) {
    ESP_LOGI(TAG, "JrBot I2C Scanner iniciado");
    printf("\n=== JrBot I2C Scanner ===\n");
    printf("OLED esperado normalmente em 0x3C ou 0x3D.\n");
    printf("Se aparecer FOUND em algum par, esse par e o correto.\n");

    while (true) {
        for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]); ++i) {
            scan_pair(pairs[i]);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        printf("\n--- repetindo scan em 5 segundos ---\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
