#include "HDC1080.h"

extern const nrf_drv_twi_t m_twi;

/* Initsialiserer HDC1080 */
void HDC1080_init()
{
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initializing HDC1080\n");
  uint8_t data[3];
 
  data[0] = CONFIG_REG_HDC;
  data[1] = 0x1A; //EN_TEMP_HUM | TEMP_RES_11_BIT | HUM_RES_8_BIT; //00011010
  data[2] = 0x00;
  
  uint32_t err = nrf_drv_twi_tx(&m_twi, HDC1080_ADDR, data, sizeof(data), false);
  if(err != NRF_SUCCESS)
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer, %d\n", err);
}

/* Henter temp og fuktighetsdata fra sensorens registere */
void HDC1080_get_temp_hum(uint16_t* raw_data, double* calc_data)
{
  uint8_t sensor_reg = SENSOR_DATA_REG;
  uint8_t raw_data_buff[4];

  uint32_t err = nrf_drv_twi_tx(&m_twi, HDC1080_ADDR, &sensor_reg, 1, false);
  if(err != NRF_SUCCESS)
   __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer, %d\n", err);
  
  nrf_delay_ms(10);

  err = nrf_drv_twi_rx(&m_twi, HDC1080_ADDR, raw_data_buff, sizeof(raw_data_buff));
  if(err != NRF_SUCCESS)
   __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer, %d\n", err);
  
  raw_data[0] =  ((uint16_t)raw_data_buff[0] << 8) | raw_data_buff[1];
  raw_data[1]  =  ((uint16_t)raw_data_buff[2] << 8) | raw_data_buff[3];

  HDC_calculate_temp_hum(raw_data, calc_data); 
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Temp: 0x%04x, %d, Humid: 0x%04x, %d\n", raw_data[0], (uint16_t)calc_data[0], raw_data[1], (uint16_t)calc_data[1]);
}

/* Regner om fra rå-data til tempratur i celscius og luftfuktughet i prosent*/
void HDC_calculate_temp_hum(uint16_t* raw_data, double* calc_data)
{
   calc_data[0] = ((double)raw_data[0] / 65536) * 165 - 40;
   calc_data[1]  = ((double)raw_data[1] / 65536) * 100;
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

