/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

#include <stdint.h>
#include <string.h>
#include "nrf.h"
#include "nordic_common.h"
#include "ble.h"
#include "ble_gap.h"

#include "app_error.h"
#include "app_timer.h"
#include "app_gpiote.h"
#include "app_button.h"
#include "ble_advdata.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "nrf_sdm.h"
#include "ble_debug_assert_handler.h"
#include "softdevice_handler.h"

/* Evaluation kit setup. */
#define SEND_NOTIFICATION_BUTTON_PIN  (BUTTON_0)

#define DEVICE_NAME                     "Multilink"                                 /**< Name of device. Will be included in the advertising data. */

#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS            1                                           /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                           /**< Size of timer operation queues. */

#define APP_GPIOTE_MAX_USERS            1                                           /**< Maximum number of users of the GPIOTE handler. */

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)    /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define APP_ADV_INTERVAL                MSEC_TO_UNITS(50, UNIT_0_625_MS)            /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                         /**< The advertising timeout (in units of seconds). */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(500, UNIT_1_25_MS)            /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(1000, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds). */

#define MULTILINK_PERIPHERAL_BASE_UUID    {0xB3, 0x58, 0x55, 0x40, 0x50, 0x60, 0x11, 0xe3, 0x8f, 0x96, 0x08, 0x00, 0x0, 0x0, 0x9a, 0x66}
#define MULTILINK_PERIPHERAL_SERVICE_UUID 0x9001
#define MULTILINK_PERIPHERAL_CHAR_UUID    0x900A

static uint16_t                 m_conn_handle = BLE_CONN_HANDLE_INVALID;
static ble_gatts_char_handles_t m_char_handles;
static uint8_t                  m_base_uuid_type;


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
    nrf_gpio_pin_set(LED_0);
    nrf_gpio_pin_set(LED_1);

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

/**@brief Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by this application.
 */
static void leds_init(void)
{
    nrf_gpio_cfg_output(LED_0);
    nrf_gpio_cfg_output(LED_1);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void timers_init(void)
{
    // Initialize timer module, making it use the scheduler
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME, 
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);  

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    uint8_t       flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    
    
    // Build and set advertising data
    memset(&advdata, 0, sizeof(advdata));
    
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = false;
    advdata.flags.size              = sizeof(flags);
    advdata.flags.p_data            = &flags;
    advdata.uuids_complete.uuid_cnt = 0;
    
    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t             err_code;
    ble_gap_adv_params_t adv_params;
    
    // Start advertising
    memset(&adv_params, 0, sizeof(adv_params));
    
    adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
    adv_params.p_peer_addr = NULL;
    adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    adv_params.interval    = APP_ADV_INTERVAL;
    adv_params.timeout     = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = sd_ble_gap_adv_start(&adv_params);
    APP_ERROR_CHECK(err_code);
    
    nrf_gpio_pin_set(LED_0);
}


/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the multilink custom service.
 */
