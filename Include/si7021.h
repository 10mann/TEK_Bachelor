#ifndef SI7021_H
#define SI7021_H

#include <stdint.h>

#define SI7021_ADDR 0x40

void si7021_init();

void si7021_get_temp_hum(uint16_t* raw, double* calc);

uint16_t si7021_calc_temp(uint16_t temp);

uint16_t si7021_calc_humid(uint16_t hum);

#endif