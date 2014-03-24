/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_app_adv_serialized_main main.c
 * @{
 * @ingroup ble_sdk_app_adv_serialization
 * @brief Serialized Advertising example application main file.
 *
 * This is a simple application for demonstrating how to set up and initiate
 * advertising through serialisation of SoftDevice Command and Events. 
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "app_scheduler.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_assert.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "nrf51_bitfields.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "boards.h"
#include "softdevice_handler.h"
#include "app_gpiote.h"
#include "app_error.h"
#include "ble_debug_assert_handler.h"
#include "app_timer.h"
#include "nrf_delay.h"

#define ADVERTISING_LED_PIN_NO        LED_0                             /**< Is on when device is advertising. */
#define CONNECTED_LED_PIN_NO          LED_1                             /**< Is on when device has connected. */
#define MAIN_FUNCTION_LED_PIN_NO      LED_2                             /**< LED indicating that the program has entered main function. */
#define ASSERT_LED_PIN_NO             LED_7                             /**< Is on when application has asserted. */


#define DEVICE_NAME                   "BLE Connectivity"                /**< Name of device. Will be included in the advertising data. */
#define APP_ADV_INTERVAL              64                                /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS    180                               /**< The advertising timeout (in units of seconds). */

#define MIN_CONN_INTERVAL             MSEC_TO_UNITS(500, UNIT_1_25_MS)  /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL             MSEC_TO_UNITS(1000, UNIT_1_25_MS) /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                 0                                 /**< Slave latency. */
#define CONN_SUP_TIMEOUT              MSEC_TO_UNITS(4000, UNIT_10_MS)   /**< Connection supervisory timeout (4 seconds) */

#define DEAD_BEEF                     0xDEADBEEF                        /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define CONN_CHIP_RESET_PIN_NO        30                                /**< Pin used for reseting the nRF51822. */
#define CONN_CHIP_RESET_TIME          50                                /**< The time to keep the reset line to the nRF51822 low (in milliseconds). */
#define CONN_CHIP_WAKEUP_TIME         500                               /**< The time for nRF51822 to reset and become ready to receive serialized commands (in milliseconds). */

#define SCHED_MAX_EVENT_DATA_SIZE     BLE_STACK_HANDLER_SCHED_EVT_SIZE  /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE              10                                /**< Maximum number of events in the scheduler queue. */

#define APP_TIMER_PRESCALER           0                                 /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS          1                                 /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE       1                                 /**< Size of timer operation queues. */

#define APP_GPIOTE_MAX_USERS          1                                 /**< Maximum number of users of the GPIOTE handler. In this example the GPIOTE user is: UART Module used by HCI Transport layer. */

static ble_gap_adv_params_t m_adv_params;


void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    nrf_gpio_pin_set(ASSERT_LED_PIN_NO);

    // This call can be used for debug purposes during application development.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    // ble_debug_assert_handler(error_code, line_num, p_file_name);

    // On assert, the system can only recover on reset.
    NVIC_SystemReset();
}


void assert_nrf_callback(uint16_t line_num, const uint8_t * file_name)
{
    app_error_handler(DEAD_BEEF, line_num, file_name);
}


/**@brief Function for initializing the LEDs.
 *
 * @details Initializes all LEDs used by this application.
 */