static void services_init(void)
{
    uint32_t                    err_code;
    ble_uuid_t                  uuid;
    ble_gatts_char_md_t         char_md;
    ble_gatts_attr_t            attr;
    ble_gatts_attr_md_t         attr_md;
    ble_gatts_attr_md_t         cccd_md;
    ble_gatts_attr_md_t         char_ud_md;
    uint16_t                    svc_test;

    static uint8_t              multilink_peripheral_data;
    static uint8_t              multilink_peripheral_ud[] = "Modifiable multilink_peripheral Data";

    ble_uuid128_t base_uuid = MULTILINK_PERIPHERAL_BASE_UUID;
    
    err_code = sd_ble_uuid_vs_add(&base_uuid, &m_base_uuid_type);
    APP_ERROR_CHECK(err_code);
    
    uuid.type = m_base_uuid_type;
    
    uuid.uuid = MULTILINK_PERIPHERAL_SERVICE_UUID;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &uuid, &svc_test);
    APP_ERROR_CHECK(err_code);

    uuid.uuid = MULTILINK_PERIPHERAL_CHAR_UUID;

    memset(&attr,0,sizeof(ble_gatts_attr_t));
    attr.p_uuid    = &uuid;
    attr.p_attr_md = &attr_md;
    attr.max_len   = 1;
    attr.p_value   = &multilink_peripheral_data;
    attr.init_len  = sizeof(multilink_peripheral_data);

    memset(&attr_md, 0, sizeof(ble_gatts_attr_md_t));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    attr_md.vlen = 0;

    memset(&cccd_md, 0, sizeof(ble_gatts_attr_md_t));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0,sizeof(ble_gatts_char_md_t));
    char_md.p_cccd_md               = &cccd_md;
    char_md.char_props.notify       = 1;
    char_md.char_props.indicate     = 1;
    char_md.char_props.read         = 1;
    char_md.char_props.write        = 1;
    char_md.char_ext_props.wr_aux   = 1;
    char_md.p_user_desc_md          = &char_ud_md;
    char_md.p_char_user_desc        = multilink_peripheral_ud;
    char_md.char_user_desc_size     = (uint8_t)strlen((char *)multilink_peripheral_ud);
    char_md.char_user_desc_max_size = (uint8_t)strlen((char *)multilink_peripheral_ud);

    memset(&char_ud_md, 0, sizeof(ble_gatts_attr_md_t));
    char_ud_md.vloc = BLE_GATTS_VLOC_STACK;
    char_ud_md.vlen = 1;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&char_ud_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&char_ud_md.write_perm);

    err_code = sd_ble_gatts_characteristic_add(BLE_GATT_HANDLE_INVALID, &char_md, &attr, &m_char_handles);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */    
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t err_code;  

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            nrf_gpio_pin_clear(LED_0);
            nrf_gpio_pin_clear(LED_1);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle    = BLE_CONN_HANDLE_INVALID;
            nrf_gpio_pin_clear(LED_1);

            advertising_start();
            break;

        case BLE_GATTS_EVT_WRITE:
            if ((p_ble_evt->evt.gatts_evt.params.write.handle == m_char_handles.cccd_handle) && (p_ble_evt->evt.gatts_evt.params.write.len == 2))
            {
                nrf_gpio_pin_set(LED_1);
            }
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0);
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
    on_ble_evt(p_ble_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
/*    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM,
                           BLE_L2CAP_MTU_DEF,
                           ble_evt_dispatch,
                           false);
  */

    uint32_t err_code;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

}

/**@brief Function for handling button presses
 */
static void button_event_handler(uint8_t pin_no)
{
    switch (pin_no)
    {
        case SEND_NOTIFICATION_BUTTON_PIN:
            if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
            {
                static uint8_t value = 0;

                ble_gatts_hvx_params_t hvx_params;
                uint16_t len = sizeof(uint8_t);

                value = (value == 0) ? 1 : 0;

                memset(&hvx_params, 0, sizeof(hvx_params));

                hvx_params.handle   = m_char_handles.value_handle;
                hvx_params.type     = BLE_GATT_HVX_NOTIFICATION;
                hvx_params.offset   = 0;
                hvx_params.p_len    = &len;
                hvx_params.p_data   = &value;

                (void)sd_ble_gatts_hvx(m_conn_handle, &hvx_params);

                if (value == 0)
                {
                    nrf_gpio_pin_clear(LED_0);
                }
                else
                {
                    nrf_gpio_pin_set(LED_0);
                }
            }
            break;
        
        // Handle any other buttons
            
        default:
            APP_ERROR_HANDLER(pin_no);
    }
}


/**@brief Function for initializing the GPIOTE handler module.
 */
static void gpiote_init(void)
{
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
}


/**@brief Function for initializing the button handler module.
 */
static void buttons_init(void)
{
    uint32_t err_code;
    
    // Note: Array must be static because a pointer to it will be saved in the Button handler
    //       module.
    static app_button_cfg_t buttons[] =
    {
        {SEND_NOTIFICATION_BUTTON_PIN, false, NRF_GPIO_PIN_PULLUP, button_event_handler},
    };
    
    APP_BUTTON_INIT(buttons, sizeof(buttons) / sizeof(buttons[0]), BUTTON_DETECTION_DELAY, false);
    
    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for application main entry.
 */
int main(void)
{
    ble_stack_init();
    leds_init();
    timers_init();
    gpiote_init();
    buttons_init();
    gap_params_init();
    advertising_init();
    services_init();

    advertising_start();
    
    for (;;)
    {
        power_manage();
    }
}
