/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

/** @example Board/nrf6310/s120/experimental/ble_app_multilink_central/main.c
 *
 * @brief Multilink BLE application main file.
 *
 * This file contains the source code for a sample application wherea Central connetects to several Peripherals.
 */
 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf_sdm.h"
#include "ble.h"
#include "client_handling.h"
#include "softdevice_types.h"       
#include "softdevice_handler.h"
#include "app_util.h"
#include "app_error.h"
#include "ble_advdata_parser.h"
#include "nrf6310.h"
#include "nrf_gpio.h"
#include "conn_mngr.h"
#include "debug.h"

#define KEY_PRESS_BUTTON_PIN_NO         BUTTON_0    /**< Button used for sending keyboard text. */
#define BONDMNGR_DELETE_BUTTON_PIN_NO   BUTTON_1    /**< Button used for deleting all bonded centrals during startup. */

#define ADVERTISING_LED_PIN_NO          LED_0       /**< Is on when device is advertising. */
#define CONNECTED_LED_PIN_NO            LED_1       /**< Is on when device has connected. */
#define ASSERT_LED_PIN_NO               LED_7       /**< Is on when application has asserted. */

#define APPL_LOG                        debug_log   /**< Debug logger macro that will be used in this file to do logging of debug information over UART. */



/**@brief Function for error handling, which is called when an error has occurred. 
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name. 
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    APPL_LOG("[APPL]: ASSERT: %s, %d, error %d\r\n", p_file_name, line_num, error_code);
    nrf_gpio_pin_set(ASSERT_LED_PIN_NO);

    // This call can be used for debug purposes during development of an application.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    // ble_debug_assert_handler(error_code, line_num, p_file_name);

    // On assert, the system can only recover with a reset.
    NVIC_SystemReset();
}


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Callback handling connection manager events.
 *
 * @details This function is called to notify the application of connection manager events.
 *
 * @param[in]   p_handle      Connection Manager Handle. For link related events, this parameter
 *                            identifies the peer.
 * @param[in]   p_event       Pointer to the connection manager event.
 * @param[in]   event_status  Status of the event.
 */
void connection_manager_event_handler(conn_mngr_handle_t *    p_handle,
                                      conn_mngr_event_t *     p_event,
                                      uint32_t                event_status)
{
    uint32_t err_code;
    
    switch(p_event->event_id)
    {        
        case CONN_MNGR_CONN_COMPLETE_IND:
        {
#ifdef ENABLE_DEBUG_LOG_SUPPORT
            ble_gap_addr_t * peer_addr;            
            peer_addr = (ble_gap_addr_t *)p_event->p_event_param;
#endif // ENABLE_DEBUG_LOG_SUPPORT
            APPL_LOG("[APPL]:[%02X %02X %02X %02X %02X %02X]: Connection Established\r\n",
                            peer_addr->addr[0], peer_addr->addr[1], peer_addr->addr[2],
                            peer_addr->addr[3], peer_addr->addr[4], peer_addr->addr[5]);
            APPL_LOG("\r\n");

            err_code = client_create(p_handle);
            APP_ERROR_CHECK(err_code);
            break;
        }
        
        case CONN_MNGR_DISCONNECT_IND:
            APPL_LOG("[APPL]: Disconnected\r\n");

            err_code = client_destroy(p_handle);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    conn_mngr_ble_evt_handler(p_ble_evt);
    client_ble_evt_handler(p_ble_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;
  
    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Connection Manager.
 *
 * @details Connection manager is initialized here.
 */
static void connection_manager_init(void)
{
    uint32_t              err_code;
    conn_mngr_app_param_t param;
    
    err_code = conn_mngr_init();
    APP_ERROR_CHECK(err_code);
    
    param.ntf_cb = connection_manager_event_handler;
    err_code = conn_mngr_register(&param);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting the Connection Manager.
 *
 * @details Connection manager start will result in scanning 
*           and connection if a device matching the policy was discovered.
 */
static void connection_manager_start(void)
{
    uint32_t err_code;
    
    err_code = conn_mngr_start();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_event_wait();
    APP_ERROR_CHECK(err_code);
}


/**@brief Funtion for freeing up a client by setting its state to idle.
 */
int main(void)
{
    debug_init();
    ble_stack_init();
    client_init();
    connection_manager_init();
    connection_manager_start();

    for (;;)
    {
        power_manage();
    }
}

