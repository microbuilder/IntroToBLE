/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf_sdm.h"
#include "ble.h"
#include "softdevice_types.h"
#include "ble_db_discovery.h"       
#include "softdevice_handler.h"
#include "app_util.h"
#include "app_error.h"
#include "ble_advdata_parser.h"
#include "nrf6310.h"
#include "nrf6350.h"
#include "nrf_gpio.h"
#include "conn_mngr.h"
#include "debug.h"
#include "ble_hrs_c.h"

#define KEY_PRESS_BUTTON_PIN_NO         BUTTON_0                  /**< Button used for sending keyboard text. */
#define BONDMNGR_DELETE_BUTTON_PIN_NO   BUTTON_1                  /**< Button used for deleting all bonded centrals during startup. */

#define SCAN_LED_PIN_NO                 LED_0                     /**< Is on when device is scanning. */
#define CONNECTED_LED_PIN               LED_1                     /**< Is on when device has connected. */
#define ASSERT_LED_PIN_NO               LED_7                     /**< Is on when application has asserted. */

static ble_db_discovery_t               m_ble_db_discovery;       /**< Structure used to identify the DB Discovery module. */
static ble_hrs_c_t                      m_ble_hrs_c;              /**< Structure used to identify the heart rate client module. */

#define APPL_LOG                        debug_log                 /**< Debug logger macro that will be used in this file to do logging of debug information over UART. */

// WARNING: The following macro MUST be un-defined (by commenting out the definition) if the user
// does not have a nRF6350 Display unit. If this is not done, the application will not work.
#define APPL_LCD_PRINT_ENABLE                                     /**< In case you do not have a functional display unit, disable this flag and observe trace on UART. */

#ifdef APPL_LCD_PRINT_ENABLE

#define APPL_LCD_CLEAR                  nrf6350_lcd_clear         /**< Macro to clear the LCD display.*/
#define APPL_LCD_WRITE                  nrf6350_lcd_write_string  /**< Macro to write a string to the LCD display.*/

#else // APPL_LCD_PRINT_ENABLE

#define APPL_LCD_WRITE(...)             true                      /**< Macro to clear the LCD display defined to do nothing when @ref APPL_LCD_PRINT_ENABLE is not defined.*/
#define APPL_LCD_CLEAR(...)             true                      /**< Macro to write a string to the LCD display defined to do nothing when @ref APPL_LCD_PRINT_ENABLE is not defined.*/

#endif // APPL_LCD_PRINT_ENABLE


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
 * @param[in]   p_handle      Connection Manager Handle. For link related events, 
 *                            identifies the peer.
 * @param[in]   p_event       Identifies the event and related parameters.
 * @param[in]   event_status  Status of the event.
 */
