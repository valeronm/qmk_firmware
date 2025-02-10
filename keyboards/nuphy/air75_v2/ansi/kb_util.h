#pragma once

#include "quantum.h"

/* side rgb mode */
enum {
    SIDE_WAVE = 0,
    SIDE_MIX,
    SIDE_STATIC,
    SIDE_BREATH,
    SIDE_OFF,
};

typedef struct
{
    uint8_t default_brightness_flag;
    uint8_t side_mode;
    uint8_t side_light;
    uint8_t side_speed;
    uint8_t side_rgb;
    uint8_t side_color;
    uint8_t sleep_enable;
    uint8_t retain1;
    uint8_t retain2;
} kb_config_t;

kb_config_t kb_config;

void loading_eeprom_data(void);
void init_kb_config(void);
void save_kb_config(void);
