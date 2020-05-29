/* Copyright (c) 2010 - 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>

/* HAL */
#include "boards.h"
#include "simple_hal.h"
#include "app_timer.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"

/* Core */
#include "nrf_mesh_configure.h"
#include "nrf_mesh.h"
#include "mesh_stack.h"
#include "device_state_manager.h"
#include "access_config.h"
#include "proxy.h"

/* LPN */
#include "mesh_lpn.h"
#include "mesh_friendship_types.h"

/* Provisioning and configuration */
#include "mesh_provisionee.h"
#include "mesh_app_utils.h"

/* Models */
#include "generic_onoff_client.h"
#include "simple_sensor_client.h"
#include "simple_sensor_server.h"
#include "simple_sensor_setup_server.h"
//#include "generic_sensor_server.h"
//#include "generic_sensor_common.h"
//#include "generic_sensor_client.h"

/* Logging and RTT */
#include "log.h"
#include "rtt_input.h"

/* Example specific includes */
#include "app_config.h"
#include "nrf_mesh_config_examples.h"
#include "example_common.h"
#include "ble_softdevice_support.h"
#include "ble_dfu_support.h"

/* nRF5 SDK */
#include "nrf_soc.h"
#include "nrf_pwr_mgmt.h"

/*  Includes  */
//#include "paper.h"
#include "screen_config.h"
#include "waveshare_epd.h"
#include "twi_sensor.h"
#include "nrf_gfx.h"
#include "nrf_lcd.h"
#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "nrf_font.h"
#include "arial_16pts.h"
#include "arial_36pts.h"
#include "arial_48pts.h"
#include "screens.h"

/* Scanner */
#include "scanner.h"

/* LPN */
#include "LPN1.h"
#include "server_data.h"
#include "client_data.h"

/* Sensors */
#include "BME280.h"
#include "HDC1080.h"
#include "TSL2581.h"
#include "si7021.h"
#include "CCS811.h"
#include "BMP180.h"

/* ADC */
#include "nrf_drv_saadc.h"
#include "bat_saadc.h"
//#include "bat_saadc.h"

extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
extern const nrf_gfx_font_desc_t orkney_24ptFontInfo;
extern const nrf_gfx_font_desc_t arial_48ptFontInfo;
extern const nrf_gfx_font_desc_t arial_16ptFontInfo;

const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(PAPER_SPI_INSTANCE);

static void terminate_friendship();
static uint32_t prevTime;
static uint32_t prev_interrupt_time = 0;

static uint8_t toggle_lpn = LPN - 1;

static lcd_cb_t lcd_cb = {
 .state    = NRFX_DRV_STATE_UNINITIALIZED,
 .height   = PAPER_PIXEL_HEIGHT,
 .width    = PAPER_PIXEL_WIDTH,
 .rotation = NRF_LCD_ROTATE_90
};

#if EPAPER_MODULE_C

static const nrf_lcd_t lcd = {
  .lcd_init = paper_init,
  .lcd_uninit = paper_uninit,
  .lcd_pixel_draw = paper_pixel_draw,
  .lcd_rect_draw = paper_rect_draw,
  .lcd_display = paper_display,
  .lcd_rotation_set = paper_rotation_set,
  .lcd_display_invert = paper_display_invert,
  .p_lcd_cb = &lcd_cb
};
#else


extern lcd_cb_t wsepd154_cb;

const nrf_lcd_t lcd = {
    .lcd_init = wsepd154_init, // Done
    .lcd_uninit = wsepd154_uninit, // Done
    .lcd_pixel_draw = wsepd154_pixel_draw, // Done
    .lcd_rect_draw = wsepd154_rect_draw, // TODO: reqd
    .lcd_display = wsepd154_display,
    .lcd_rotation_set = wsepd154_rotation_set,
    .lcd_display_invert = wsepd154_display_invert, 
    .p_lcd_cb = &wsepd154_cb
};
#endif
//extern const nrf_lcd_t lcd;


/** The maximum duration to scan for incoming Friend Offers. */
#define FRIEND_REQUEST_TIMEOUT_MS (MESH_LPN_FRIEND_REQUEST_TIMEOUT_MAX_MS)
/** The upper limit for two subsequent Friend Polls. */
#define POLL_TIMEOUT_MS (SEC_TO_MS(10))
/** The time between LPN sending a request and listening for a response. */
#define RECEIVE_DELAY_MS (100)

