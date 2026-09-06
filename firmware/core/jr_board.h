#pragma once
#include "sdkconfig.h"
#include "jr_pinmap_fixed.h"

#define JR_OLED_I2C_PORT 0
#define JR_OLED_I2C_HZ 100000

#if defined(CONFIG_JR_HEADLESS_DIAGNOSTIC) && CONFIG_JR_HEADLESS_DIAGNOSTIC
#define JR_OLED_ENABLED 0
#else
#define JR_OLED_ENABLED 1
#endif

#if defined(CONFIG_JR_CAMERA_PINS_CONFIRMED) && CONFIG_JR_CAMERA_PINS_CONFIRMED
#define JR_CAMERA_ENABLED 1
#else
#define JR_CAMERA_ENABLED 0
#endif

#if defined(CONFIG_JR_AUDIO_HW04_CONFIRMED) && CONFIG_JR_AUDIO_HW04_CONFIRMED
#define JR_AUDIO_ENABLED 1
#else
#define JR_AUDIO_ENABLED 0
#endif

/* MS3625 pinout is fixed in HW04, but the RX driver is not implemented yet. */
#define JR_MIC_PINOUT_DEFINED 1
#define JR_MIC_ENABLED 0

#if !JR_OLED_ENABLED
#define JR_PROFILE_NAME "headless_diagnostic"
#elif JR_AUDIO_ENABLED
#define JR_PROFILE_NAME "face_audio"
#else
#define JR_PROFILE_NAME "face_only"
#endif

_Static_assert(JR_AUDIO_BCLK_GPIO == 21, "HW04 BCLK must be GPIO21");
_Static_assert(JR_AUDIO_LRC_GPIO == 47, "HW04 WS must be GPIO47");
_Static_assert(JR_AUDIO_DIN_GPIO == 42, "HW04 amplifier DIN must use GPIO42");
_Static_assert(JR_MIC_SD_GPIO == 41, "HW04 microphone SD must use GPIO41");
