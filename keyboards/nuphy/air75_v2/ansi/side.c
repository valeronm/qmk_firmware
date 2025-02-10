/*
Copyright 2023 @ Nuphy <https://nuphy.com/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ansi.h"
#include "side_table.h"
#include "kb_util.h"

#define SIDE_EFFECTS        5
#define SIDE_BRIGHT_LEVELS  6
#define SIDE_SPEED_LEVELS   5
#define SIDE_COLOUR_MAX     8

#define SIDE_LINE_LEDS      6
#define SIDE_LINE_NUM       2
#define SIDE_LED_NUM        SIDE_LINE_LEDS * SIDE_LINE_NUM

#define RF_LED_LINK_PERIOD  500
#define RF_LED_PAIR_PERIOD  250

/* positions */
enum {
    LEFT_SIDE,
    RIGHT_SIDE,
};

uint8_t side_play_point     = 0;
uint8_t side_play_cnt       = 0;
uint32_t side_play_timer    = 0;
uint8_t r_temp, g_temp, b_temp;
rgb_t side_leds[SIDE_LED_NUM] = {0};

const uint8_t side_speed_table[SIDE_EFFECTS][SIDE_SPEED_LEVELS] = {
    [SIDE_WAVE]   = {10, 14, 20, 28, 38}, //
    [SIDE_MIX]    = {10, 14, 20, 28, 38}, //
    [SIDE_STATIC] = {50, 50, 50, 50, 50}, //
    [SIDE_BREATH] = {10, 14, 20, 28, 38}, //
    [SIDE_OFF]    = {50, 50, 50, 50, 50}, //
};

const uint8_t side_light_table[SIDE_BRIGHT_LEVELS] = {
    0,   //
    22,  //
    34,  //
    55,  //
    79,  //
    106, //
};

const uint8_t side_led_index_tab[SIDE_LINE_LEDS][SIDE_LINE_NUM] = {
    {5, 6},  //
    {4, 7},  //
    {3, 8},  //
    {2, 9},  //
    {1, 10}, //
    {0, 11}, //
};

extern DEV_INFO_STRUCT dev_info;
extern uint8_t         rf_blink_cnt;
extern uint16_t        rf_link_show_time;
extern bool            f_bat_hold;
extern bool            f_sys_show;
extern bool            f_sleep_show;

void side_ws2812_setleds(rgb_led_t *ledarray, uint16_t leds);
void rgb_matrix_update_pwm_buffers(void);

/**
 * @brief  side leds set color vaule.
 * @param  index: index of side_leds[].
 * @param  ...
 */
void side_rgb_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    side_leds[index].r = red;
    side_leds[index].g = green;
    side_leds[index].b = blue;
}

void side_rgb_set_row_color(uint8_t row, uint8_t red, uint8_t green, uint8_t blue) {
    for (int i = 0; i < SIDE_LINE_NUM; i++)
        side_rgb_set_color(side_led_index_tab[row][i], red, green, blue);
}

/**
 * @brief  refresh side leds.
 */
void side_rgb_refresh(void) {
    side_ws2812_setleds(side_leds, SIDE_LED_NUM);
}

/**
 * @brief  Adjusting the brightness of side lights.
 * @param  dir: 0 - decrease, 1 - increase.
 * @note  save to eeprom.
 */
void side_light_control(uint8_t dir) {
    if (dir) {
        if (kb_config.side_light < SIDE_BRIGHT_LEVELS - 1)
            kb_config.side_light++;
    } else {
        if (kb_config.side_light > 0)
            kb_config.side_light--;
    }
    save_kb_config();
}

/**
 * @brief  Adjusting the speed of side lights.
 * @param  dir: 0 - decrease, 1 - increase.
 * @note  save to eeprom.
 */
void side_speed_control(uint8_t dir) {
    if (kb_config.side_speed > SIDE_SPEED_LEVELS)
        kb_config.side_speed = 2;

    if (dir) {
        if (kb_config.side_speed > 0)
            kb_config.side_speed--;
    } else {
        if (kb_config.side_speed < SIDE_SPEED_LEVELS - 1)
            kb_config.side_speed++;
    }
    save_kb_config();
}

