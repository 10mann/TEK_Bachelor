#ifndef SCREENS_H
#define SCREENS_H

#include "nrf_gfx.h"
#include "screen_config.h"
#include "app_timer.h"
#include "hal.h"

#define GRAPH_MAX_TEMP 35
#define GRAPH_MIN_TEMP 18
//#define GRAPH_TEMP_DIFF 5
#define GRAPH_TOP WSEPD_WIDTH - 15
#define GRAPH_BOTTOM 4
#define GRAPH_START 25
#define GRAPH_LINES 5
#define GRAPH_TIME_ON_SCREEN HAL_SECS_TO_RTC_TICKS(10)

// Punkter
#if EPAPER_MODULE1_54
#define BATT_PT_X 5
#define BATT_PT_Y 174
#define BT_PT_X 5
#define BT_PT_Y 148
#define TIME_PT_X 5
#define TIME_PT_Y 5
#define TEMP_PT_X 128
#define TEMP_PT_Y 130
#define HUMID_PT_X 54
#define HUMID_PT_Y 124
#define LIGHT_PT_X 180
#define LIGHT_PT_Y 188
#define LIGHT_LEVEL_PT_Y 0
#define LIGHT_LEVEL_PT_X 178

#else
#define BATT_PT_X  5
#define BATT_PT_Y 270
#define BT_PT_X 5
#define BT_PT_Y 249
#define TIME_PT_X 5
#define TIME_PT_Y 5
#define TEMP_PT_X 58
#define TEMP_PT_Y 118
#define HUMID_PT_X 50
#define HUMID_PT_Y 225
#define LIGHT_PT_X 108
#define LIGHT_PT_Y 280
#define LIGHT_LEVEL_PT_Y 0
#define LIGHT_LEVEL_PT_X 106
#endif

// Batteriverdier
#define BATT_LEVEL_100 0
#define BATT_LEVEL_80 1
#define BATT_LEVEL_50 2
#define BATT_LEVEL_20 3
#define BATT_LEVEL_0 4

// Ikoner
#define BATT_ICON_WIDTH 21
#define BATT_ICON_HEIGHT 19
#define BT_ICON_WIDTH 21
#define BT_ICON_HEIGHT 19
#define TIME_ICON_WIDTH 21
#define TIME_ICON_HEIGHT 23
#define LIGHT_ICON_HEIGHT 21
#define LIGHT_ICON_WIDTH 21

void draw_background();
void draw_background_external(uint16_t lpn_id);

uint16_t string_to_pixel_length(const nrf_gfx_font_desc_t* p_font, uint8_t* string);

void write_sensor_data(uint8_t lpn_id);

void print_lpn_id();
void print_external_lpn_id(uint16_t lpn_id);

void LPN_print_temp(uint16_t temp);
void LPN_print_humid(uint16_t humid);
void LPN_print_lux(uint16_t lux);

bool screen_change(uint8_t lpn_id);
void draw_temp_graph();
void draw_battery();
void draw_bt_icon();
void draw_time_interval();
void draw_light_level();

uint8_t get_LPN_interval_mode();

void screens_init();

#endif





