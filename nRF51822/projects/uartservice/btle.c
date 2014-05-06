/**************************************************************************/
/*!
    @file     btle.c
    @author   hathach (tinyusb.org)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2014, K. Townsend (microBuilder.eu)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/
#include "common/common.h"

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
#include "btle_uart.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
btle_service_driver_t btle_service_list[] =
{
    {
        .uuid_base     = BLE_UART_UUID_BASE,
        .uuid16        = BLE_UART_UUID_PRIMARY_SERVICE,
        .init          = uart_service_init,
        .event_handler = uart_service_handler
    },
};

enum {
  BTLE_SERVICE_COUNT = sizeof(btle_service_list) / sizeof(btle_service_driver_t)
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
    Checks is all values in the supplied array are zero

    @returns  'true' if all values in the array are zero
              'false' if any value is non-zero
*/
/**************************************************************************/
static inline bool is_all_zeros(uint8_t arr[], uint32_t count) ATTR_ALWAYS_INLINE ATTR_PURE;
static inline bool is_all_zeros(uint8_t arr[], uint32_t count)
{
  for ( uint32_t i = 0; i < count; i++)
  {
    if (arr[i] != 0 ) return false;
  }

  return true;
}

/**************************************************************************/
/*!
    Initialises BTLE and the underlying HW/SoftDevice
*/
/**************************************************************************/
error_t btle_init(void)
{
  /* Initialise the SoftDevice using an external 32kHz XTAL for LFCLK */
  SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);
  
  /* Setup the event handler callbacks to point to functions in this file */
  ASSERT_STATUS( softdevice_ble_evt_handler_set( btle_handler ) );
  ASSERT_STATUS( softdevice_sys_evt_handler_set( btle_soc_event_handler ) );

  /* Initialise the bond manager (holds stored bond data, etc.) */
  bond_manager_init();
  
  /* Initialise GAP */
  btle_gap_init();

  /* Initialise Services */
  for(uint16_t i=0; i<BTLE_SERVICE_COUNT; i++)
  {
    btle_service_driver_t* const p_service = &btle_service_list[i];

    /* If we are using a custom UUID we first need to add it to the stack */
    if ( is_all_zeros(p_service->uuid_base, 16) )
    {
      /* Seems to be a standard 16-bit BLE UUID */
      p_service->uuid_type = BLE_UUID_TYPE_BLE;
    }
    else
    {
      /* Seems to be a custom UUID, which needs to be added */
      p_service->uuid_type = custom_add_uuid_base( p_service->uuid_base );
      ASSERT( p_service->uuid_type >= BLE_UUID_TYPE_VENDOR_BEGIN, ERROR_INVALIDPARAMETER );
    }

    if ( p_service->init != NULL) ASSERT_STATUS( p_service->init(p_service->uuid_type) );
  }

  btle_advertising_init(btle_service_list, BTLE_SERVICE_COUNT);
  btle_advertising_start();

  return ERROR_NONE;
}


/**************************************************************************/
/*!
    Callback handler for SOC events from the SD
*/
/**************************************************************************/
static void btle_soc_event_handler(uint32_t sys_evt)
{
  pstorage_sys_event_handler(sys_evt);
}

/**************************************************************************/
/*!
    Callback handler for general events from the SD
*/
/**************************************************************************/
static void btle_handler(ble_evt_t * p_ble_evt)
{
  /* First call the library service event handlers */
  btle_gap_handler(p_ble_evt);
  ble_bondmngr_on_ble_evt(p_ble_evt);
  ble_conn_params_on_ble_evt(p_ble_evt);

  /* Service Handler */
  for(uint16_t i=0; i<BTLE_SERVICE_COUNT; i++)
  {
    if ( btle_service_list[i].event_handler != NULL )
    {
      btle_service_list[i].event_handler(p_ble_evt);
    }
  }

  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
      // ToDo: Blink an LED, etc.
      break;

    case BLE_GAP_EVT_DISCONNECTED:
      /* Since we are not in a connection and have not started advertising, store bonds */
      (void) ble_bondmngr_bonded_centrals_store();
      /* Start advertising again (change this if necessary!) */
      btle_advertising_start();
      break;

    case BLE_GAP_EVT_TIMEOUT:
      if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT)
      {
        /* Start advertising again (change this if necessary!) */
        btle_advertising_start();
      }
      break;

    case BLE_GATTC_EVT_TIMEOUT:
    case BLE_GATTS_EVT_TIMEOUT:
      /* ToDo: Disconnect on GATT Server and Client timeout events. */
      break;

    default: break;
  }
}

/**************************************************************************/
/*!
    Initialises the bond manager (used to store bonding data, etc.)

    @returns    ERROR_NONE if the update was successful, otherwise an
                appropriate error_t value
*/
/**************************************************************************/
static error_t bond_manager_init(void)
{
  /* Initialise the persistent storage module */
  ASSERT_STATUS( pstorage_init() );

  /* Clear bond data on reset if bond delete button is pressed at init (reset) */
  ble_bondmngr_init_t bond_para =
  {
      .flash_page_num_bond     = CFG_BLE_BOND_FLASH_PAGE_BOND                     ,
      .flash_page_num_sys_attr = CFG_BLE_BOND_FLASH_PAGE_SYS_ATTR                 ,
      .bonds_delete            = boardButtonCheck(CFG_BLE_BOND_DELETE_BUTTON_NUM) ,
      .evt_handler             = NULL                                             ,
      .error_handler           = service_error_callback
  };

  /* Initialise the bond manager */
  ASSERT_STATUS( ble_bondmngr_init( &bond_para ) );

  return ERROR_NONE;
}

/**************************************************************************/
/*!
    Callback handler for service errors

    @param[in]  nrf_error
*/
/**************************************************************************/
static void service_error_callback(uint32_t nrf_error)
{
  ASSERT_STATUS_RET_VOID( nrf_error );
}

/**************************************************************************/
/*!
    Callback handler for error that occur inside the SoftDevice

    @param[in]  line_num
    @param[in]  p-file_name
*/
/**************************************************************************/
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
  ASSERT(false, (void) 0);
}

/**************************************************************************/
/*!
    Callback handler for general errors above the SoftDevice layer

    @param[in]  error_code
    @param[in]  line_num
    @param[in]  p-file_name

    @returns
*/
/**************************************************************************/
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
  ASSERT_STATUS_RET_VOID( error_code );
  
  /* Perform a full system reset */
  NVIC_SystemReset();
}
