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

btle_service_t btle_service[] =
{
    {
        .uuid       = BLE_UUID_HEART_RATE_SERVICE,
        .char_count = 2,
        .char_pool  =
        {
            {
                .uuid = BLE_UUID_HEART_RATE_MEASUREMENT_CHAR,
                .properties = { .notify = 1 },
                .len_min = 2,
                .len_max = 2
            },
            {
                .uuid = BLE_UUID_BODY_SENSOR_LOCATION_CHAR,
                .properties = { .read = 1 },
                .len_min = 1,
                .len_max = 1,
                .init_value = (uint8_t []) { 3 }
            }
        }
    }
};

enum {
  SERVICE_COUNT = sizeof(btle_service) / sizeof(btle_service_t)
};

btle_service_custom_driver_t btle_service_custom_driver[] =
{
    {
        .uuid_base         = CFG_BLE_UART_UUID_BASE,
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

static inline bool is_all_zeros(uint8_t arr[], uint32_t count) ATTR_ALWAYS_INLINE ATTR_PURE;
static inline bool is_all_zeros(uint8_t arr[], uint32_t count)
{
  for(uint32_t i=0; i<count; i++) if (arr[i] != 0) return false;

  return true;
}

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

  for(uint8_t sid=0; sid<SERVICE_COUNT; sid++)
  {
    btle_service_t * p_service = &btle_service[sid];

    // add the custom UUID to stack if it is non standard service
    if ( is_all_zeros(p_service->uuid_base, 16) )
    {
      p_service->uuid_type = BLE_UUID_TYPE_BLE; // standard type
    }
    else
    {
      p_service->uuid_type = custom_add_uuid_base(p_service->uuid_base);
      ASSERT( p_service->uuid_type >= BLE_UUID_TYPE_VENDOR_BEGIN, ERROR_INVALIDPARAMETER);
    }

    /*------------- Add primary service -------------*/
    ble_uuid_t service_uuid = { .type = p_service->uuid_type, .uuid = p_service->uuid };
    ASSERT_STATUS( sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &p_service->handle) );

    /*------------- Add all characteristics -------------*/
    for(uint8_t cid=0; cid<p_service->char_count; cid++)
    {
      btle_characteristic_t * p_char = &p_service->char_pool[cid];
      ble_uuid_t char_uuid = { .type = p_service->uuid_type, .uuid = p_char->uuid };

      ASSERT_STATUS(custom_add_in_characteristic(p_service->handle, &char_uuid, p_char->properties,
                                                 p_char->init_value, p_char->len_min, p_char->len_max,
                                                 &p_char->handle) );
    }
  }

  /*------------- Custom Services -------------*/
//  for(uint16_t i=0; i<BTLE_SERVICE_CUSTOM_MAX; i++)
//  {
//    if ( btle_service_custom_driver[i].init != NULL )
//    {
//      /* add the custom UUID to stack */
//      uint8_t uuid_type = custom_add_uuid_base(btle_service_custom_driver[i].uuid_base);
//      ASSERT( uuid_type >= BLE_UUID_TYPE_VENDOR_BEGIN, ERROR_INVALIDPARAMETER);
//
//      btle_service_custom_driver[i].service_uuid.type = uuid_type; /* store for later use in advertising */
//      ASSERT_STATUS( btle_service_custom_driver[i].init(uuid_type) );
//    }
//  }
//  btle_advertising_init(NULL, 0, btle_service_custom_driver, BTLE_SERVICE_CUSTOM_MAX);

  btle_advertising_init(NULL, 0, NULL, 0);
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
  btle_gap_handler(p_ble_evt);
  ble_bondmngr_on_ble_evt(p_ble_evt);
  ble_conn_params_on_ble_evt(p_ble_evt);

  /*------------- Standard Service Handler -------------*/
//  for(uint16_t i=0; i<BTLE_SERVICE_MAX; i++)
//  {
//    if ( btle_service_driver[i].event_handler != NULL )
//    {
//      btle_service_driver[i].event_handler(p_ble_evt);
//    }
//  }

  /*------------- Custom Service Handler -------------*/
//  for(uint16_t i=0; i<BTLE_SERVICE_CUSTOM_MAX; i++)
//  {
//    if ( btle_service_custom_driver[i].event_handler != NULL )
//    {
//      btle_service_custom_driver[i].event_handler(p_ble_evt);
//    }
//  }

  /*------------- Application Specific Handler (modify to your own need) -------------*/
  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
      // blink some LED when connected
    break;

    case BLE_GAP_EVT_DISCONNECTED:
      // Since we are not in a connection and have not started advertising, store bonds
      ASSERT_STATUS_RET_VOID ( ble_bondmngr_bonded_centrals_store() );
      btle_advertising_start();
    break;

    case BLE_GAP_EVT_TIMEOUT:
      if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT)
      {
        btle_advertising_start();
      }
    break;

    case BLE_GATTC_EVT_TIMEOUT:
    case BLE_GATTS_EVT_TIMEOUT:
      // Disconnect on GATT Server and Client timeout events.
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


