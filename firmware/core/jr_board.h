#pragma once
#include "sdkconfig.h"
#include "jr_pinmap_fixed.h"

/* New revision-specific approval: old sdkconfig audio approval is NOT reused. */
#ifndef CONFIG_JR_AUDIO_BCLK_GPIO
#define CONFIG_JR_AUDIO_BCLK_GPIO -1
#endif
#ifndef CONFIG_JR_AUDIO_WS_GPIO
#define CONFIG_JR_AUDIO_WS_GPIO -1
#endif
#ifndef CONFIG_JR_AUDIO_DOUT_GPIO
#define CONFIG_JR_AUDIO_DOUT_GPIO -1
#endif
#define JR_AUDIO_BCLK_GPIO CONFIG_JR_AUDIO_BCLK_GPIO
#define JR_AUDIO_LRC_GPIO CONFIG_JR_AUDIO_WS_GPIO
#define JR_AUDIO_DIN_GPIO CONFIG_JR_AUDIO_DOUT_GPIO
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
#if defined(CONFIG_JR_AUDIO_HW03_CONFIRMED) && CONFIG_JR_AUDIO_HW03_CONFIRMED
#define JR_AUDIO_ENABLED 1
#else
#define JR_AUDIO_ENABLED 0
#endif

/* Policy whitelist, NOT a statement that a pin is free on the physical PCB.
 * Existing OLED/camera, USB, UART, boot and memory pins stay reserved even off.
 * 39/40 also need an external-JTAG check; 48 may drive an on-board RGB LED.
 */
#define JR_AUDIO_CANDIDATE(p) ((p)==14 || (p)==38 || (p)==39 || (p)==40 || (p)==48)
#define JR_AUDIO_CONFIG_VALID(p) ((p)==-1 || (JR_AUDIO_CANDIDATE(p) && !JR_GPIO_BLOCKED(p)))
_Static_assert(JR_AUDIO_CONFIG_VALID(JR_AUDIO_BCLK_GPIO), "Unsafe BCLK GPIO: see docs/PINAGEM.md");
_Static_assert(JR_AUDIO_CONFIG_VALID(JR_AUDIO_LRC_GPIO), "Unsafe WS GPIO: see docs/PINAGEM.md");
_Static_assert(JR_AUDIO_CONFIG_VALID(JR_AUDIO_DIN_GPIO), "Unsafe DOUT GPIO: see docs/PINAGEM.md");
_Static_assert(JR_AUDIO_BCLK_GPIO < 0 || JR_AUDIO_LRC_GPIO < 0 || JR_AUDIO_BCLK_GPIO != JR_AUDIO_LRC_GPIO, "Audio BCLK/WS conflict");
_Static_assert(JR_AUDIO_BCLK_GPIO < 0 || JR_AUDIO_DIN_GPIO < 0 || JR_AUDIO_BCLK_GPIO != JR_AUDIO_DIN_GPIO, "Audio BCLK/DOUT conflict");
_Static_assert(JR_AUDIO_LRC_GPIO < 0 || JR_AUDIO_DIN_GPIO < 0 || JR_AUDIO_LRC_GPIO != JR_AUDIO_DIN_GPIO, "Audio WS/DOUT conflict");
#if JR_AUDIO_ENABLED
_Static_assert(JR_AUDIO_BCLK_GPIO >= 0 && JR_AUDIO_LRC_GPIO >= 0 && JR_AUDIO_DIN_GPIO >= 0,
               "Audio needs three physically verified GPIOs; -1 means unassigned");
#endif
_Static_assert(JR_OLED_SDA_GPIO==1 && JR_OLED_SCL_GPIO==2,"Keep OLED on GPIO1/2");
#if !JR_OLED_ENABLED
#define JR_PROFILE_NAME "headless_diagnostic"
#elif JR_AUDIO_ENABLED
#define JR_PROFILE_NAME "face_audio"
#else
#define JR_PROFILE_NAME "face_only"
#endif