void connection_manager_event_handler(conn_mngr_handle_t *    p_handle,
                                      conn_mngr_event_t *     p_event,
                                      uint32_t                event_status)
{
    static bool scan_in_progress = false;
    bool        lcd_write_status;
    
    switch(p_event->event_id)
    {
        case CONN_MNGR_SCAN_START_IND:
            scan_in_progress = true;

            APPL_LOG("[APPL]: Scan started\r\n");

            nrf_gpio_pin_set(SCAN_LED_PIN_NO);
            break;

        case CONN_MNGR_SCAN_STOP_IND:
            scan_in_progress = false;

            APPL_LOG("[APPL]: Scan stopped\r\n");

            nrf_gpio_pin_clear(SCAN_LED_PIN_NO);
            break;

        case CONN_MNGR_CONN_REQUESTED_IND:
            {
                ble_gap_addr_t * peer_addr;
            
                peer_addr = (ble_gap_addr_t *)p_event->p_event_param;

                APPL_LOG("\r\n[APPL]:[%02X %02X %02X %02X %02X %02X]: Connection Requested\r\n",
                                peer_addr->addr[0], peer_addr->addr[1], peer_addr->addr[2],
                                peer_addr->addr[3], peer_addr->addr[4], peer_addr->addr[5]);

                lcd_write_status = APPL_LCD_WRITE("Connecting", 10, LCD_UPPER_LINE, 0);

                if (!lcd_write_status)
                {
                    APPL_LOG ("[APPL]: LCD Write failed!\r\n");
                }
            }
            break;

        case CONN_MNGR_CONN_COMPLETE_IND:
            {
                ble_gap_addr_t * peer_addr;
            
                peer_addr = (ble_gap_addr_t *)p_event->p_event_param;

                APPL_LOG ("[APPL]:[%02X %02X %02X %02X %02X %02X]: Connection Established\r\n",
                                peer_addr->addr[0], peer_addr->addr[1], peer_addr->addr[2],
                                peer_addr->addr[3], peer_addr->addr[4], peer_addr->addr[5]);
                APPL_LOG("\r\n");

                nrf_gpio_pin_set(CONNECTED_LED_PIN);

                lcd_write_status = APPL_LCD_WRITE("Connected", 9, LCD_UPPER_LINE, 0);
                if (!lcd_write_status)
                {
                    APPL_LOG("[APPL]: LCD Write failed!\r\n");
                }

                uint32_t err_code = ble_db_discovery_start(&m_ble_db_discovery,
                                                           p_handle->conn_handle);
                APP_ERROR_CHECK(err_code);
            }
            break;

        case CONN_MNGR_DISCONNECT_IND:

            memset(&m_ble_db_discovery, 0 , sizeof (m_ble_db_discovery));

            APPL_LOG("[APPL]: Disconnected\r\n");

            lcd_write_status = APPL_LCD_CLEAR();
            if (!lcd_write_status)
            {
                APPL_LOG("[APPL]: LCD Clear failed!\r\n");
            }

            lcd_write_status = APPL_LCD_WRITE("Disconnected", 12, LCD_UPPER_LINE, 0);
            if (!lcd_write_status)
            {
                APPL_LOG("[APPL]:[4]: LCD Write failed!\r\n");
            }

            nrf_gpio_pin_clear(CONNECTED_LED_PIN);
            break;

        default:
            break;
    }
    
    if(scan_in_progress)
    {
        lcd_write_status = APPL_LCD_WRITE("Scanning", 8, LCD_UPPER_LINE, 0);
        if (!lcd_write_status)
        {
            APPL_LOG("[APPL]: LCD Write failed!\r\n");
        }
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
    ble_db_discovery_on_ble_evt(&m_ble_db_discovery, p_ble_evt);
    ble_hrs_c_on_ble_evt(&m_ble_hrs_c, p_ble_evt);
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

    err_code     = conn_mngr_register(&param);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by this application.
 */
static void leds_init(void)
{
    nrf_gpio_cfg_output(SCAN_LED_PIN_NO);
    nrf_gpio_cfg_output(CONNECTED_LED_PIN);
    nrf_gpio_cfg_output(ASSERT_LED_PIN_NO);
}


/** @brief Function for the initialization of the nRF 6350 display.
 */
void nrf6350_init(void)
{
#ifdef APPL_LCD_PRINT_ENABLE
    bool success;

    success = nrf6350_lcd_init();
    if (success)
    {
        success = nrf6350_lcd_on();
        APP_ERROR_CHECK_BOOL(success);
        
        success = nrf6350_lcd_set_contrast(LCD_CONTRAST_HIGH);
        APP_ERROR_CHECK_BOOL(success);
    }
#endif // APPL_LCD_PRINT_ENABLE
}


/**@brief Function for starting the Connection Manager.
 *
 * @details Connection manager start will result in scanning 
 *          and connection if a device matching the policy was discovered.
 */
static void connection_manager_start(void)
{
    uint32_t err_code;
    
    err_code = conn_mngr_start();
    APP_ERROR_CHECK(err_code);
}


/** @brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_event_wait();
    APP_ERROR_CHECK(err_code);
}


/**@brief Heart Rate Collector Handler.
 */
static void hrs_c_evt_handler(ble_hrs_c_t * p_hrs_c, ble_hrs_c_evt_t * p_hrs_c_evt)
{
    bool     success;
    uint32_t err_code;
    
    switch (p_hrs_c_evt->evt_type)
    {
        case BLE_HRS_C_EVT_DISCOVERY_COMPLETE:
            // Heart rate service discovered. Enable notification of Heart Rate Measurement.
            err_code = ble_hrs_c_hrm_notif_enable(p_hrs_c);
            APP_ERROR_CHECK(err_code);
            
            success = APPL_LCD_WRITE("Heart Rate", 10, LCD_UPPER_LINE, 0);
            APP_ERROR_CHECK_BOOL(success);
            break;
        
        case BLE_HRS_C_EVT_HRM_NOTIFICATION:
        {
            APPL_LOG("[APPL]: HR Measurement received %d \r\n", p_hrs_c_evt->params.hrm.hr_value);

            char hr_as_string[sizeof(uint16_t)];

            sprintf(hr_as_string, "%d", p_hrs_c_evt->params.hrm.hr_value);

            success = APPL_LCD_WRITE(hr_as_string, strlen(hr_as_string), LCD_LOWER_LINE, 0);
            APP_ERROR_CHECK_BOOL(success);
            break;
        }
        default:
            break;
    }
}


/**
 * @brief Heart rate collector initialization.
 */
static void hrs_c_init(void)
{
    ble_hrs_c_init_t hrs_c_init_obj;

    hrs_c_init_obj.evt_handler = hrs_c_evt_handler;

    uint32_t err_code = ble_hrs_c_init(&m_ble_hrs_c, &hrs_c_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Database discovery collector initialization.
 */
static void db_discovery_init(void)
{
    ble_db_discovery_init_t db_discovery_init_obj;

    uint32_t err_code = ble_db_discovery_init(&db_discovery_init_obj);
    APP_ERROR_CHECK(err_code);
}


int main(void)
{
    // Initialization of various modules.
    debug_init();
    leds_init();
    nrf6350_init();    
    ble_stack_init();
    connection_manager_init();
    db_discovery_init();
    hrs_c_init();
    
    // Start scanning for peripherals with Heart Rate UUID are found.
    connection_manager_start();
    
    for (;;)
    {
        power_manage();
    }
}


