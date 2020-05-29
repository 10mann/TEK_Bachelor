#ifndef CSS811_H
#define CSS811_H

#include <stdint.h>
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "app_timer.h"

#define CCS811_WAKE 19

#define CCS811_ADDR 0x5A

#define CCS811_STATUS 0x00
#define MEAS_MODE 0x01
#define ALG_RESULT_DATA 0x02
#define RAW_DATA 0x03
#define ENV_DATA 0x05
#define THRESHOLDS 0x10
#define BASELINE 0x11
#define HW_ID 0x20
#define HW_VERSION 0x21
#define FW_BOOT_VERSION 0x23
#define FW_APP_VERSION 0x24
#define INTERNAL_STATE 0xA0
#define ERROR_ID 0xE0
#define APP_START 0xF4
#define SW_RESET 0xFF

#define RESET_DELAY HAL_MS_TO_RTC_TICKS(2000)
#define BOOT_DELAY HAL_MS_TO_RTC_TICKS(1000)

void ccs811_init();
void ccs811_uninit();
void ccs811_boot();
void ccs811_start_co2();
void ccs811_read_ID();
void ccs811_read_status();
void ccs811_wake();
void ccs811_sleep();
uint16_t ccs811_get_co2();
uint16_t ccs811_get_calc_co2();
bool ccs811_read_co2();
void ccs811_callback(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

#endif