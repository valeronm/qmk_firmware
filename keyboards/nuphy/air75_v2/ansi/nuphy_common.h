// Copyright 2023 Nuphy <https://nuphy.com/>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"
#include "ansi.h"
#include "kb_util.h"

// ---- Shared global state ----

extern DEV_INFO_STRUCT dev_info;
extern host_driver_t  *m_host_driver;
extern uint8_t         host_mode;

// Timers (incremented every 10ms in timer_pro)
extern uint16_t rf_linking_time;
extern uint16_t rf_link_show_time;
extern uint16_t no_act_time;

// RF state
extern uint8_t rf_blink_cnt;
extern uint8_t rf_sw_temp;
extern bool    f_uart_ack;
extern bool    f_send_channel;
extern bool    f_dial_sw_init_ok;
extern bool    f_rf_sw_press;
extern bool    f_rf_hand_ok;
extern bool    f_rf_read_data_ok;
extern bool    f_rf_sts_sysc_ok;
extern bool    f_rf_new_adv_ok;
extern bool    f_rf_reset;

// Sleep state
extern bool f_goto_sleep;
extern bool f_wakeup_prepare;

// UI state
extern bool f_bat_hold;
extern bool f_bat_num_show;
extern bool f_sys_show;
extern bool f_sleep_show;

// Report buffers (shared between rf.c and ansi.c)
extern uint8_t bitkb_report_buf[32];
extern uint8_t bytekb_report_buf[8];

// ---- Shared functions ----

// rf.c
void    rf_uart_init(void);
void    rf_device_init(void);
void    dev_sts_sync(void);
void    uart_receive_pro(void);
void    uart_send_report_func(void);
uint8_t uart_send_cmd(uint8_t cmd, uint8_t ack_cnt, uint8_t delayms);
void    uart_send_report(uint8_t report_type, uint8_t *report_buf, uint8_t report_size);
void    uart_send_report_keyboard(report_keyboard_t *report);
void    uart_send_report_nkro(report_nkro_t *report);
void    uart_send_mouse_report(report_mouse_t *report);
void    uart_send_consumer_report(report_extra_t *report);
void    uart_send_system_report(report_extra_t *report);

// ansi.c
void break_all_key(void);
void timer_pro(void);

// side.c
void side_led_show(void);
void side_speed_control(uint8_t dir);
void side_light_control(uint8_t dir);
void side_colour_control(uint8_t dir);
void side_mode_control(uint8_t dir);
void num_led_show(void);
void rgb_test_show(void);
void device_reset_show(void);
void device_reset_init(void);

// sleep.c
void sleep_handle(void);