/**
 * @brief  Switch to the next color of side lights.
 * @param  dir: 0 - prev, 1 - next.
 * @note  save to eeprom.
 */
void side_colour_control(uint8_t dir) {
    if (kb_config.side_mode != SIDE_WAVE) {
        if (kb_config.side_rgb) {
            kb_config.side_rgb    = 0;
            kb_config.side_color = 0;
        }
    }
    if (dir) {
        if (kb_config.side_rgb) {
            kb_config.side_rgb    = 0;
            kb_config.side_color = 0;
        } else {
            kb_config.side_color++;
            if (kb_config.side_color >= SIDE_COLOUR_MAX) {
                kb_config.side_rgb    = 1;
                kb_config.side_color = 0;
            }
        }
    } else {
        if (kb_config.side_rgb) {
            kb_config.side_rgb    = 0;
            kb_config.side_color = SIDE_COLOUR_MAX - 1;
        } else {
            kb_config.side_color--;
            if (kb_config.side_color >= SIDE_COLOUR_MAX) {
                kb_config.side_rgb    = 1;
                kb_config.side_color = 0;
            }
        }
    }
    save_kb_config();
}

/**
 * @brief  Change the color mode of side lights.
 * @param  dir: 0 - prev, 1 - next.
 * @note  save to eeprom.
 */
void side_mode_control(uint8_t dir) {
    if (dir) {
        kb_config.side_mode++;
        if (kb_config.side_mode > SIDE_OFF) {
            kb_config.side_mode = SIDE_WAVE;
        }
    } else {
        if (kb_config.side_mode > SIDE_WAVE) {
            kb_config.side_mode--;
        } else {
            kb_config.side_mode = SIDE_OFF;
        }
    }
    side_play_point          = 0;
    save_kb_config();
}

/**
 * @brief  set left side leds.
 * @param  ...
 */
void set_left_rgb(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < SIDE_LINE_LEDS; i++)
        side_rgb_set_color(side_led_index_tab[i][LEFT_SIDE], r, g, b);
}

/**
 * @brief  set right side leds.
 * @param  ...
 */
void set_right_rgb(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < SIDE_LINE_LEDS; i++)
        side_rgb_set_color(side_led_index_tab[i][RIGHT_SIDE], r, g, b);
}

/**
 * @brief  set both side leds.
 * @param  ...
 */
void set_both_rgb(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < SIDE_LINE_LEDS; i++)
        side_rgb_set_row_color(i, r, g, b);
}

/**
 * @brief  set left side leds.
 */
void sys_sw_led_show(void) {
    static uint32_t sys_show_timer = 0;
    static bool     sys_show_flag  = false;

    if (f_sys_show) {
        f_sys_show     = false;
        sys_show_timer = timer_read32();
        sys_show_flag  = true;
    }

    if (sys_show_flag) {
        if ((timer_elapsed32(sys_show_timer) / 500) % 2 == 0) {
            if (dev_info.sys_sw_state == SYS_SW_MAC) {
                set_right_rgb(RGB_HALF_WHITE);
            } else {
                set_right_rgb(RGB_HALF_BLUE);
            }
        } else {
            set_right_rgb(RGB_OFF);
        }
        if (timer_elapsed32(sys_show_timer) >= 3000) {
            sys_show_flag = false;
        }
    }
}

/**
 * @brief  sleep_sw_led_show.
 */
void sleep_sw_led_show(void) {
    static uint32_t sleep_show_timer = 0;
    static bool     sleep_show_flag  = false;

    if (f_sleep_show) {
        f_sleep_show     = false;
        sleep_show_timer = timer_read32();
        sleep_show_flag  = true;
    }

    if (sleep_show_flag) {
        if ((timer_elapsed32(sleep_show_timer) / 500) % 2 == 0) {
            if (kb_config.sleep_enable) {
                set_right_rgb(RGB_HALF_GREEN);
            } else {
                set_right_rgb(RGB_HALF_RED);
            }
        } else {
            set_right_rgb(RGB_OFF);
        }
        if (timer_elapsed32(sleep_show_timer) >= 3000) {
            sleep_show_flag = false;
        }
    }
}

