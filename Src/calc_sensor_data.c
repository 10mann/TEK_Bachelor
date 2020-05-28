#include "calc_sensor_data.h"
#include <math.h>

struct calib_data calibration = 
{.dig_t1 = 28326, .dig_t2 = 26604, .dig_t3 = 12850, 
.dig_h1 = 75, .dig_h2 = 383, .dig_h3 = 0, 
.dig_h4 = 271, .dig_h5 = 50, .dig_h6 = 30};

/*  Formula for calculating lux, derived from Excel sheet   */
/*  Formel for å beregne lux, funnet ved regresjon via Excel  */
uint16_t calc_lux_level(uint16_t LDR_resistance)
{
  long double lux = (long double)LDR_resistance; 
  lux = pow(lux, -2.1) * LDR_CONSTANT;

  return (uint16_t)lux;
}

/*  BME280 temp formula found in BME280 datasheet  */
/*  BME280 temp-formel, funnet i databladet til BME280  */
uint16_t BME280_calculate_temp(uint16_t raw_temp_data)
{
  if(raw_temp_data == 0)
    return 0;

  double var1;
  double var2;
  double temperature;
  var1 = ((double)raw_temp_data / 1024.0) - ((double)calibration.dig_t1) / 1024.0;
  var1 = (var1 * (double)calibration.dig_t2) / 16.0;
  var2 = (((double)raw_temp_data) / 8192.0) - (((double)calibration.dig_t1) / 8192.0);
  var2 = (var2 * var2 * (double)calibration.dig_t3) / 16.0;
  calibration.t_fine = (int32_t)(var1 + var2);
  temperature = ((var1 + var2) / 320.0);

  return (uint16_t)temperature;
}

/*  BME280 humidity formula found in BME280 datasheet */
/*  BME280 fuktighet-formel funnet i databladet til BME280  */
uint16_t BME280_calculate_hum(uint16_t raw_hum_data)
{
  if(raw_hum_data == 0)
    return 0;

  double humidity;
  double humidity_min = 0.0;
  double humidity_max = 100.0;
  double var1;
  double var2;
  double var3;
  double var4;
  double var5;
  double var6;

  var1 = ((double)calibration.t_fine) - 4800.0; //76800
  var2 = (((double)calibration.dig_h4) * 64.0 + (((double)calibration.dig_h5) / 16384.0) * var1);
  var3 = raw_hum_data - var2;
  var4 = ((double)calibration.dig_h2) / 65536.0;
  var5 = (1.0 + (((double)calibration.dig_h3) / 67108864.0) * var1);
  var6 = 1.0 + (((double)calibration.dig_h6) / 67108864.0) * var1 * var5;
  var6 = var3 * var4 * (var5 * var6);

  humidity = var6 * (1.0 - ((double)calibration.dig_h1) * var6 / 524288.0);

  if(humidity > humidity_max)
  {
    humidity = humidity_max;
  }
  else if(humidity < humidity_min)
  {
    humidity = humidity_min;
  }

  return (uint16_t)humidity;
}

/*  HDC1080 temp formula found in HDC1080 datasheet */
/*  HDC1080 temp-formel funnet i databladet til HDC1080 */
uint16_t HDC_calc_temp(uint16_t raw_temp)
{
  if(raw_temp == 0)
    return 0;

  int16_t calc_temp = (double)raw_temp / 65536 * 165 - 40;
  return (uint16_t)calc_temp;
}

/*  HDC1080 humidity formula found in HDC1080 datasheet */
/*  HDC1080 fuktighet-formel funnet i databladet til HDC1080 */
uint16_t HDC_calc_humid(uint16_t raw_humid)
{
  if(raw_humid == 0)
    return 0;

  int16_t calc_humid = (double)raw_humid / 65536 * 100;

  if(calc_humid > 100)
    calc_humid = 100;
  else if(calc_humid < 0)
    calc_humid = 0;

  return (uint16_t)calc_humid;
}

/*  SI7021 temp formula found in SI7021 datasheet */
/*  SI7021 temp-formel funnet i databladet til SI7021 */
uint16_t si7021_calc_temp(uint16_t temp)
{
  if(temp == 0)
    return 0;

  temp *= 0.00268;
  temp -= 47;
  
  return (uint16_t)temp;
}

/*  SI7021 humidity formula found in SI7021 datasheet */
/*  SI7021 fuktighet-formel funnet i databladet til SI7021 */
uint16_t si7021_calc_humid(uint16_t hum)
{
  if(hum == 0)
    return 0;

  hum *= 0.00191;
  hum -= 6;

  if(hum > 100)
    hum = 100;
  else if(hum < 0)
    hum = 0;

  return (uint16_t)hum;
}