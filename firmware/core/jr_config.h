#pragma once

#define JR_APP_VERSION "JrBot_V2.1.8D_TTS_TWO_BLOCK_2S"
#define JR_BUILD_STAMP_SP "2026-11-26_DEVELOP_TTS_TWO_BLOCK_2S"

/* Generated locally on every installer build to identify the actual Git commit. */
#if __has_include("jr_firmware_revision_local.h")
#include "jr_firmware_revision_local.h"
#else
#define JR_FIRMWARE_GIT_COMMIT "unknown"
#endif

#if __has_include("wifi_config.local.h")
#include "wifi_config.local.h"
#endif
#ifndef JR_WIFI_SSID
#define JR_WIFI_SSID ""
#endif
#ifndef JR_WIFI_PASSWORD
#define JR_WIFI_PASSWORD ""
#endif
#ifndef JR_WIFI_HOSTNAME
#define JR_WIFI_HOSTNAME "jrbot"
#endif
#ifndef JR_WIFI_USE_STATIC_IP
#define JR_WIFI_USE_STATIC_IP 0
#endif
#ifndef JR_WIFI_STATIC_IP
#define JR_WIFI_STATIC_IP "192.168.0.50"
#endif
#ifndef JR_WIFI_GATEWAY
#define JR_WIFI_GATEWAY "192.168.0.1"
#endif
#ifndef JR_WIFI_SUBNET
#define JR_WIFI_SUBNET "255.255.255.0"
#endif
#ifndef JR_WIFI_DNS1
#define JR_WIFI_DNS1 "8.8.8.8"
#endif
#ifndef JR_WIFI_DNS2
#define JR_WIFI_DNS2 "8.8.4.4"
#endif