/**
 * @brief  sys_led_show.
 */
void sys_led_show(void) {
    bool caps_lock = false;
    if (dev_info.link_mode == LINK_USB)
        caps_lock = host_keyboard_led_state().caps_lock;
    else
        caps_lock = dev_info.rf_led & 0x02;

    if (caps_lock)
        set_left_rgb(RGB_HALF_CYAN);
}

/**
 * @brief  light_point_playing.
 * @param trend:
 * @param step:
 * @param len:
 * @param point:
 */
static void light_point_playing(uint8_t trend, uint8_t step, uint8_t len, uint8_t *point) {
    if (trend) {
        *point += step;
        if (*point >= len) *point -= len;
    } else {
        *point -= step;
        if (*point >= len) *point = len - (255 - *point) - 1;
    }
}

/**
 * @brief  count_rgb_light.
 * @param light_temp:
 */
static void count_rgb_light(uint8_t light_temp) {
    uint16_t temp;

    temp   = (light_temp)*r_temp + r_temp;
    r_temp = temp >> 8;

    temp   = (light_temp)*g_temp + g_temp;
    g_temp = temp >> 8;

    temp   = (light_temp)*b_temp + b_temp;
    b_temp = temp >> 8;
}

/**
 * @brief  side_wave_mode_show.
 */
static void side_wave_mode_show(void) {
    uint8_t play_index;

    if (kb_config.side_rgb)
        light_point_playing(0, 3, FLOW_COLOUR_TAB_LEN, &side_play_point);
    else
        light_point_playing(0, 2, WAVE_TAB_LEN, &side_play_point);

    play_index = side_play_point;
    for (int i = 0; i < SIDE_LINE_LEDS; i++) {
        if (kb_config.side_rgb) {
            r_temp = flow_rainbow_colour_tab[play_index].r;
            g_temp = flow_rainbow_colour_tab[play_index].g;
            b_temp = flow_rainbow_colour_tab[play_index].b;

            light_point_playing(1, 24, FLOW_COLOUR_TAB_LEN, &play_index);
        } else {
            r_temp = colour_lib[kb_config.side_color].r;
            g_temp = colour_lib[kb_config.side_color].g;
            b_temp = colour_lib[kb_config.side_color].b;

            light_point_playing(1, 12, WAVE_TAB_LEN, &play_index);
            count_rgb_light(wave_data_tab[play_index]);
        }

        count_rgb_light(side_light_table[kb_config.side_light]);

        side_rgb_set_row_color(i, r_temp >> 2, g_temp >> 2, b_temp >> 2);
    }
}

/**
 * @brief  side_spectrum_mode_show.
 */
static void side_spectrum_mode_show(void) {
    light_point_playing(1, 1, FLOW_COLOUR_TAB_LEN, &side_play_point);

    r_temp = flow_rainbow_colour_tab[side_play_point].r;
    g_temp = flow_rainbow_colour_tab[side_play_point].g;
    b_temp = flow_rainbow_colour_tab[side_play_point].b;

    count_rgb_light(side_light_table[kb_config.side_light]);

    set_both_rgb(r_temp >> 2, g_temp >> 2, b_temp >> 2);
}

/**
 * @brief  side_breathe_mode_show.
 */
static void side_breathe_mode_show(void) {
    static uint8_t play_point = 0;

    light_point_playing(0, 1, BREATHE_TAB_LEN, &play_point);

    r_temp = colour_lib[kb_config.side_color].r;
    g_temp = colour_lib[kb_config.side_color].g;
    b_temp = colour_lib[kb_config.side_color].b;

    count_rgb_light(breathe_data_tab[play_point]);
    count_rgb_light(side_light_table[kb_config.side_light]);

    set_both_rgb(r_temp >> 2, g_temp >> 2, b_temp >> 2);
}

/**
 * @brief  side_static_mode_show.
 */
