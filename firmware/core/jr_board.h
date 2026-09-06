#pragma once
#include "sdkconfig.h"

/* Physical GPIO numbers, not connector positions. OLED pinout is fixed. */
#define JR_OLED_SDA_GPIO 1
#define JR_OLED_SCL_GPIO 2
#define JR_OLED_I2C_PORT 0
#define JR_OLED_I2C_HZ 100000
#define JR_AUDIO_BCLK_GPIO 39
#define JR_AUDIO_LRC_GPIO 40
#define JR_AUDIO_DIN_GPIO 41
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
/* Existing camera wiring: not a universal ESP32-S3 pinout. */
#define JR_CAM_PWDN -1
#define JR_CAM_RESET -1
#define JR_CAM_XCLK 15
#define JR_CAM_SDA 4
#define JR_CAM_SCL 5
#define JR_CAM_D0 11
#define JR_CAM_D1 9
#define JR_CAM_D2 8
#define JR_CAM_D3 10
#define JR_CAM_D4 12
#define JR_CAM_D5 18
#define JR_CAM_D6 17
#define JR_CAM_D7 16
#define JR_CAM_VSYNC 6
#define JR_CAM_HREF 7
#define JR_CAM_PCLK 13
#if defined(CONFIG_JR_AUDIO_PINS_CONFIRMED) && CONFIG_JR_AUDIO_PINS_CONFIRMED
#define JR_AUDIO_ENABLED 1

#else
#define JR_AUDIO_ENABLED 0

#endif
#if !JR_OLED_ENABLED
#define JR_PROFILE_NAME "headless_diagnostic"
#elif JR_AUDIO_ENABLED
#define JR_PROFILE_NAME "face_audio"
#else
#define JR_PROFILE_NAME "face_only"
#endif
_Static_assert(JR_OLED_SDA_GPIO == 1 && JR_OLED_SCL_GPIO == 2, "OLED must remain on GPIO1/2");
_Static_assert(JR_AUDIO_BCLK_GPIO != JR_OLED_SDA_GPIO && JR_AUDIO_BCLK_GPIO != JR_OLED_SCL_GPIO, "GPIO conflict");
_Static_assert(JR_AUDIO_LRC_GPIO != JR_OLED_SDA_GPIO && JR_AUDIO_LRC_GPIO != JR_OLED_SCL_GPIO, "GPIO conflict");
_Static_assert(JR_AUDIO_DIN_GPIO != JR_OLED_SDA_GPIO && JR_AUDIO_DIN_GPIO != JR_OLED_SCL_GPIO, "GPIO conflict");
