#include "BMP180.h"

#include "nrf_drv_twi.h"
#include "twi_sensor.h"
#include "log.h"
#include "nrf_delay.h"
#include <math.h>

extern const nrf_drv_twi_t m_twi;

int16_t AC1, AC2, AC3, VB1, VB2, MB, MC, MD;
uint16_t AC4, AC5, AC6;
double c5, c6, mc, md, x0, x1, x2, y0_, y1_, y2, p0, p1, p2;


void BMP180_init()
{
  double c3, c4, b1;
  uint8_t reg = 0xAA;
  uint8_t data[2];
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  AC1 = (data[0] << 8) | data[1];

  reg = 0xAC;
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  AC2 = (data[1] << 8) | data[0];

  reg = 0xAE;
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  AC3 = (data[1] << 8) | data[0];

  reg = 0xB0;
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  AC4 = (data[1] << 8) | data[0];

  reg = 0xB2;
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  AC5 = (data[1] << 8) | data[0];

  reg = 0xB4;
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  AC6 = (data[1] << 8) | data[0];

  reg = 0xB6;
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  VB1 = (data[1] << 8) | data[0];

  reg = 0xB8;
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  VB2 = (data[1] << 8) | data[0];

  reg = 0xBA;
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  MB = (data[1] << 8) | data[0];

  reg = 0xBC;
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  MC = (data[1] << 8) | data[0];

  reg = 0xBE;
  nrf_drv_twi_tx(&m_twi, BMP180_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(&m_twi, BMP180_ADDR, data, 2);
  MD = (data[1] << 8) | data[0];
/*
  c3 = 160.0 * pow(2, -15) * AC3;
  c4 = pow(10, -3) * pow(2, -15) * AC4;
  b1 = pow(160, 2) * pow(2, -30) * VB1;
  c5 = (pow(2, -15) / 160.0) * AC5;
  c6 = AC6;
  mc = (pow(2, 11) / pow(160, 2)) * MC;
  md = MD / 160.0;
  x0 = AC1;
  x1 = 160.0 * pow(2, -13) * AC2;
  x2 = pow(160, 2) * pow(2, -25) * VB2;
  y0_ = c4 * pow(2, 15);
  y1_ = c4 * c3;
  y2 = c4 * b1;
  p0 = (3791.0 - 8.0) / 1600.0;
  p1 = 1 - 7357.0 * pow(2, -20);
  p2 = 3038.0 * 100.0 * pow(2, -36);
*/
}

void BMP180_read(uint16_t* raw, double* calc)
{
  uint32_t err;

  uint8_t buffer[2] = {BMP180_CTRL_MEAS, BMP180_TEMP};
  err = nrf_drv_twi_tx(&m_twi, BMP180_ADDR, buffer, sizeof(buffer), false);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
    return;
  }
  
  nrf_delay_ms(5);
  buffer[0] = BMP180_REG;
  err = nrf_drv_twi_tx(&m_twi, BMP180_ADDR, buffer, 1, false);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
    return;
  }
  err = nrf_drv_twi_rx(&m_twi, BMP180_ADDR, buffer, 2);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
    return;
  }
  raw[0] = (buffer[0] << 8) | buffer[1];
  calc[0] = BMP180_calc_temp(raw[0]);
  
  buffer[0] = BMP180_CTRL_MEAS;
  buffer[1] = BMP180_PRESSURE;
  err = nrf_drv_twi_tx(&m_twi, BMP180_ADDR, buffer, sizeof(buffer), false);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
    return;
  }
  nrf_delay_ms(5);

  buffer[0] = BMP180_REG;
  err = nrf_drv_twi_tx(&m_twi, BMP180_ADDR, buffer, 1, false);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
    return;
  }
  err = nrf_drv_twi_rx(&m_twi, BMP180_ADDR, buffer, 2);
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error with transfer %d\n", err);
    return;
  }
  raw[1] = (buffer[0] << 8) | buffer[1]; 
  calc[1] = BMP180_calc_pressure(raw[1], calc[0]);
}

uint16_t BMP180_calc_temp(uint16_t raw)
{
  double a, tu, temp;
  tu = (double)raw;
  x1 = (tu - AC6) * AC5 / pow(2, 15);
  x2 = MC * pow(2, 11) / (x1 + MD);
  c5 = x1 + x2;
  temp = (c5 + 8) / 16.0;
/*
  a = c5 * (tu - c6);
  temp = a + (mc / (a + md));
*/

  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Temp: %d, 0x%04x, %d\n", (uint16_t)temp, raw, raw);
  return (uint16_t)temp;
}

uint16_t BMP180_calc_pressure(uint16_t raw, double temp)
{
  double pu, B3, B4, B7, x, y, z, pressure, T, p;
  pressure = (double)raw;
  c6 = c5 - 4000.0;
  x0 = (VB2 * (c6 * c6 / pow(2, 12))) / pow(2, 11);
  x1 = AC2 * c6 / pow(2, 11);
  x2 = x0 + x1;
  B3 = (((AC1 * 4  + x2) + 2) / 4);
  x0 = AC3 * c6 / pow(2, 13);
  x1 = (VB1 * (c6 * c6 / pow(2, 12))) / pow(2, 16);
  x2 = ((x0 + x1) + 2) / 4.0;
  B4 = AC4 * (unsigned long)(x2 + 32768) / pow(2, 15);
  B7 = (pressure - B3) * 50000;
  if(B7 < 0x80000000)
  {
    p = B7 * 2 / B4;
  }
  else
  {
    p = B7 / B4 * 2;
  }
  
  x0 = (p / pow(2, 8)) * (p / pow(2, 8));
  x0 = (x0 * 3038) / pow(2, 16);
  x1 = (-7357 * p) / pow(2, 16);
  p = p + (x0 + x1 + 3791) / pow(2, 4);

  pressure = p;
/*
  s = T - 25;
  x = (x2 * pow(s, 2)) + (x1 * s) + x0;
  y = (y2 * pow(s, 2)) + (y1_ * s) + y0_;
  z = (pressure - x) / y;
  pressure = (p2 * pow(z, 2)) + (p1 * z) + p0;
*/
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Pressure: %d, %d\n", (uint16_t)pressure, raw);
  return (uint16_t)pressure;
}