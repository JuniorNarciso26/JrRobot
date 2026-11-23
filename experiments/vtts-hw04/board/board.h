/* JrBot HW04 ESP32-S3 N16R8 board overlay for the vTTS isolated test.
 * SPDX-License-Identifier: MIT
 * Source: JrBot reference wiring (not copied from upstream board).
 */
#pragma once
#include "driver/i2s_types.h"
#define BSP_NAME "JrBot HW04 / MAX98357A"
#define BSP_DEFAULT_VOL 75
#define BSP_DEFAULT_GAIN_DB 0.0f
#define BSP_I2S_PORT I2S_NUM_0
#define BSP_I2S_MCLK I2S_GPIO_UNUSED
#define BSP_I2S_BCLK 21
#define BSP_I2S_WS 47
#define BSP_I2S_DOUT 42
#define BSP_DAC_UPSAMPLE 2
