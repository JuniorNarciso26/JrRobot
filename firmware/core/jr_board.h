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
#define JR_CAMERA_ENABLED 0 /* Deferred until the physical board is verified. */
#if defined(CONFIG_JR_AUDIO_PINS_CONFIRMED) && CONFIG_JR_AUDIO_PINS_CONFIRMED
#define JR_AUDIO_ENABLED 1
#define JR_PROFILE_NAME "face_audio"
#else
#define JR_AUDIO_ENABLED 0
#define JR_PROFILE_NAME "face_only"
#endif
_Static_assert(JR_OLED_SDA_GPIO == 1 && JR_OLED_SCL_GPIO == 2, "OLED must remain on GPIO1/2");
_Static_assert(JR_AUDIO_BCLK_GPIO != JR_OLED_SDA_GPIO && JR_AUDIO_BCLK_GPIO != JR_OLED_SCL_GPIO, "GPIO conflict");
_Static_assert(JR_AUDIO_LRC_GPIO != JR_OLED_SDA_GPIO && JR_AUDIO_LRC_GPIO != JR_OLED_SCL_GPIO, "GPIO conflict");
_Static_assert(JR_AUDIO_DIN_GPIO != JR_OLED_SDA_GPIO && JR_AUDIO_DIN_GPIO != JR_OLED_SCL_GPIO, "GPIO conflict");
