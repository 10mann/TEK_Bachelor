#include "CCS811.h"
#include "nrf_drv_twi.h"
#include "log.h"
#include "twi_sensor.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "LPN1.h"

APP_TIMER_DEF(ccs811_timer);

extern const nrf_drv_twi_t m_twi;
static uint16_t raw = 0;
static uint16_t calc = 0;
static uint8_t sw_reset[5] = {0xFF, 0x11, 0xE5, 0x72, 0x8A};
static uint8_t phase = 0;

void ccs811_uninit()
{
  uint32_t err;
  uint8_t buffer[2] = {MEAS_MODE, 0x00};

  err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, buffer, 2, false);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
}

void ccs811_init()
{
  app_timer_create(&ccs811_timer, APP_TIMER_MODE_SINGLE_SHOT, ccs811_boot);

  uint32_t err;
  err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, sw_reset, sizeof(sw_reset), false);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "SW reset failed\n");
  }

  app_timer_start(ccs811_timer, RESET_DELAY, NULL);
}

void ccs811_boot()
{
  uint32_t err;

  if(phase == 0)
  {
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "CCS811 bootup\n");
    ccs811_read_status();
    uint8_t buffer = 0xF4;
    err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, &buffer, 1, false);
    if(err != NRF_SUCCESS)
    {
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Bootup failed\n");
    }
    phase++;
    app_timer_start(ccs811_timer, BOOT_DELAY, NULL);
  }
  else
  {
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Starting CO2\n");
    ccs811_read_status();
    //ccs811_start_co2();
    phase = 0;
    twi_uninit();
  }
}

void ccs811_start_co2()
{
  uint32_t err;
  uint8_t buffer[2] = {MEAS_MODE, 0x48};

  err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, buffer, 2, false);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
/*
  else
  {
    err = nrf_drv_twi_rx(&m_twi, CCS811_ADDR, buffer, 1);
    if(err == NRF_SUCCESS)
    {
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "CO2 started, mode 0x%02x\n", buffer[0]);
    }
    else
    {
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error reading MEAS_register %d\n", err);
    }
  }
*/
}

void ccs811_wake()
{
  nrf_gpio_pin_clear(CCS811_WAKE);
}

void ccs811_sleep()
{
  nrf_gpio_pin_set(CCS811_WAKE);
}

bool ccs811_read_co2()
{
  uint32_t err;
  bool done = false;
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Reading CO2\n");

  uint8_t buffer[2] = {ALG_RESULT_DATA};
  err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, buffer, 1, false);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  err = nrf_drv_twi_rx(&m_twi, CCS811_ADDR, buffer, 2);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  calc = (buffer[0] << 8) | buffer[1];

  buffer[0] = RAW_DATA;
  err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, buffer, 1, false);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  err = nrf_drv_twi_rx(&m_twi, CCS811_ADDR, buffer, 2);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  raw = (buffer[0] << 8) | buffer[1];
  
  if(calc > 0)
  {
    done = true;
    buffer[0] = MEAS_MODE;
    buffer[1] = 0x00;
    err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, buffer, 2, false);
    if(err != NRF_SUCCESS)
    {
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
    }
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "CO2: %d, 0x%04x\n", calc, raw);
  }
  //ccs811_sleep();
  return done;
}

void ccs811_read_ID()
{
  uint32_t err;
  uint8_t buffer = HW_ID;
  err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, &buffer, 1, false);
  if(err != NRF_SUCCESS)
  { 
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  err = nrf_drv_twi_rx(&m_twi, CCS811_ADDR, &buffer, 1);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Device ID: 0x%02x\n", buffer);
}

void ccs811_read_meas_mode()
{
  uint32_t err;
  uint8_t buffer = MEAS_MODE;
  err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, &buffer, 1, false);
  if(err != NRF_SUCCESS)
  { 
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  err = nrf_drv_twi_rx(&m_twi, CCS811_ADDR, &buffer, 1);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Meas mode: 0x%02x\n", buffer);
}

void ccs811_read_status()
{
  uint32_t err;
  uint8_t error = ERROR_ID;
  uint8_t status = CCS811_STATUS;
  err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, &status, 1, false);
  if(err != NRF_SUCCESS)
  { 
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  err = nrf_drv_twi_rx(&m_twi, CCS811_ADDR, &status, 1);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
  }
  else
  {
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Status: 0x%02x\n", status);
    if(status & 0x1)
    {
      err = nrf_drv_twi_tx(&m_twi, CCS811_ADDR, &error, 1, false);
      if(err != NRF_SUCCESS)
      {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
      }
      err = nrf_drv_twi_rx(&m_twi, CCS811_ADDR, &error, 1);
      if(err != NRF_SUCCESS)
      {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
      }
      else
      {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error 0x%02x\n", error);
      }
    }
  }
}

uint16_t ccs811_get_co2()
{
  return calc;
}

uint16_t ccs811_get_calc_co2()
{
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Returning %d\n", calc);
  return (uint16_t)calc;
}

void ccs811_callback(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{ 
  if(action == NRF_GPIOTE_POLARITY_HITOLO)
  {
    //ccs811_read_status();
    //ccs811_read_meas_mode();
    //ccs811_read_ID();
    if(ccs811_read_co2())
    {
      ccs811_read_status();
      twi_uninit();
      update_data(false);
    }
  }
}