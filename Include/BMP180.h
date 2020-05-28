#ifndef  BMP180_H
#define BMP180_H

#include <stdint.h>

#define BMP180_ADDR 0x77
#define BMP180_REG 0xF6
#define BMP180_CTRL_MEAS 0xF4
#define BMP180_PRESSURE 0x34
#define BMP180_TEMP 0x2E
#define BMP180_ID 0xD0
#define BMP180_CALIB_DATA 0xBF

void BMP180_init();
void BMP180_read(uint16_t* raw, double* calc);
uint16_t BMP180_calc_temp(uint16_t raw);
uint16_t BMP180_calc_pressure(uint16_t p, double t);

#endif
