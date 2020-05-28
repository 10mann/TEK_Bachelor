#include <stdio.h>
#include "nrf_drv_twi.h"
#include "nrf_delay.h"
#include "log.h"

#ifndef HDC1080_H
#define HDC1080_H

#define HDC1080_ADDR 0x40
#define CONFIG_REG_HDC 0x02
#define SENSOR_DATA_REG 0x00   
#define SENSOR_DATA_LEN 4
#define EN_TEMP_HUM 0x10
#define TEMP_RES_11_BIT 0x04
#define HUM_RES_8_BIT 0x02


void error_check_HDC(uint32_t error, int identity);
void HDC1080_init();
uint16_t HDC_calc_temp(uint16_t raw_temp);
uint16_t HDC_calc_humid(uint16_t raw_humid);
void HDC1080_get_temp_hum(uint16_t* raw_data, double* calc_data);
void HDC_calculate_temp_hum(uint16_t* raw_data, double* calc_data);


#endif // HDC1080_H