#define APP_STATE_OFF                   0
#define APP_STATE_ON                    1

/** The time before state ON is switched to OFF */
#define APP_STATE_ON_TIMEOUT_MS         (SEC_TO_MS(10))

#define APP_UNACK_MSG_REPEAT_COUNT      2
#define STATIC_AUTH_DATA                {0x6E, 0x6F, 0x72, 0x64, 0x69, 0x63, 0x5F, 0x65, \
                                         0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x5F, 0x31}

static simple_sensor_client_t m_sensor_client;
static simple_sensor_server_t m_sensor_server;
static simple_sensor_setup_server_t m_sensor_setup_server;

static bool m_device_provisioned;
static uint64_t lpn_timeout_interval = HAL_SECS_TO_RTC_TICKS(2); //300
static volatile uint8_t lpn_timer_index = 0;
static uint8_t lpn_timer_count = 6;
static uint8_t lpn_interval_mode = LPN_INTERVAL_MODE_2;

static uint32_t prev_timeout;

APP_TIMER_DEF(m_state_on_timer);
APP_TIMER_DEF(toggle_lpn_info);
APP_TIMER_DEF(toggle_lpn_time);
APP_TIMER_DEF(empty_timer_id);

static void app_mesh_core_event_cb (const nrf_mesh_evt_t * p_evt);

static nrf_mesh_evt_handler_t m_mesh_core_event_handler = { .evt_cb = app_mesh_core_event_cb };

//  Det var et problem på Nordic-forumet om at hvis ingen timer kjørte i bakgrunn så ville app_timer blir deaktivert
//  Ble fikset ved å kjøre en tom teller
void empy_timer_handler()
{
    uint32_t timeout = (0x00FFFFFF / 2);
    app_timer_start(empty_timer_id, timeout, NULL);
}

bool device_is_proviosioned()
{
  return m_device_provisioned;
}

uint8_t get_LPN_interval_mode()
{
  return lpn_interval_mode;
}

static void device_identification_start_cb(uint8_t attention_duration_s)
{
#if SIMPLE_HAL_LEDS_ENABLED
    hal_led_mask_set(LEDS_MASK, false);
    hal_led_blink_ms(BSP_LED_2_MASK  | BSP_LED_3_MASK,
                     LED_BLINK_ATTENTION_INTERVAL_MS,
                     LED_BLINK_ATTENTION_COUNT(attention_duration_s));
#endif
}

static void provisioning_aborted_cb(void)
{
#if SIMPLE_HAL_LEDS_ENABLED
    hal_led_blink_stop();
#endif
}

static void provisioning_complete_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Successfully provisioned\n");

    /* Restores the application parameters after switching from the Provisioning
     * service to the Proxy  */
    gap_params_init();
    conn_params_init();

#if BLE_DFU_SUPPORT_ENABLED
    ble_dfu_support_service_init();
#endif

    dsm_local_unicast_address_t node_address;
    dsm_local_unicast_addresses_get(&node_address);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Node Address: 0x%04x \n", node_address.address_start);

#if SIMPLE_HAL_LEDS_ENABLED
    hal_led_blink_stop();
    hal_led_mask_set(LEDS_MASK, LED_MASK_STATE_OFF);
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_PROV);
#endif
}

//  Vekker noden og oppdaterer sensor-data og skjerm
static void state_on_timer_handler(void *p_unused)
{
  uint32_t retval;
  UNUSED_VARIABLE(p_unused);
  retval = app_timer_stop(m_state_on_timer);
  if(retval != NRF_SUCCESS)
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Timer couldn't be stopped\n");

  if(++lpn_timer_index >= lpn_timer_count)
  {
    uint32_t time = app_timer_cnt_get();
    time = time - prev_timeout;
    time = HAL_RTC_TICKS_TO_SECS(time);
    prev_timeout = app_timer_cnt_get();
    lpn_timer_index = 0;
    update_and_send_data(&m_sensor_server);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "State on timer handler %d\n", time);
  }

  ERROR_CHECK(app_timer_start(m_state_on_timer, lpn_timeout_interval, NULL));

}

