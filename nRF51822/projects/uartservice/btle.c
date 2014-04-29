/**************************************************************************/
/*!
    @file     btle.c
*/
/**************************************************************************/

#include "common/common.h"

/* ---------------------------------------------------------------------- */
/* INCLUDE                                                                */
/* ---------------------------------------------------------------------- */
#include "boards/board.h"
#include "btle.h"

#include "nordic_common.h"
#include "pstorage.h"
#include "softdevice_handler.h"
#include "ble_radio_notification.h"
#include "ble_flash.h"
#include "ble_bondmngr.h"
#include "ble_conn_params.h"

#include "btle_gap.h"
#include "btle_advertising.h"
#include "custom_helper.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
// standard service UUID to driver offset
#define UUID2OFFSET(uuid)   ((uuid)-0x1800)

btle_service_driver_t const btle_service_driver[] =
{
    #if CFG_BLE_DEVICE_INFORMATION
    [UUID2OFFSET(BLE_UUID_DEVICE_INFORMATION_SERVICE)] =
    {
        .init          = device_information_init,
        .event_handler = NULL,
    },
    #endif

    #if CFG_BLE_BATTERY
    [UUID2OFFSET(BLE_UUID_BATTERY_SERVICE)] =
    {
        .init          = battery_init,
        .event_handler = battery_handler,
    },
    #endif

    #if CFG_BLE_HEART_RATE
//    [UUID2OFFSET(BLE_UUID_HEART_RATE_SERVICE)] =
//    {
//        .init          = heart_rate_init,
//        .event_handler = heart_rate_handler,
//    },
    #endif

    #if CFG_BLE_IMMEDIATE_ALERT
    [UUID2OFFSET(BLE_UUID_IMMEDIATE_ALERT_SERVICE)] =
    {
        .init          = immediate_alert_init,
        .event_handler = immediate_alert_handler,
    },
    #endif

    #if CFG_BLE_TX_POWER
    [UUID2OFFSET(BLE_UUID_TX_POWER_SERVICE)] =
    {
        .init          = tx_power_init,
        .event_handler = NULL,
    },
    #endif

    #if CFG_BLE_LINK_LOSS
    [UUID2OFFSET(BLE_UUID_LINK_LOSS_SERVICE)] =
    {
        .init          = link_loss_init,
        .event_handler = link_loss_handler,
    },
    #endif
};

enum {
  BTLE_SERVICE_MAX = sizeof(btle_service_driver) / sizeof(btle_service_driver_t)
};

btle_service_custom_driver_t btle_service_custom_driver[] =
{
    {
        .uuid_base         = BLE_UART_UUID_BASE,
        .service_uuid.uuid = BLE_UART_UUID_PRIMARY_SERVICE,
        .init              = uart_service_init,
        .event_handler     = uart_service_handler
    },
};

enum {
  BTLE_SERVICE_CUSTOM_MAX = sizeof(btle_service_custom_driver) / sizeof(btle_service_custom_driver_t)
};

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
static void service_error_callback(uint32_t nrf_error);
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name);
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name);

static error_t bond_manager_init(void);

static void btle_handler(ble_evt_t * p_ble_evt);
static void btle_soc_event_handler(uint32_t sys_evt);

/**************************************************************************/
/*!
    @brief      Initialises BTLE and the underlying HW/SoftDevice

    @returns
*/
/**************************************************************************/
error_t btle_init(void)
{
  SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false); // Initialize the SoftDevice handler module.
  ASSERT_STATUS( softdevice_ble_evt_handler_set(btle_handler) );
  ASSERT_STATUS( softdevice_sys_evt_handler_set(btle_soc_event_handler) ); // TODO look into detail later

  bond_manager_init();
  btle_gap_init();

  /*------------- Standard Services -------------*/
  for(uint16_t i=0; i<BTLE_SERVICE_MAX; i++)
  {
    if ( btle_service_driver[i].init != NULL )
    {
      ASSERT_STATUS( btle_service_driver[i].init() );
    }
  }

  /*------------- Custom Services -------------*/
  for(uint16_t i=0; i<BTLE_SERVICE_CUSTOM_MAX; i++)
  {
    if ( btle_service_custom_driver[i].init != NULL )
    {
      /* add the custom UUID to stack */
      uint8_t uuid_type = custom_add_uuid_base(btle_service_custom_driver[i].uuid_base);
      ASSERT( uuid_type >= BLE_UUID_TYPE_VENDOR_BEGIN, ERROR_INVALIDPARAMETER);

      btle_service_custom_driver[i].service_uuid.type = uuid_type; /* store for later use in advertising */
      ASSERT_STATUS( btle_service_custom_driver[i].init(uuid_type) );
    }
  }

  btle_advertising_init(btle_service_driver, BTLE_SERVICE_MAX, btle_service_custom_driver, BTLE_SERVICE_CUSTOM_MAX);
  btle_advertising_start();

  return ERROR_NONE;
}

