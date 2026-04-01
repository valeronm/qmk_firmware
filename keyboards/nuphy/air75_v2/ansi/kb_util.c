#include "kb_util.h"

#define EEPROM_INIT_MARKER 0xA5

kb_config_t kb_config;

static bool kb_config_is_valid(void) {
    if (kb_config.side_mode > SIDE_OFF) return false;
    if (kb_config.side_light >= SIDE_BRIGHT_LEVELS) return false;
    if (kb_config.side_speed >= SIDE_SPEED_LEVELS) return false;
    if (kb_config.side_color >= SIDE_COLOUR_MAX) return false;
    return true;
}

void loading_eeprom_data(void) {
    eeconfig_read_kb_datablock(&kb_config, 0, sizeof(kb_config));
    if (kb_config.default_brightness_flag != EEPROM_INIT_MARKER || !kb_config_is_valid()) {
        rgb_matrix_sethsv(255, 255, RGB_MATRIX_MAXIMUM_BRIGHTNESS - RGB_MATRIX_VAL_STEP * 2);
        init_kb_config();
        save_kb_config();
    }
}

void init_kb_config(void) {
    kb_config.default_brightness_flag = EEPROM_INIT_MARKER;
    kb_config.side_mode               = SIDE_WAVE;
    kb_config.side_light              = 3;
    kb_config.side_speed              = 2;
    kb_config.side_rgb                = 1;
    kb_config.side_color              = 0;
    kb_config.sleep_enable            = true;
}

void save_kb_config(void) {
    eeconfig_update_kb_datablock(&kb_config, 0, sizeof(kb_config));
}