static void side_static_mode_show(void) {
    for (int i = 0; i < SIDE_LINE_LEDS; i++) {
        r_temp = colour_lib[kb_config.side_color].r;
        g_temp = colour_lib[kb_config.side_color].g;
        b_temp = colour_lib[kb_config.side_color].b;

        count_rgb_light(side_light_table[kb_config.side_light]);

        side_rgb_set_row_color(i, r_temp >> 2, g_temp >> 2, b_temp >> 2);
    }
}

/**
 * @brief  side_off_mode_show.
 */
static void side_off_mode_show(void) {
    set_both_rgb(RGB_OFF);
}

/**
 * @brief  rf_led_show.
 */
void rf_led_show(void) {
    static uint32_t rf_blink_timer = 0;
    uint16_t        rf_blink_priod = 0;

    if (rf_blink_cnt || (rf_link_show_time < RF_LINK_SHOW_TIME)) {
        if (dev_info.link_mode == LINK_RF_24) {
            set_left_rgb(RGB_HALF_GREEN);
        } else if (dev_info.link_mode == LINK_USB) {
            set_left_rgb(RGB_HALF_YELLOW);
        } else {
            set_left_rgb(RGB_HALF_BLUE);
        }
    } else {
        rf_blink_timer = timer_read32();
        return;
    }

    if (rf_blink_cnt) {
        if (dev_info.rf_state == RF_PAIRING)
            rf_blink_priod = RF_LED_PAIR_PERIOD;
        else
            rf_blink_priod = RF_LED_LINK_PERIOD;

        if (timer_elapsed32(rf_blink_timer) > (rf_blink_priod >> 1)) {
            set_left_rgb(RGB_OFF);
        }

        if (timer_elapsed32(rf_blink_timer) >= rf_blink_priod) {
            rf_blink_cnt--;
            rf_blink_timer = timer_read32();
        }
    }
}

/**
 * @brief  bat_num_led.
 */
void bat_num_led(uint8_t bat_percent)
{
    uint8_t r, g, b;

    // set color
    if (bat_percent <= 15) {
        r = 0xff; g = 0x00; b = 0x00;
    }
    else if (bat_percent <= 50) {
        r = 0xff; g = 0x40; b = 0x00;
    }
    else if (bat_percent <= 80) {
        r = 0xff; g = 0xff; b = 0x00;
    }
    else {
        r = 0x00; g = 0xff; b = 0x00;
    }

    // set percent
    if (bat_percent >= 1) rgb_matrix_set_color(29, r, g, b);
    if (bat_percent > 10) rgb_matrix_set_color(28, r, g, b);
    if (bat_percent > 20) rgb_matrix_set_color(27, r, g, b);
    if (bat_percent > 30) rgb_matrix_set_color(26, r, g, b);
    if (bat_percent > 40) rgb_matrix_set_color(25, r, g, b);
    if (bat_percent > 50) rgb_matrix_set_color(24, r, g, b);
    if (bat_percent > 60) rgb_matrix_set_color(23, r, g, b);
    if (bat_percent > 70) rgb_matrix_set_color(22, r, g, b);
    if (bat_percent > 80) rgb_matrix_set_color(21, r, g, b);
    if (bat_percent > 90) rgb_matrix_set_color(20, r, g, b);
}

void num_led_show(void)
{
    static uint8_t num_bat_temp         = 0;
    num_bat_temp         = dev_info.rf_baterry;
    bat_num_led(num_bat_temp);
}

void bat_led_close(void)
{
    for(int i=20; i<=29; i++) {
        rgb_matrix_set_color(i,0,0,0);
    }

}

/**
 * @brief  bat_percent_led.
 */
