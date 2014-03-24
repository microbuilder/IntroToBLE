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

/** @file
 *
 * @defgroup ble_sdk_app_ibeacon_main main.c
 * @{
 * @ingroup ble_sdk_app_ibeacon
 * @brief iBeacon Transmitter Sample Application main file.
 *
 * This file contains the source code for an iBeacon transmitter sample application.
 */

#include <stdbool.h>
#include <stdint.h>
#include "ble_advdata.h"
#include "boards.h"
#include "nordic_common.h"
#include "softdevice_handler.h"

#define ADVERTISING_LED_PIN_NO        LED_0                             /**< Is on when device is advertising. */
#define ASSERT_LED_PIN_NO             LED_7                             /**< Is on when application has asserted. */

#define APP_CFG_NON_CONN_ADV_TIMEOUT  0                                 /**< Time for which the device must be advertising in non-connectable mode (in seconds). 0 disables timeout. */
#define NON_CONNECTABLE_ADV_INTERVAL  MSEC_TO_UNITS(100, UNIT_0_625_MS) /**< The advertising interval for non-connectable advertisement (100 ms). This value can vary between 100ms to 10.24s). */

#define APP_CLBEACON_INFO_LENGTH      0x17                              /**< Total length of information advertised by the iBeacon. */
#define APP_ADV_DATA_LENGTH           0x15                              /**< Length of manufacturer specific data in the advertisement. */
#define APP_DEVICE_TYPE               0x02                              /**< 0x02 refers to iBeacon. */
#define APP_MEASURED_RSSI             0xC3                              /**< The iBeacon's measured RSSI at 1 meter distance in dBm. */
#define APP_COMPANY_IDENTIFIER        0x004C                            /**< Company identifier for Apple Inc. as per www.bluetooth.org. */
#define APP_MAJOR_VALUE               0x01, 0x02                        /**< Major value used to identify iBeacons. */ 
#define APP_MINOR_VALUE               0x03, 0x04                        /**< Minor value used to identify iBeacons. */ 
#define APP_IBEACON_UUID              0x01, 0x12, 0x23, 0x34, \
                                      0x45, 0x56, 0x67, 0x78, \
                                      0x89, 0x9a, 0xab, 0xbc, \
                                      0xcd, 0xde, 0xef, 0xf0            /**< Proprietary UUID for iBeacon. */

#define DEAD_BEEF                     0xDEADBEEF                        /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

static ble_gap_adv_params_t m_adv_params;                               /**< Parameters to be passed to the stack when starting advertising. */
static uint8_t clbeacon_info[APP_CLBEACON_INFO_LENGTH] =                /**< Information advertised by the iBeacon. */
{
    APP_DEVICE_TYPE,     // Manufacturer specific information. Specifies the device type in this 
                         // implementation. 
    APP_ADV_DATA_LENGTH, // Manufacturer specific information. Specifies the length of the 
                         // manufacturer specific data in this implementation.
    APP_IBEACON_UUID,    // 128 bit UUID value. 
    APP_MAJOR_VALUE,     // Major arbitrary value that can be used to distinguish between iBeacons. 
    APP_MINOR_VALUE,     // Minor arbitrary value that can be used to distinguish between iBeacons. 
    APP_MEASURED_RSSI    // Manufacturer specific information. The iBeacon's measured TX power in 
                         // this implementation. 
};

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
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by this application.
 */
static void leds_init(void)
{
    nrf_gpio_cfg_output(ADVERTISING_LED_PIN_NO);
    nrf_gpio_cfg_output(ASSERT_LED_PIN_NO);    
}


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t        err_code;
    ble_advdata_t   advdata;
    uint8_t         flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

    ble_advdata_manuf_data_t manuf_specific_data;
    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;
    manuf_specific_data.data.p_data        = (uint8_t *) clbeacon_info;
    manuf_specific_data.data.size          = APP_CLBEACON_INFO_LENGTH;

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_NO_NAME;
    advdata.flags.size              = sizeof(flags);
    advdata.flags.p_data            = &flags;
    advdata.p_manuf_specific_data   = &manuf_specific_data;

    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);

    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    m_adv_params.type        = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND;
    m_adv_params.p_peer_addr = NULL;                             // Undirected advertisement.
    m_adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval    = NON_CONNECTABLE_ADV_INTERVAL;
    m_adv_params.timeout     = APP_CFG_NON_CONN_ADV_TIMEOUT;
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code;

    err_code = sd_ble_gap_adv_start(&m_adv_params);
    APP_ERROR_CHECK(err_code);

    nrf_gpio_pin_set(ADVERTISING_LED_PIN_NO);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);
}


/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Function for application main entry.
 */
int main(void)
{
    // Initialize.
    leds_init();
    ble_stack_init();
    advertising_init();

    // Start execution.
    advertising_start();

    // Enter main loop.
    for (;;)
    {
        power_manage();
    }
}

/**
 * @}
 */
