/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
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

/**@file
 *
 * @defgroup ble_sdk_app_connectivity_main main.c
 * @{
 * @ingroup ble_sdk_app_connectivity
 *
 * @brief BLE Connectivity application.
 */

#include <string.h>
#include <stddef.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_error.h"
#include "nrf51_bitfields.h"
#include "nrf_gpio.h"
#include "ble.h"
#include "softdevice_handler.h"
#include "boards.h"
#include "ble_debug_assert_handler.h"
#include "app_error.h"
#include "app_gpiote.h"
#include "hci_transport.h"
#include "ble_rpc_event_encoder.h"
#include "ble_rpc_cmd_decoder.h"
#include "app_timer.h"

#define ASSERT_LED_PIN_NO                  LED_7                                          /**< Is on when application has asserted. */

#define APP_TIMER_PRESCALER                0                                              /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS               1                                              /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE            1                                              /**< Size of timer operation queues. */

#define APP_GPIOTE_MAX_USERS               1                                              /**< Maximum number of users of the GPIOTE handler. In this example the GPIOTE user is: UART Module used by HCI Transport layer. */

#define SCHED_QUEUE_SIZE                   10u                                            /**< Maximum number of events in the scheduler queue. */
#define SCHED_MAX_EVENT_DATA_SIZE          MAX(sizeof(app_timer_event_t),\
                                               BLE_STACK_HANDLER_SCHED_EVT_SIZE)          /**< Maximum size of scheduler events. */

#define DEAD_BEEF                          0xDEADBEEF                                     /**< Value used as error code on stack dump. Can be used to identify stack location on stack unwind. */


/**@brief Function for handling an error, which is called when an error has occurred. 
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name. 
 */
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


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called if the ASSERT macro fails.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for managing power.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for scheduling a transport layer event.
 * 
 * @details Transfer the given event to the scheduler to be processed in thread mode.
 * 
 * @param[in] event       RPC transport layer event.
 */
void transport_evt_handle(hci_transport_evt_t event)
{
    if (event.evt_type == HCI_TRANSPORT_RX_RDY)
    {
        uint32_t err_code = app_sched_event_put(NULL, 0, ble_rpc_cmd_handle);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for initializing the LEDs.
 *
 * @details Initializes all LEDs used by this application.
 */
static void leds_init(void)
{
    nrf_gpio_cfg_output(ASSERT_LED_PIN_NO);
}

/**@brief Callback function for transport layer to indicate when it 
 *        is finished transmitting the bytes from the TX buffer.
 *
 * @param[in] result      TX done event result code.
 */
void transport_tx_complete_handle(hci_transport_tx_done_result_t result)
{
    uint32_t err_code = hci_transport_tx_free(); 
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code;
    
    leds_init();
    
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
    
    // Initialize SoftDevice.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, true);
    
    // Subscribe for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_rpc_event_handle);
    APP_ERROR_CHECK(err_code);

    // Configure GPIOTE with one user for the UART flow control feature. 
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
    
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);

    // Open transport layer.
    err_code = hci_transport_open();
    APP_ERROR_CHECK(err_code);
    
    err_code = hci_transport_evt_handler_reg(transport_evt_handle);
    APP_ERROR_CHECK(err_code);
    
    err_code = hci_transport_tx_done_register(transport_tx_complete_handle);
    APP_ERROR_CHECK(err_code);
    
    // Enter main loop.
    for (;;)
    {   
        app_sched_execute();
        power_manage();   
    }
}
/** @} */
