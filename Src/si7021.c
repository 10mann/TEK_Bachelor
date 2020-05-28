#include "si7021.h"

#include "nrf_drv_twi.h"
#include "log.h"

extern const nrf_drv_twi_t m_twi;

void si7021_init()
{
  uint8_t buffer[2] = {0xE6, 0xFB};
  nrf_drv_twi_tx(&m_twi, SI7021_ADDR, buffer, sizeof(buffer), false);
}

void si7021_get_temp_hum(uint16_t* raw, double* calc)
{
  uint8_t buffer[2] = {0xE5};
  uint8_t data[4];
  nrf_drv_twi_tx(&m_twi, SI7021_ADDR, buffer, 1, false);
  nrf_drv_twi_rx(&m_twi, SI7021_ADDR, data, 2);

  buffer[0] = 0xE0;
  nrf_drv_twi_tx(&m_twi, SI7021_ADDR, buffer, 1, false);
  nrf_drv_twi_rx(&m_twi, SI7021_ADDR, &data[2], 2);

  uint16_t temp_raw = (data[2] << 8) | data[3];
  uint16_t hum_raw = (data[0] << 8) | data[1];

  uint16_t temp_calc = si7021_calc_temp(temp_raw);
  uint16_t hum_calc = si7021_calc_humid(hum_raw);

  raw[0] = temp_raw;
  raw[1] = hum_raw;

  calc[0] = temp_calc;
  calc[1] = hum_calc;
}

/*  SI7021 temp formula found in SI7021 datasheet */
/*  SI7021 temp-formel funnet i databladet til SI7021 */
uint16_t si7021_calc_temp(uint16_t temp)
{
  if(temp == 0)
    return 0;

  //int16_t temp = (int16_t)temp_;
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