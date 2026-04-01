// Copyright 2023 Nuphy <https://nuphy.com/>
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Second WS2812 instance for side LEDs on DRIVER_SIDE_PIN (C8).
// Uses the work_louder pattern: rename QMK's bitbang driver symbols
// and re-include it with a different pin.

#include "config.h"
#include "ws2812.h"

// Override pin and LED count for the side LED chain
#undef WS2812_DI_PIN
#define WS2812_DI_PIN DRIVER_SIDE_PIN

#undef WS2812_LED_COUNT
#define WS2812_LED_COUNT 12

// Rename all symbols to avoid conflict with main WS2812 instance
#define sendByte             side_sendByte
#define ws2812_leds          side_ws2812_leds
#define ws2812_init          side_ws2812_init
#define ws2812_set_color     side_ws2812_set_color
#define ws2812_set_color_all side_ws2812_set_color_all
#define ws2812_flush         side_ws2812_flush

#include "ws2812_bitbang.c"
