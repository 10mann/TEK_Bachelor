#include "twi_sensor.h"
#include "boards.h"
#include "log.h"
#include "nrf_delay.h"
#include "LPN_config.h"

#if TWI0_ENABLED
#define TWI_INSTANCE_ID     0
#endif

const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);


//  Setter opp TWI på pinne 21 og 23 med 100kHz klokkehastighet
void twi_init (void)
{
    ret_code_t err_code;
    const nrf_drv_twi_config_t twi_config = {
       .scl                = LPN_CLK_PIN,
       .sda                = LPN_DATA_PIN,
       .frequency          = NRF_DRV_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL /*twi_handler*/, NULL);
    if(err_code != NRF_SUCCESS)
    {
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Couldn't initialize TWI, Error %d\n", err_code);
    }
    else
    {
      nrf_drv_twi_enable(&m_twi);
      //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "TWI init\n");
    }
}

//  Iverksetter TWI
//  Det var et problem med strømtrekk for TWI
//  Fikset ved å tvinge den helt av, Errata 89
void twi_uninit()
{
  nrf_drv_twi_uninit(&m_twi);
  *(volatile uint32_t *)0x40003FFC = 0;
  *(volatile uint32_t *)0x40003FFC;
  *(volatile uint32_t *)0x40003FFC = 1;
   //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "TWI uninit\n");
}

void enable_sensors()
{
  nrf_gpio_pin_set(ENABLE_SENSOR);
}

void disable_sensors()
{
  nrf_gpio_pin_clear(ENABLE_SENSOR);
}