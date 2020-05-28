#ifndef BAT_SAADC_H
#define BAT_SAADC_H

#include "LPN_config.h"
#include <stdint.h>
//#include "nrf_drv_saadc.h"

#if LPN != LPN_ID_3
#define SAADC_SAMPLES_IN_BUFFER 2
#else
#define SAADC_SAMPLES_IN_BUFFER 1
#endif
#define VOLTAGE_DIVIDER_R2 10000
#define AVERAGE_CYCLES 5
#define SAADC_CALIBRATION 60 //mV
#define SAADC_BUFFERS 2
#define LDR_CONSTANT 10000000000

#define BATT_100 2900
#define BATT_80_LOW 2850
#define BATT_50_LOW 2750
#define BATT_20_LOW 2500

uint32_t saadc_init();

void saadc_uninit();

uint16_t get_batt_level();
uint16_t get_LDR_data();
uint16_t calc_lux_level(uint16_t LDR_raw);
#endif