//  Kjører etter man har valgt å enten hente ekstern data, eller oppdatere lokal data
void toggle_lpn_data()
{
  app_timer_stop(toggle_lpn_info);
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Getting new data\n");
    if(toggle_lpn == LPN_ID_1)
    {
      display_LPN_1_data(&m_sensor_client);
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "LPN 1\n");
    }
  
    else if(toggle_lpn == LPN_ID_2)
    {
      display_LPN_2_data(&m_sensor_client);
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "LPN 2\n");
    }

    else if(toggle_lpn == LPN_ID_3)
    {
      display_LPN_3_data(&m_sensor_client);
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "LPN 3\n");
    }

  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Going to sleep\n");
  toggle_lpn = LPN - 1;
}

//  Blir kjørt etter man har valgt intervall, setter noden i ønsket tids-intervall
void toggle_lpn_interval()
{
  app_timer_stop(m_state_on_timer);
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Lpn time mode %d\n", lpn_interval_mode);

  if(lpn_interval_mode == LPN_INTERVAL_MODE_1)
  {
    lpn_timer_count = 3;
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Timeout set to 15 minutes\n");
  }
  else if(lpn_interval_mode == LPN_INTERVAL_MODE_2)
  {
    lpn_timer_count = 6;
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Timeout set to 30 minutes\n");
  }
  else if(lpn_interval_mode == LPN_INTERVAL_MODE_3)
  {
    lpn_timer_count = 12;
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Timeout set to 1 hour\n");
  }
  update_data(false);
  app_timer_start(m_state_on_timer, lpn_timeout_interval, NULL);
  lpn_timer_index = 0;
  prev_timeout = app_timer_cnt_get();
}

static void node_reset(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Node reset  -----\n");

#if SIMPLE_HAL_LEDS_ENABLED
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_RESET);
#endif

    /* This function may return if there are ongoing flash operations. */
    mesh_stack_device_reset();
}

static void config_server_evt_cb(const config_server_evt_t * p_evt)
{
    if (p_evt->type == CONFIG_SERVER_EVT_NODE_RESET)
    {
        node_reset();
    }
}

static void initiate_friendship()
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initiating the friendship establishment procedure.\n");

    mesh_lpn_friend_request_t freq;
    freq.friend_criteria.friend_queue_size_min_log = MESH_FRIENDSHIP_MIN_FRIEND_QUEUE_SIZE_16;
    freq.friend_criteria.receive_window_factor = MESH_FRIENDSHIP_RECEIVE_WINDOW_FACTOR_1_0;
    freq.friend_criteria.rssi_factor = MESH_FRIENDSHIP_RSSI_FACTOR_2_0;
    freq.poll_timeout_ms = POLL_TIMEOUT_MS;
    freq.receive_delay_ms = RECEIVE_DELAY_MS;

    uint32_t status = mesh_lpn_friend_request(freq, FRIEND_REQUEST_TIMEOUT_MS);
    switch (status)
    {
        case NRF_SUCCESS:
            break;

        case NRF_ERROR_INVALID_STATE:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Already in an active friendship\n");
#if SIMPLE_HAL_LEDS_ENABLED
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_ERROR);
#endif
            break;

        case NRF_ERROR_INVALID_PARAM:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Friend request parameters outside of valid ranges.\n");
#if SIMPLE_HAL_LEDS_ENABLED
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_ERROR);
#endif
            break;

        default:
            ERROR_CHECK(status);
            break;
    }
}

static void terminate_friendship()
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Terminating the active friendship\n");

    uint32_t status = mesh_lpn_friendship_terminate();
    switch (status)
    {
        case NRF_SUCCESS:
            break;

        case NRF_ERROR_INVALID_STATE:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Not in an active friendship\n");
#if SIMPLE_HAL_LEDS_ENABLED
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_ERROR);
#endif
            break;

        default:
            ERROR_CHECK(status);
            break;
    }
}

#if RTT_INPUT_ENABLED
static void rtt_input_handler(int key)
{
    if (key >= '0' && key <= '3')
    {
        uint32_t button_number = key - '0';
        button_event_handler(button_number);
    }
}
#endif

