#include "BME280.h"

#include <stdio.h>

#include "nrf_drv_twi.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "log.h"
#include "nrf_delay.h"


struct calib_data calibration = 
{.dig_t1 = 28326, .dig_t2 = 26604, .dig_t3 = 12850, 
.dig_h1 = 75, .dig_h2 = 383, .dig_h3 = 0, 
.dig_h4 = 271, .dig_h5 = 50, .dig_h6 = 30};

extern const nrf_drv_twi_t m_twi;

/* Initsialiserer BME280 */
void BME280_init()//nrf_drv_twi_t const * twi)
{
  uint8_t data[2];
 
  data[0] = CONFIG_REG_BME;
  data[1] = FILTER_OFF;
  
  uint32_t err = nrf_drv_twi_tx(&m_twi, BME280_ADDR, data, sizeof(data), false); // Turns off IIR-filter
   if(err != NRF_SUCCESS)
    error_check_BME(err, 1);

  //get_compensation_values(&calibration);

}
/* Henter temp og fuktighetsdata fra sensorens registere */
void BME280_get_temp_hum(uint16_t* raw_data, double* calc_data)
{
  uint8_t data[2];
  uint16_t lsb;
  uint16_t xlsb;
  uint16_t msb;
  uint16_t lsb_16;
  uint16_t msb_16;
  
  uint8_t raw_data_buff[8] = {0};
  uint8_t sensor_reg = SENSORS_REG;

  uint32_t err;

  data[0] = CTRL_HUM;
  data[1] = 0x03; //EN_HUM;
  
  err = nrf_drv_twi_tx(&m_twi, BME280_ADDR, data, sizeof(data), false); // Enables humidity mesurments
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with humid transfer %d\n", err);
  }
   
  data[0] = CTRL_MEAS;
  data[1] = 0x61; //EN_TEMP | DIS_PRES | FORCE_MODE 0b01100001 
  for(uint8_t i = 0; i < 2; i++)
  {
    err = nrf_drv_twi_tx(&m_twi, BME280_ADDR, data, sizeof(data), false); 
    if(err != NRF_SUCCESS)
    {
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with init transfer %d\n", err);
    }
    nrf_delay_ms(10);
    BME280_read(sensor_reg, sizeof(raw_data_buff), raw_data_buff);
  }

  msb  = (uint32_t)raw_data_buff[3] << 8;
  lsb  = (uint32_t)raw_data_buff[4] << 0;
  xlsb = (uint32_t)raw_data_buff[5] >> 8;
  raw_data[0] = msb | lsb | xlsb;

  msb = (uint32_t)raw_data_buff[6] << 8;
  lsb = (uint32_t)raw_data_buff[7];
  raw_data[1] = msb | lsb;

  calc_data[0] = BME280_calculate_temp(raw_data[0]);
  calc_data[1] = BME280_calculate_hum(raw_data[1]);
}

/* Henter kaliberingsverdier fra sensorens register og lagrer de i en struct */
void get_compensation_values(struct calib_data *calib_data)
{
  uint8_t comp_temp_pres[26];
  uint8_t comp_hum[7];
  uint8_t reg_addr = COMP_TEMPRATUR_PRESURE_REG;
  
  int16_t dig_h4_lsb;
  int16_t dig_h4_msb;
  int16_t dig_h5_lsb;
  int16_t dig_h5_msb;

  BME280_read(reg_addr, sizeof(comp_temp_pres), comp_temp_pres);

  reg_addr = COMP_HUMIDITY_REG;
  
  BME280_read(reg_addr, sizeof(comp_hum), comp_hum);

  calib_data->dig_t1 = (((uint16_t)comp_temp_pres[1] << 8) | (uint16_t)comp_temp_pres[0]);
  calib_data->dig_t2 = (((uint16_t)comp_temp_pres[3] << 8) | (uint16_t)comp_temp_pres[2]);
  calib_data->dig_t3 = (((uint16_t)comp_temp_pres[4] << 8) | (uint16_t)comp_temp_pres[4]);
  

  calib_data->dig_h1 = comp_temp_pres[25];
  calib_data->dig_h2 = (((int16_t)comp_hum[1] << 8) | (int16_t)comp_hum[0]);
  calib_data->dig_h3 = comp_hum[2];

  dig_h4_msb = (int16_t)(int8_t)comp_hum[3] * 16;
  dig_h4_lsb = (int16_t)(comp_hum[4] & 0x0F);
  calib_data->dig_h4 = dig_h4_msb | dig_h4_lsb;

  dig_h5_msb = (int16_t)(int8_t)comp_hum[5] * 16;
  dig_h5_lsb = (int16_t)(comp_hum[4] >> 4);
  calib_data->dig_h5 = dig_h5_msb | dig_h5_lsb;

  calib_data->dig_h6 = (int8_t)comp_hum[6];
/*
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\ndig_t1: %d\ndig_t2: %d\ndig_t3: %d\ndig_h1: %d\ndig_h2: %d\ndig_h3: %d\ndig_h4: %d\ndig_h_5: %d\ndig_h6: %d\n", 
                                      calib_data->dig_t1, calib_data->dig_t2, calib_data->dig_t3, calib_data->dig_h1, calib_data->dig_h2, calib_data->dig_h3, calib_data->dig_h4, calib_data->dig_h5, calib_data->dig_h6);
*/
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

  var1 = ((double)calibration.t_fine) - 4800.0; 
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

/* Funksjon for lesing av register */
void BME280_read(uint8_t reg_addr, uint16_t len, uint8_t * data)
{
  uint32_t err;
  err = nrf_drv_twi_tx(&m_twi, BME280_ADDR, &reg_addr, 1, false);
  if(err != NRF_SUCCESS)
  {
     __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

  err = nrf_drv_twi_rx(&m_twi, BME280_ADDR, data, len);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  
  uint8_t buffer[2];
  buffer[0] = CTRL_HUM;
  buffer[1] = 0x00;

  err = nrf_drv_twi_tx(&m_twi, BME280_ADDR, buffer, sizeof(buffer), false);
  if(err != NRF_SUCCESS)
  {
     __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

  buffer[0] = CTRL_MEAS;
  err = nrf_drv_twi_tx(&m_twi, BME280_ADDR, buffer, sizeof(buffer), false);
  if(err != NRF_SUCCESS)
  {
     __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
}

/* Error sjekk ved I2C overføring*/
void error_check_BME(uint32_t error, int identity)
{
    //NRF_LOG_INFO("BME_error%i: %d\n", identity, error);
    //NRF_LOG_FLUSH();
}

uint8_t BME280_get_ID()
{
  uint8_t data[2];
  data[0] = CTRL_MEAS;
  data[1] = EN_TEMP | DIS_PRES | FORCE_MODE;

  uint32_t err = nrf_drv_twi_tx(&m_twi, BME280_ADDR, data, sizeof(data), false); // Enables temprature mesurments, disables presure messurments and set forced mode
  if(err != NRF_SUCCESS)
  error_check_BME(err, 3);

  data[0] = 0xD0;
  nrf_drv_twi_tx(&m_twi, BME280_ADDR, data, 1, false);

  uint8_t id = 0;
  nrf_drv_twi_rx(&m_twi, BME280_ADDR, &id, 1);

  return id;
}