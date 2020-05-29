#ifndef twi_sensor_h__
#define twi_sensor_h__

#include "nrf_drv_twi.h"
#include "generic_onoff_client.h"

#define ENABLE_SENSOR NRF_GPIO_PIN_MAP(1, 9)

void find_twi_address();
void twi_init();
void twi_uninit();
void update_and_send_data();
void send_sensor_data_unack(generic_onoff_client_t*, uint8_t*, uint8_t,
                                        const model_transition_t*, uint8_t);
void enable_sensors();
void disable_sensors();

#endif
