// JrBot HW04 pin mapping for the separate Ivy AI evaluation.
// Does not modify JrBot production firmware.
// SPDX-License-Identifier: MIT
#pragma once

#define IVY_HW04_I2S_BCLK 21
#define IVY_HW04_I2S_LRCK 47
#define IVY_HW04_I2S_DIN  41
#define IVY_HW04_I2S_DOUT 42
#define IVY_HW04_I2S_MCLK (-1)
#define IVY_HW04_AUDIO_RATE 16000

// Ivy's original ES8311 codec control is NOT available on HW04.
// I2S-only direct digital microphone + MAX98357A amplifier.
// Do not configure the camera/SSD1306/WebRTC in the Ivy audio-only test.
#define IVY_HW04_ES8311_PRESENT 0
#define IVY_HW04_AUDIO_ONLY 1