static void btle_soc_event_handler(uint32_t sys_evt)
{
  pstorage_sys_event_handler(sys_evt);
}

/**************************************************************************/
/*!
    @brief

    @param[in]  p_ble_evt
    
    @returns
*/
/**************************************************************************/
static void btle_handler(ble_evt_t * p_ble_evt)
{
  //------------- library service handler -------------//
  ble_bondmngr_on_ble_evt(p_ble_evt);
  ble_conn_params_on_ble_evt(p_ble_evt);

  /*------------- Standard Service Handler -------------*/
  for(uint16_t i=0; i<BTLE_SERVICE_MAX; i++)
  {
    if ( btle_service_driver[i].event_handler != NULL )
    {
      btle_service_driver[i].event_handler(p_ble_evt);
    }
  }

  /*------------- Custom Service Handler -------------*/
  for(uint16_t i=0; i<BTLE_SERVICE_CUSTOM_MAX; i++)
  {
    if ( btle_service_custom_driver[i].event_handler != NULL )
    {
      btle_service_custom_driver[i].event_handler(p_ble_evt);
    }
  }

  /*------------- Application Specific Handler (modify to your own need) -------------*/
  static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
      m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      ASSERT_STATUS_RET_VOID( app_button_enable() );
    break;

    case BLE_GAP_EVT_DISCONNECTED:
      // Since we are not in a connection and have not started advertising, store bonds
      ASSERT_STATUS_RET_VOID ( ble_bondmngr_bonded_centrals_store() );
      m_conn_handle = BLE_CONN_HANDLE_INVALID;
      btle_advertising_start(); // TODO use a power-efficient advertising scheme (fast, slow, sleep)
    break;

    case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
    {
      ble_gap_sec_params_t sec_params =
      {
          .timeout      = 30                   , /**< Timeout for Pairing Request or Security Request (in seconds). */
          .bond         = 1                    , /**< Perform bonding. */
          .mitm         = 0                    , /**< Man In The Middle protection not required. */
          .io_caps      = BLE_GAP_IO_CAPS_NONE , /**< No I/O capabilities. */
          .oob          = 0                    , /**< Out Of Band data not available. */
          .min_key_size = 7                    , /**< Minimum encryption key size. */
          .max_key_size = 16                     /**< Maximum encryption key size. */
      };
      ASSERT_STATUS_RET_VOID ( sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_SUCCESS, &sec_params) );
    }
    break;

    case BLE_GAP_EVT_TIMEOUT:
      if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT)
      {
        btle_advertising_start(); // TODO use a power-efficient advertising scheme (fast, slow, sleep)
      }
    break;

    case BLE_GATTC_EVT_TIMEOUT:
    case BLE_GATTS_EVT_TIMEOUT:
      // Disconnect on GATT Server and Client timeout events.
//      ASSERT_STATUS_RET_VOID ( sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION) ); TODO find in proximity example
    break;

    default: break;
  }
}

/**************************************************************************/
/*!
    @brief      Initialises the bond manager

    @returns
*/
/**************************************************************************/
static error_t bond_manager_init(void)
{
  // Initialize persistent storage module.
  ASSERT_STATUS( pstorage_init() );

  // Will clear bond data on reset if bond delete button is pressed at init (reset)
  ble_bondmngr_init_t bond_para =
  {
      .flash_page_num_bond     = CFG_BLE_BOND_FLASH_PAGE_BOND                     ,
      .flash_page_num_sys_attr = CFG_BLE_BOND_FLASH_PAGE_SYS_ATTR                 ,
      .bonds_delete            = boardButtonCheck(CFG_BLE_BOND_DELETE_BUTTON_NUM) ,
      .evt_handler             = NULL                                             ,
      .error_handler           = service_error_callback
  };

  ASSERT_STATUS( ble_bondmngr_init( &bond_para ) );

  return ERROR_NONE;
}

/**************************************************************************/
/*!
    @brief
    @param[in]  nrf_error
    @returns
*/
/**************************************************************************/
static void service_error_callback(uint32_t nrf_error)
{
  ASSERT_STATUS_RET_VOID( nrf_error );
}

/**************************************************************************/
/*!
    @brief      Callback when an error occurs inside the SoftDevice

    @param[in]  line_num
    @param[in]  p-file_name

    @returns
*/
/**************************************************************************/
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
  ASSERT(false, (void) 0);
}

/**************************************************************************/
/*!
    @brief      Handler for general errors above the SoftDevice layer

    @param[in]  error_code
    @param[in]  line_num
    @param[in]  p-file_name

    @returns
*/
/**************************************************************************/
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
  ASSERT_STATUS_RET_VOID( error_code );
  NVIC_SystemReset();
}