static void app_mesh_core_event_cb(const nrf_mesh_evt_t * p_evt)
{
    /* USER_NOTE: User can insert mesh core event proceesing here */
    switch (p_evt->type)
    {
        case NRF_MESH_EVT_LPN_FRIEND_OFFER:
        {
            const nrf_mesh_evt_lpn_friend_offer_t *p_offer = &p_evt->params.friend_offer;

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO,
                  "Received friend offer from 0x%04X\n",
                  p_offer->src);

            uint32_t status = mesh_lpn_friend_accept(p_offer);
            switch (status)
            {
                case NRF_SUCCESS:
                    break;

                case NRF_ERROR_INVALID_STATE:
                case NRF_ERROR_INVALID_PARAM:
                case NRF_ERROR_NULL:
                    __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR,
                          "Cannot accept friendship: %d\n",
                          status);
#if SIMPLE_HAL_LEDS_ENABLED
                    hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS,
                                     LED_BLINK_CNT_ERROR);
#endif
                    break;

                default:
                    ERROR_CHECK(status);
                    break;
            }

            break;
        }

        case NRF_MESH_EVT_LPN_FRIEND_POLL_COMPLETE:
            //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Friend poll procedure complete\n");
            break;

        case NRF_MESH_EVT_LPN_FRIEND_REQUEST_TIMEOUT:
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Friend Request timed out\n");
#if SIMPLE_HAL_LEDS_ENABLED
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_ERROR);
#endif
            break;

        case NRF_MESH_EVT_FRIENDSHIP_ESTABLISHED:
        {
            const nrf_mesh_evt_friendship_established_t *p_est =
                    &p_evt->params.friendship_established;
            (void) p_est;

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO,
                  "Friendship established with: 0x%04X\n",
                  p_est->friend_src);

#if SIMPLE_HAL_LEDS_ENABLED
            hal_led_pin_set(BSP_LED_1, true);
#endif
            break;
        }

        case NRF_MESH_EVT_FRIENDSHIP_TERMINATED:
        {
            const nrf_mesh_evt_friendship_terminated_t *p_term = &p_evt->params.friendship_terminated;
            UNUSED_VARIABLE(p_term);

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO,
                  "Friendship with 0x%04X terminated. Reason: %d\n",
                  p_term->friend_src, p_term->reason);

#if SIMPLE_HAL_LEDS_ENABLED
            hal_led_pin_set(BSP_LED_1, false);
#endif

            ERROR_CHECK(app_timer_stop(m_state_on_timer));
            break;
        }

        default:
            break;
    }
}

//  Starter mesh-modeller
static void models_init_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initializing and adding models\n");

    uint32_t err;

    err = simple_sensor_server_init(&m_sensor_server, 1);
    if(err != NRF_SUCCESS)
    {
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error adding model: %d\n", err);
    }

    err = simple_sensor_setup_server_init(&m_sensor_setup_server, 1);
    if(err != NRF_SUCCESS)
    {
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error adding model: %d\n", err);
    }

    err = simple_sensor_client_init(&m_sensor_client, 2);
    if(err != NRF_SUCCESS)
    {
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error adding model: %d\n", err);
    }
}

static void mesh_init(void)
{
    mesh_stack_init_params_t init_params =
    {
        .core.irq_priority       = NRF_MESH_IRQ_PRIORITY_LOWEST,
        .core.lfclksrc           = DEV_BOARD_LF_CLK_CFG,
        .core.p_uuid             = NULL,
        .models.models_init_cb   = models_init_cb,
        .models.config_server_cb = config_server_evt_cb
    };

    uint32_t status = mesh_stack_init(&init_params, &m_device_provisioned);
    switch (status)
    {
        case NRF_ERROR_INVALID_DATA:
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Data in the persistent memory was corrupted. Device starts as unprovisioned.\n");
            break;
        case NRF_SUCCESS:
            break;
        default:
            ERROR_CHECK(status);
    }

    /* Register event handler to receive LPN and friendship events. */
    nrf_mesh_evt_handler_add(&m_mesh_core_event_handler);
}

