#include "bat_saadc.h"
//#include "battery_saadc.h"
#include "nrf_drv_saadc.h"
#include "log.h"
#include "screens.h"
#include <math.h>

static nrf_saadc_value_t saadc_buffer[SAADC_BUFFERS][SAADC_SAMPLES_IN_BUFFER];
uint16_t batt_voltage = 0;
uint16_t LDR_voltage = 0;
uint8_t avr_index = 0;
volatile bool saadc_done = false;

//  Callback-funksjon som kjøres hver gang ADC har fylt opp tildelt buffer
//  Kaller nrf_drv_saadc_sample-funkjsonen til X antall målinger er tatt, og tar deretter gjennomsnittet av målingene
//  Etter det, blir det gitt et nytt buffer til ADC-en
void saadc_callback(nrf_drv_saadc_evt_t const* p_event)
{
  if(p_event->type == NRF_DRV_SAADC_EVT_DONE)
  {
    if(avr_index == 0)
    {
      batt_voltage = 0;
      LDR_voltage = 0;
    }
    if(avr_index > 1)
    {
      //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Index: %d, Battery voltage: %d, LDR voltage: %d\n", avr_index - 1, (uint16_t)(p_event->data.done.p_buffer[0] * 0.88), (uint16_t)(p_event->data.done.p_buffer[1] * 0.88));
      batt_voltage += p_event->data.done.p_buffer[0];// + 150;//0.88;
#if LPN != LPN_ID_3
      LDR_voltage += p_event->data.done.p_buffer[1];// + 150;
#endif
    }

    if(avr_index++ == AVERAGE_CYCLES + 1)
    {
      avr_index = 0;
      // 0.9 = Referanse * 1000 / (Gain * 2^(oppløsning)) 
      batt_voltage = batt_voltage * 0.9;
      batt_voltage /= AVERAGE_CYCLES;
      batt_voltage += SAADC_CALIBRATION;
#if LPN != LPN_ID_3
      LDR_voltage = LDR_voltage * 0.9;
      LDR_voltage /= AVERAGE_CYCLES;
      LDR_voltage += SAADC_CALIBRATION;
#endif
      //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nAverage\nBattery voltage: %d, LDR voltage: %d\nLight level: %d\n", batt_voltage, LDR_voltage, calc_lux_level(LDR_voltage));
    }
    else
    {
      nrf_drv_saadc_sample();
    }
    
    nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAADC_SAMPLES_IN_BUFFER);
    saadc_done = true;
  }
}

//  Setter opp ADC-en, én kanal hvis det er LPN 3 som skal brukes, ellers blir det satt opp to kanaler
//  én for å måle batterit og én for å måle LDR-spenning
//  Det er brukt 12-bit oppløsning, 40µs tilvendingstid, en gain på 1/6 og en intern referansespenning på 0,6V
uint32_t saadc_init()
{
  uint32_t err;
  nrf_drv_saadc_config_t saadc_config;
  nrf_saadc_channel_config_t saadc_channel_config;

  saadc_config.resolution = NRF_SAADC_RESOLUTION_12BIT;
  saadc_config.oversample = NRF_SAADC_OVERSAMPLE_DISABLED;
  saadc_config.interrupt_priority = APP_IRQ_PRIORITY_MID;
  saadc_config.low_power_mode = true;

  err = nrf_drv_saadc_init(&saadc_config, saadc_callback);
  if(err != NRF_SUCCESS)
    return err;

  saadc_channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;
  saadc_channel_config.gain = NRF_SAADC_GAIN1_6;
  saadc_channel_config.acq_time = NRF_SAADC_ACQTIME_40US;
  saadc_channel_config.mode = NRF_SAADC_MODE_SINGLE_ENDED;
  saadc_channel_config.pin_p = NRF_SAADC_INPUT_VDD;
  saadc_channel_config.pin_n = NRF_SAADC_INPUT_DISABLED;
  saadc_channel_config.resistor_p = NRF_SAADC_RESISTOR_DISABLED;
  saadc_channel_config.resistor_n = NRF_SAADC_RESISTOR_DISABLED;

  err = nrf_drv_saadc_channel_init(0, &saadc_channel_config);
  if(err != NRF_SUCCESS)
    return err;
#if LPN != LPN_ID_3
  saadc_channel_config.pin_p = NRF_SAADC_INPUT_AIN0;
  err = nrf_drv_saadc_channel_init(1, &saadc_channel_config);
  if(err != NRF_SUCCESS)
    return err;
#endif

  for(uint8_t i = 0; i < SAADC_BUFFERS; i++)
  {
    err = nrf_drv_saadc_buffer_convert(saadc_buffer[i], SAADC_SAMPLES_IN_BUFFER);
    if(err != NRF_SUCCESS)
      return err;
  }
}

//  Returnerer en verdi for å bedømme resterende batteri-prosent
uint16_t get_batt_level()
{
  if(batt_voltage >= BATT_100)
  {
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Battery percentage 100\n");
    return BATT_LEVEL_100;
  }
  else if((batt_voltage >= BATT_80_LOW) && (batt_voltage < BATT_100))
  {
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Battery percentage 80\n");
    return BATT_LEVEL_80;
  }
  else if((batt_voltage >= BATT_50_LOW) && (batt_voltage < BATT_80_LOW))
  {
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Battery percentage 50\n");
    return BATT_LEVEL_50;
  }
  else if((batt_voltage >= BATT_20_LOW) && (batt_voltage < BATT_50_LOW))
  { 
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Battery percentage 20\n");
    return BATT_LEVEL_20;
  }
  else
  {
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Battery percentage 0\n");
    return BATT_LEVEL_0;
  }
}

/*  Formula for calculating lux, derived from Excel sheet   */
/*  Formel for å beregne lux, funnet ved regresjon via Excel  */
uint16_t calc_lux_level(uint16_t LDR_resistance)
{
  long double lux = (long double)LDR_resistance; 
  lux = pow(lux, -2.1) * LDR_CONSTANT;

  return (uint16_t)lux;
}

//  Returnerer siste måling for LDR-spenning
uint16_t get_LDR_data()
{
  long double resistance = VOLTAGE_DIVIDER_R2 * (batt_voltage - LDR_voltage) / LDR_voltage;
  return (uint16_t)resistance;
}

//  Skrur av ADC mens noden er i dvale for å spare strøm
void saadc_uninit()
{
  nrf_drv_saadc_uninit();
}