static void leds_init(void)
{
    // Initialize debug pins
    nrf_gpio_cfg_output(ADVERTISING_LED_PIN_NO);
    nrf_gpio_cfg_output(CONNECTED_LED_PIN_NO);
    nrf_gpio_cfg_output(MAIN_FUNCTION_LED_PIN_NO);
    nrf_gpio_cfg_output(ASSERT_LED_PIN_NO);
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    uint32_t err_code;
    
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            nrf_gpio_pin_set(CONNECTED_LED_PIN_NO);
            nrf_gpio_pin_clear(ADVERTISING_LED_PIN_NO);
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
            nrf_gpio_pin_clear(CONNECTED_LED_PIN_NO);
            nrf_gpio_pin_set(ADVERTISING_LED_PIN_NO);
        
            err_code = sd_ble_gap_adv_start(&m_adv_params);
            APP_ERROR_CHECK(err_code);
            break;
       
        case BLE_GAP_EVT_TIMEOUT:
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT)
             {
                 nrf_gpio_pin_clear(ADVERTISING_LED_PIN_NO);
                 nrf_gpio_pin_clear(MAIN_FUNCTION_LED_PIN_NO);

                 // Place the nRF51822 in System Off Mode.
                 err_code = sd_power_system_off();
                 APP_ERROR_CHECK(err_code);
             }
            break;
        
        default:
            // No implementation needed.
           break;
    }
}


/**@brief Function for initializing the BLE Soft Device and Advertisement parameters.
 *
 * @details This function is called to initialize BLE stack and GAP Parameters needed for advertising
 *          event has been received.
 *
 */
static void bluetooth_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_cp;
    ble_advdata_t           advdata;
    ble_gap_conn_sec_mode_t sec_mode;
    uint8_t                 flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    
    // Initialize SoftDevice.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, true);
    
    // Subscribe for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
    
    // Set GAP parameters
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    
    err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_TAG);
    APP_ERROR_CHECK(err_code);
    
    memset(&gap_cp, 0, sizeof(gap_cp));
    
    gap_cp.min_conn_interval = MIN_CONN_INTERVAL;
    gap_cp.max_conn_interval = MAX_CONN_INTERVAL;
    gap_cp.slave_latency     = SLAVE_LATENCY;
    gap_cp.conn_sup_timeout  = CONN_SUP_TIMEOUT;
    
    err_code = sd_ble_gap_ppcp_set(&gap_cp);
    APP_ERROR_CHECK(err_code);
    
    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));
    
    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = true;
    advdata.flags.size         = sizeof(flags);
    advdata.flags.p_data       = &flags;
    
    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);
    
    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));
    
    m_adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
    m_adv_params.p_peer_addr = NULL;                           // Undirected advertisement.
    m_adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval    = APP_ADV_INTERVAL;
    m_adv_params.timeout     = APP_ADV_TIMEOUT_IN_SECONDS;
}


/**@brief Function for power management.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for resetting the nRF51822.
 *
 */
static void connectivity_chip_reset(void)
{
    nrf_gpio_cfg_output(CONN_CHIP_RESET_PIN_NO);
    
    // Signal a reset to the nRF51822 by setting the reset pin on the nRF51822 low.
    nrf_gpio_pin_clear(CONN_CHIP_RESET_PIN_NO);
    nrf_delay_ms(CONN_CHIP_RESET_TIME);
    
    // Set the reset level to high again.
    nrf_gpio_pin_set(CONN_CHIP_RESET_PIN_NO);
    
    // Wait for nRF51822 to be ready.
    nrf_delay_ms(CONN_CHIP_WAKEUP_TIME);
}


/**@brief Function for initializing the event scheduler.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

/**@brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code;
    
    leds_init();
    
    // Indicate that the application has started up. This can be used to verify that 
    // application resets if reset button is pressed or the chip has been reset with 
    // a debugger. The led will be off during reset.
    nrf_gpio_pin_set(MAIN_FUNCTION_LED_PIN_NO);
    
    // Configure GPIOTE peripheral with one user for the UART flow control feature. 
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS); 

    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);

    connectivity_chip_reset();    
    scheduler_init();
    bluetooth_init();
    
    err_code = sd_ble_gap_adv_start(&m_adv_params);
    APP_ERROR_CHECK(err_code);

    nrf_gpio_pin_set(ADVERTISING_LED_PIN_NO);
    
    // Enter main loop.
    for (;;)
    {
        app_sched_execute();

        // Power management.
        power_manage();
    }
}
/*
@}
*/