#if BLE_DFU_SUPPORT_ENABLED
/** Initializes Power Management. Required for BLE DFU. */
static void power_management_init(void)
{
    uint32_t err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}
#endif

static void initialize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS/* | LOG_SRC_BEARER*/, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh LPN Demo -----\n");

    ERROR_CHECK(app_timer_init());

#if SIMPLE_HAL_LEDS_ENABLED
    hal_leds_init();
#endif
/*
#if BUTTON_BOARD
    ERROR_CHECK(hal_buttons_init(button_event_handler));
#endif
*/
#if BLE_DFU_SUPPORT_ENABLED
    ble_dfu_support_init();
    power_management_init();
#endif

    ble_stack_init();
    gap_params_init();
    conn_params_init();

#if BLE_DFU_SUPPORT_ENABLED
    ble_dfu_support_service_init();
#endif

    mesh_init();
    ERROR_CHECK(sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE));

    mesh_lpn_init();
}

//  Setter opp noen tellere og starter mesh-stacken
static void start(void)
{
#if RTT_INPUT_ENABLED
    rtt_input_enable(rtt_input_handler, RTT_INPUT_POLL_PERIOD_MS);
#endif

    ERROR_CHECK(app_timer_create(&m_state_on_timer, APP_TIMER_MODE_SINGLE_SHOT,
                                 state_on_timer_handler));

    ERROR_CHECK(app_timer_create(&toggle_lpn_info, APP_TIMER_MODE_SINGLE_SHOT, toggle_lpn_data));

    ERROR_CHECK(app_timer_create(&toggle_lpn_time, APP_TIMER_MODE_SINGLE_SHOT, toggle_lpn_interval));

    ERROR_CHECK(app_timer_create(&empty_timer_id, APP_TIMER_MODE_SINGLE_SHOT, empy_timer_handler));

    uint32_t timeout = (0x00FFFFFF / 2);
    app_timer_start(empty_timer_id, timeout, NULL);

    if (!m_device_provisioned)
    {
        static const uint8_t static_auth_data[NRF_MESH_KEY_SIZE] = STATIC_AUTH_DATA;
        mesh_provisionee_start_params_t prov_start_params =
        {
            .p_static_data    = static_auth_data,
            .prov_complete_cb = provisioning_complete_cb,
            .prov_device_identification_start_cb = device_identification_start_cb,
            .prov_device_identification_stop_cb = NULL,
            .prov_abort_cb = provisioning_aborted_cb,
            .p_device_uri = EX_URI_LPN
        };
        ERROR_CHECK(mesh_provisionee_prov_start(&prov_start_params));
    }

    mesh_app_uuid_print(nrf_mesh_configure_device_uuid_get());

    ERROR_CHECK(mesh_stack_start());

#if SIMPLE_HAL_LEDS_ENABLED
    hal_led_mask_set(LEDS_MASK, LED_MASK_STATE_OFF);
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_START);
#endif
}

//  Setter opp alle sensorer som skal settes opp, pluss ADC
void init_sensors()
{
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Enable pin %d\n", ENABLE_SENSOR);
  nrf_gpio_cfg_output(ENABLE_SENSOR);
  nrf_gpio_cfg_output(CCS811_WAKE);
  enable_sensors();
  nrf_delay_ms(100);
  twi_init();
#if LPN == LPN_ID_1
  si7021_init();

#elif LPN == LPN_ID_2
  BME280_init();

#elif LPN == LPN_ID_3
  TSL2581_init();
  HDC1080_init();
  ccs811_uninit();

#endif
  twi_uninit();
  disable_sensors();
}

void friendship_establish_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  if(action == NRF_GPIOTE_POLARITY_HITOLO && !mesh_lpn_is_in_friendship())
  {
    initiate_friendship();
  }
}

