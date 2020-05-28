#include <stdio.h>
#include "nrf_drv_twi.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"
#include "log.h"

#include "TSL2581.h"

extern const nrf_drv_twi_t m_twi;

uint8_t TSL_data[4] = {1, 1, 1, 1};
uint8_t data[2];
float Ch0;
float Ch1;

/* Initsialiserer TSL2581 */
void TSL2581_init()
{
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Starting TSL2581\n");
  data[0] = CMD | CTRL_REG;
  data[1] = POWER_ON;

  uint32_t err;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  
  nrf_delay_ms(2);

  data[0] = CMD | ALS_REG;
  data[1] = ITIME;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

  data[0] = CMD| AN_REG;
  data[1] = GAIN_1X;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

  data[0] = CMD | CTRL_REG;
  data[1] = ADC_EN | POWER_ON;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
   
  data[0] = CTRL_REG; //CMD | CTRL_REG;
  data[1] = ADC_EN | POWER_OFF;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

  data[0] = CTRL_REG; //CMD | CTRL_REG;
  data[1] = POWER_OFF;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

}

/* Henter rå-data fra sensor */
void TSL2581_read(uint16_t* calc_lux)
{

  data[0] = CMD | CTRL_REG;
  data[1] = POWER_ON;

  uint32_t err;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  
  nrf_delay_ms(2);

  data[0] = CMD | CTRL_REG;
  data[1] = ADC_EN | POWER_ON;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

  nrf_delay_ms(10);

  data[0] = WORD_BIT;
  data[1] = CH0_REG;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

  err = nrf_drv_twi_rx(&m_twi, TSL2581_ADDR, TSL_data, sizeof(TSL_data));
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

  data[0] = CTRL_REG; //CMD | CTRL_REG;
  data[1] = ADC_EN | POWER_OFF;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

  data[0] = CTRL_REG; //CMD | CTRL_REG;
  data[1] = POWER_OFF;
  err = nrf_drv_twi_tx(&m_twi, TSL2581_ADDR, data, sizeof(data), true);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }

  Ch0 = (TSL_data[1] << 8) | TSL_data[0];
  Ch1 = (TSL_data[3] << 8) | TSL_data[2];
  uint8_t index = 0;

  *calc_lux = calculateLux();
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Lux: %d\n", *calc_lux);
}

/* Regner om fra rå-data til lux */
uint16_t calculateLux()
{
  uint16_t lux = 0;
  uint16_t ch_data;
  if(Ch0 > 0)
    ch_data = Ch1 / Ch0;
  else
    return 0;

  if(ch_data >= 0.54)
  {
    lux = 0;
  }

  else if((0.45 <= ch_data) && (ch_data < 0.54))
  {
    lux = (0.062*Ch0 - 0.0100*Ch1);
  }

  else if((0.38 <= ch_data) && (ch_data < 0.45))
  {
    lux = (0.0974*Ch0 - 0.1786*Ch1);
  }

  else if((0.30 <= ch_data) && (ch_data < 0.38))
  {
    lux = (0.1649*Ch0 - 0.3562*Ch1);
  }

  else if((0 <= ch_data) && (ch_data < 0.30))
  {
    lux = (0.13*Ch0 - 0.24*Ch1);
  }

  return (lux);
}


