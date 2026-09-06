#pragma once
#include "jr_pinmap_fixed.h"

#define JR_OLED_I2C_PORT 0
#define JR_OLED_I2C_HZ 100000

/* HW04 final hardware-test profile: all confirmed peripherals are compiled in. */
#define JR_OLED_ENABLED 1
#define JR_CAMERA_ENABLED 1
#define JR_AUDIO_ENABLED 1
#define JR_MIC_PINOUT_DEFINED 1
#define JR_MIC_ENABLED 1
#define JR_PROFILE_NAME "full_hardware_test"

_Static_assert(JR_AUDIO_BCLK_GPIO == 21, "HW04 BCLK must be GPIO21");
_Static_assert(JR_AUDIO_LRC_GPIO == 47, "HW04 WS must be GPIO47");
_Static_assert(JR_AUDIO_DIN_GPIO == 42, "HW04 amplifier DIN must use GPIO42");
_Static_assert(JR_MIC_SCK_GPIO == 21, "HW04 microphone SCK must use GPIO21");
_Static_assert(JR_MIC_WS_GPIO == 47, "HW04 microphone WS must use GPIO47");
_Static_assert(JR_MIC_SD_GPIO == 41, "HW04 microphone SD must use GPIO41");
