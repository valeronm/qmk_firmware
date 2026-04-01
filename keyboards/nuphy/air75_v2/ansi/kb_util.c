#include "kb_util.h"

#define DEFAULT_BRIGHTNESS_FLAG 0xA5

void loading_eeprom_data(void) {
    eeconfig_read_kb_datablock(&kb_config, 0, sizeof(kb_config));
    if (kb_config.default_brightness_flag != DEFAULT_BRIGHTNESS_FLAG) {
        /* first power on, set rgb matrix brightness at middle level*/
        rgb_matrix_sethsv(255, 255, RGB_MATRIX_MAXIMUM_BRIGHTNESS - RGB_MATRIX_VAL_STEP * 2);
        init_kb_config();
        save_kb_config();
    }
}

void init_kb_config(void) {
    kb_config.default_brightness_flag = DEFAULT_BRIGHTNESS_FLAG;
    kb_config.side_mode    = SIDE_WAVE;
    kb_config.side_light   = 3;
    kb_config.side_speed   = 2;
    kb_config.side_rgb     = 1;
    kb_config.side_color   = 0;
    kb_config.sleep_enable = true;
}

void save_kb_config(void) {
    eeconfig_update_kb_datablock(&kb_config, 0, sizeof(kb_config));
}
