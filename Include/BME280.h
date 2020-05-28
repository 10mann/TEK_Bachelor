#include <stdint.h>

#ifndef BME280_H
#define BME280_H

#define BME280_ADDR 0x76
#define CTRL_HUM    0xF2
#define CTRL_MEAS   0xF4
#define CONFIG_REG_BME  0xF5
#define EN_TEMP     0x20 
#define EN_HUM      0x01
#define DIS_PRES    0x00
#define FORCE_MODE  0x01
#define FILTER_OFF  0x00
#define SENSORS_REG 0xF7
#define COMP_TEMPRATUR_PRESURE_REG 0x88
#define COMP_HUMIDITY_REG 0xE1


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

void error_check_BME(uint32_t error, int identity);
void BME280_init();
void BME280_get_temp_hum(uint16_t* raw_data, double* calc_data);
void get_compensation_values(struct calib_data *calib_data);
void BME280_read(uint8_t reg_addr, uint16_t len, uint8_t * data);
uint16_t BME280_calculate_temp(uint16_t raw_temp_data);
uint16_t BME280_calculate_hum(uint16_t raw_hum_data);
uint8_t BME280_get_ID();

#endif //BME280_H