void bat_percent_led(uint8_t bat_percent) {
    uint8_t bat_end_led = 0;
    uint8_t bat_r, bat_g, bat_b;

    if (bat_percent <= 15) {
        bat_end_led = 0;
        bat_r = 0x80, bat_g = 0, bat_b = 0;
    } else if (bat_percent <= 20) {
        bat_end_led = 1;
        bat_r = 0x80, bat_g = 0x40, bat_b = 0;
    } else if (bat_percent <= 40) {
        bat_end_led = 2;
        bat_r = 0x80, bat_g = 0x40, bat_b = 0;
    } else if (bat_percent <= 60) {
        bat_end_led = 3;
        bat_r = 0x80, bat_g = 0x40, bat_b = 0;
    } else if (bat_percent <= 80) {
        bat_end_led = 4;
        bat_r = 0x80, bat_g = 0x40, bat_b = 0;
    } else if (bat_percent <= 95) {
        bat_end_led = 5;
        bat_r = 0x80, bat_g = 0x40, bat_b = 0;
    } else {
        bat_end_led = 5;
        bat_r = 0, bat_g = 0x80, bat_b = 0;
    }

    uint8_t i = 0;
    for (; i <= bat_end_led; i++)
        side_rgb_set_color(11 - i, bat_r >> 2, bat_g >> 2, bat_b >> 2);

    for (; i < 6; i++)
        side_rgb_set_color(11 - i, 0, 0, 0);
}

/**
 * @brief  bat_led_show.
 */
void bat_led_show(void) {
    static bool bat_show_flag   = 1;
    static bool bat_show_breath = 0;
    static bool f_init          = 1;

    static uint8_t  play_point     = 0;
    static uint32_t bat_play_timer = 0;
    static uint32_t bat_show_time  = 0;

    static uint32_t bat_sts_debounce = 0;
    static uint32_t bat_per_debounce = 0;
    static uint8_t  charge_state     = 0;
    static uint8_t  bat_percent      = 0;

    if (f_init) {
        f_init        = 0;
        bat_show_time = timer_read32();
        charge_state  = dev_info.rf_charge;
        bat_percent   = dev_info.rf_baterry;
    }

    if (charge_state != dev_info.rf_charge) {
        if (timer_elapsed32(bat_sts_debounce) > 1000) {
            if (((charge_state & 0x01) == 0) && ((dev_info.rf_charge & 0x01) != 0)) {
                bat_show_flag   = true;
                bat_show_breath = true;
                bat_show_time   = timer_read32();
            }
            charge_state = dev_info.rf_charge;
        }
    } else {
        bat_sts_debounce = timer_read32();
        if (timer_elapsed32(bat_show_time) > 5000) {
            bat_show_flag   = false;
            bat_show_breath = false;
        }
        if (charge_state == 0x03) {
            bat_show_breath = true;
        } else if (charge_state & 0x01) {
            dev_info.rf_baterry = 100;
        }
    }

    if (bat_percent != dev_info.rf_baterry) {
        if (timer_elapsed32(bat_per_debounce) > 1000) {
            bat_percent = dev_info.rf_baterry;
        }
    } else {
        bat_per_debounce = timer_read32();
        if (bat_percent < 15) {
            bat_show_flag = true;
            bat_show_time = timer_read32();
        }
    }

    if (f_bat_hold || bat_show_flag) {
        if (bat_show_breath) {
            if (timer_elapsed32(bat_play_timer) > 10) {
                bat_play_timer = timer_read32();
                light_point_playing(0, 1, BREATHE_TAB_LEN, &play_point);
            }
            count_rgb_light(breathe_data_tab[play_point]);
            set_right_rgb(RGB_HALF_ORANGE);
        } else {
            bat_percent_led(bat_percent);
        }
    }
}

/**
 * @brief  device_reset_show.
 */
void device_reset_show(void) {

    writePinHigh(DC_BOOST_PIN);
    setPinOutput(DRIVER_SIDE_CS_PIN);
    setPinOutput(DRIVER_LED_CS_PIN);
    writePinLow(DRIVER_SIDE_CS_PIN);
    writePinLow(DRIVER_LED_CS_PIN);

    for (int blink_cnt = 0; blink_cnt < 3; blink_cnt++) {
        rgb_matrix_set_color_all(0x10, 0x10, 0x10);
        set_both_rgb(RGB_QUARTER_WHITE);
        rgb_matrix_update_pwm_buffers();
        side_rgb_refresh();
        wait_ms(200);

        rgb_matrix_set_color_all(RGB_OFF);
        set_both_rgb(RGB_OFF);
        rgb_matrix_update_pwm_buffers();
        side_rgb_refresh();
        wait_ms(200);
    }
}

