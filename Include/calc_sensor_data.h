#ifndef CALC_SENSOR_DATA_H
#define CALC_SENSOR_DATA_H

#include <stdint.h>

#define LDR_CONSTANT 10000000000

struct calib_data
{
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;
    uint8_t dig_h1;
    int16_t dig_h2;
    uint8_t dig_h3;
    int16_t dig_h4;
    int16_t dig_h5;
    int8_t dig_h6;
    int32_t t_fine;
};

/*  Lux */
uint16_t calc_lux_level(uint16_t LDR_resistance);

/* BME280 */
uint16_t BME280_calculate_temp(uint16_t raw_temp_data);
uint16_t BME280_calculate_hum(uint16_t raw_hum_data);

/*  HDC1080 */
uint16_t HDC_calc_temp(uint16_t raw_temp);
uint16_t HDC_calc_humid(uint16_t raw_humid);

/*  SI7021  */
uint16_t si7021_calc_temp(uint16_t temp);
uint16_t si7021_calc_humid(uint16_t hum);

#endif