//  Funksjon for å oppdatere lokal data, eller hente ekstern data
//  Ett sekund etter siste knappetrykk kjøres en funksjon som sjekket hvilken indeks man er på og enten oppdaterer skjermen
// eller henter inn ekstern data
void send_node_status_get_message(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  if(action == NRF_GPIOTE_POLARITY_HITOLO)
  {
    uint32_t time_diff = app_timer_cnt_get() - prev_interrupt_time;
    time_diff = HAL_RTC_TICKS_TO_MS(time_diff);
    if(time_diff < 150)
    {
      return;
    }
    app_timer_stop(toggle_lpn_info);
    prev_interrupt_time = app_timer_cnt_get();

    if(++toggle_lpn > LPN_ID_3)
      toggle_lpn = LPN_ID_1;

    app_timer_start(toggle_lpn_info, HAL_SECS_TO_RTC_TICKS(1), NULL);
  }
}

//  Denne funksjonen kjører hver gang man skal stille intervall
//  intervall-modus inkrementerer og etter ett sekund etter siste knappetrykk vil noden bli stilt inn på valgt modus
void select_lpn_time(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Interrupt\n");
  if(action == NRF_GPIOTE_POLARITY_HITOLO)
  {
    uint32_t interrupt_time = app_timer_cnt_get();
    uint32_t time_diff = interrupt_time - prev_interrupt_time;
    time_diff = HAL_RTC_TICKS_TO_MS(time_diff);
    if(time_diff < 150)
    {
      return;
    }
    prev_interrupt_time = interrupt_time;
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Interval interrupt %d\n", time_diff);
    app_timer_stop(toggle_lpn_time);
    if(lpn_interval_mode++ >= LPN_INTERVAL_MODE_3)
    {
      lpn_interval_mode = LPN_INTERVAL_MODE_1;
    }

    app_timer_start(toggle_lpn_time, HAL_SECS_TO_RTC_TICKS(2), NULL); //1 sekund på å velge intervall
  }
}

//  Tegner en graf på skjermen når riktig knapp blir trykket
void draw_LPN_temp_graph(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  draw_temp_graph();
}

//  Setter opp low power interrupt for vekke noden hver gang en funksjonsknapp blir trykket
void button_interrupt_init()
{
    ret_code_t err_code;

    err_code = nrf_drv_gpiote_init();
    if(err_code != NRF_SUCCESS)
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error initializing gpiote\n");

    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_in_init(GET_REMOTE_DATA_PIN, &in_config, send_node_status_get_message);
    if(err_code != NRF_SUCCESS)
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error initializing pin %d interrupt %d\n", GET_REMOTE_DATA_PIN, err_code);
    nrf_drv_gpiote_in_event_enable(GET_REMOTE_DATA_PIN, true);

    err_code = nrf_drv_gpiote_in_init(SET_INTERVAL_PIN, &in_config, select_lpn_time);
    if(err_code != NRF_SUCCESS)
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error initializing pin %d interrupt %d\n", SET_INTERVAL_PIN, err_code);
    nrf_drv_gpiote_in_event_enable(SET_INTERVAL_PIN, true);

    err_code = nrf_drv_gpiote_in_init(DRAW_GRAPH_PIN, &in_config, draw_LPN_temp_graph);
    if(err_code != NRF_SUCCESS)
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error initializing pin %d interrupt %d\n", DRAW_GRAPH_PIN, err_code);
    nrf_drv_gpiote_in_event_enable(DRAW_GRAPH_PIN, true);
}

int main(void)
{
  uint32_t err;
  initialize();
  start();
  sensors_server_init(&m_sensor_server, &m_sensor_setup_server);
  sensors_client_init(&m_sensor_client);
  init_sensors();
  init_lpn(&m_sensor_server);
  access_default_ttl_set(5);
  //uint32_t time = app_timer_cnt_get();
  err = nrf_gfx_init(&lcd);

  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error starting display\n");
  }
  else
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Display started: %d\n", err);
  }

  draw_background();
  nrf_gfx_display(&lcd);
  nrf_gfx_uninit(&lcd);

//  Brukt for å måle til skjermen bruker på å oppdatere
/*
  uint32_t time2 = app_timer_cnt_get();
  time2 = time2 - time;
  time2 = HAL_RTC_TICKS_TO_MS(time2);
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Update time: %d\n", time2);
*/
  button_interrupt_init();

  err = app_timer_start(m_state_on_timer, lpn_timeout_interval, NULL);

  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error setting up timer interrupt\n");
  }
  else
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Timer interrupt started\n");
  }

  for (;;)
  {
    sd_app_evt_wait();
  }
}