/**
 * @brief  device_reset_init.
 */
void device_reset_init(void) {
    side_play_point = 0;

    side_play_cnt   = 0;
    side_play_timer = timer_read32();

    f_bat_hold = false;

    rgb_matrix_enable();
    rgb_matrix_mode(RGB_MATRIX_DEFAULT_MODE);
    rgb_matrix_set_speed(255 - RGB_MATRIX_SPD_STEP * 2);
    rgb_matrix_sethsv(255, 255, RGB_MATRIX_MAXIMUM_BRIGHTNESS - RGB_MATRIX_VAL_STEP * 2);

    init_kb_config();
    save_kb_config();
}

/**
 *      RGB test
*/
void rgb_test_show(void)
{
    // open power control
    writePinHigh(DC_BOOST_PIN);
    setPinOutput(DRIVER_LED_CS_PIN);
    writePinLow(DRIVER_LED_CS_PIN);
    setPinOutput(DRIVER_SIDE_CS_PIN);
    writePinLow(DRIVER_SIDE_CS_PIN);

    // set test color
    rgb_matrix_set_color_all(RGB_RED);
    rgb_matrix_update_pwm_buffers();
    set_both_rgb(RGB_RED);
    side_rgb_refresh();
    wait_ms(500);

    rgb_matrix_set_color_all(RGB_GREEN);
    rgb_matrix_update_pwm_buffers();
    set_both_rgb(RGB_GREEN);
    side_rgb_refresh();
    wait_ms(500);

    rgb_matrix_set_color_all(RGB_BLUE);
    rgb_matrix_update_pwm_buffers();
    set_both_rgb(RGB_BLUE);
    side_rgb_refresh();
    wait_ms(500);

    rgb_matrix_set_color_all(RGB_HALF_WHITE);
    rgb_matrix_update_pwm_buffers();
    set_both_rgb(RGB_HALF_WHITE);
    side_rgb_refresh();
    wait_ms(500);

    rgb_matrix_set_color_all(RGB_HALF_YELLOW);
    rgb_matrix_update_pwm_buffers();
    set_both_rgb(RGB_HALF_YELLOW);
    side_rgb_refresh();
    wait_ms(500);

    rgb_matrix_set_color_all(RGB_HALF_MAGENTA);
    rgb_matrix_update_pwm_buffers();
    set_both_rgb(RGB_HALF_MAGENTA);
    side_rgb_refresh();
    wait_ms(500);

    rgb_matrix_set_color_all(RGB_HALF_CYAN);
    rgb_matrix_update_pwm_buffers();
    set_both_rgb(RGB_HALF_CYAN);
    side_rgb_refresh();
    wait_ms(500);
}

/**
 * @brief  side_led_show.
 */
void side_led_show(void) {
    static uint32_t side_refresh_time = 0;

    side_play_cnt += timer_elapsed32(side_play_timer);
    side_play_timer = timer_read32();

    bool change_effect = false;
    uint8_t change_delay = side_speed_table[kb_config.side_mode][kb_config.side_speed];
    if (side_play_cnt > change_delay) {
        side_play_cnt -= change_delay;
        change_effect = true;
    }

    if (change_effect) {
        if (side_play_cnt > 20) side_play_cnt = 0;

        switch (kb_config.side_mode) {
            case SIDE_WAVE:
                side_wave_mode_show();
                break;
            case SIDE_MIX:
                side_spectrum_mode_show();
                break;
            case SIDE_BREATH:
                side_breathe_mode_show();
                break;
            case SIDE_STATIC:
                side_static_mode_show();
                break;
            case SIDE_OFF:
                side_off_mode_show();
                break;
        }
    }

    bat_led_show();
    sleep_sw_led_show();
    sys_sw_led_show();

    sys_led_show();
    rf_led_show();

    if (timer_elapsed32(side_refresh_time) > 20) {
        side_refresh_time = timer_read32();
        side_rgb_refresh();
    }
}
