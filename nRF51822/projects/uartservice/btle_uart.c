/**************************************************************************/
/*!
    @file     btle_uart.c
    @author   hathach (tinyusb.org), K. Townsend (microBuilder.eu)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2013, K. Townsend (microBuilder.eu)
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
#include "btle_uart.h"
#include "custom_helper.h"
#include "ble_srv_common.h"

typedef struct
{
  uint16_t                  service_handle;
  uint16_t                  conn_handle;
  uint8_t                   uuid_type;
  ble_gatts_char_handles_t  in_handle;
  ble_gatts_char_handles_t  out_handle;
  bool                      is_indication_waiting;
} uart_srvc_t;

static uart_srvc_t m_uart_srvc;

/**************************************************************************/
/*!
    @brief      Initialises the UART service, adding it to the SoftDevice
                stack, assigning the UUID and characteristics, etc.

    @returns
    @retval     ERROR_NONE        Everything executed normally.
    @retval     ERROR_NOT_FOUND   Failed adding the base UUID
*/
/**************************************************************************/
error_t uart_service_init(uint8_t uuid_base_type)
{
  memclr_(&m_uart_srvc, sizeof(uart_srvc_t));
  m_uart_srvc.conn_handle = BLE_CONN_HANDLE_INVALID;
  m_uart_srvc.uuid_type = uuid_base_type;

  /* Add the primary service first ... */
  ble_uuid_t ble_uuid =
  {
     .type = m_uart_srvc.uuid_type,
     .uuid = BLE_UART_UUID_PRIMARY_SERVICE
  };
  ASSERT_STATUS( sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_uart_srvc.service_handle) );

  /* ... then add the individual characteristics */
  ble_uuid.uuid = BLE_UART_UUID_IN;
  ASSERT_STATUS(custom_add_in_characteristic(m_uart_srvc.service_handle,
                                             &ble_uuid, (ble_gatt_char_props_t) {.indicate = 1},
                                             NULL, 1, BLE_UART_MAX_LENGTH,
                                             &m_uart_srvc.in_handle) );

  ble_uuid.uuid = BLE_UART_UUID_OUT;
  ASSERT_STATUS(custom_add_in_characteristic(m_uart_srvc.service_handle,
                                             &ble_uuid, (ble_gatt_char_props_t) {.write = 1},
                                             NULL, 1, BLE_UART_MAX_LENGTH,
                                             &m_uart_srvc.out_handle) );

  return ERROR_NONE;
}

/**************************************************************************/
/*!
    @brief      The service handler, which will be called every time
                a BLE event arrives.  Anything particular to this service
                should be handled here instead of in the higher level
                generic BLE event handler.

    @param[in]  p_ble_evt   A pointer to the BLE event data
*/
/**************************************************************************/
void uart_service_handler(ble_evt_t * p_ble_evt)
{
  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
      m_uart_srvc.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    break;

    case BLE_GAP_EVT_DISCONNECTED:
      m_uart_srvc.conn_handle = BLE_CONN_HANDLE_INVALID;
    break;

    /* Capture the 'indicate' confirmation from the central here */
    case BLE_GATTS_EVT_HVC:
      if( m_uart_srvc.in_handle.value_handle == p_ble_evt->evt.gatts_evt.params.hvc.handle )
      {
        /* Clear the flag and fire the indicate callback with success */
        m_uart_srvc.is_indication_waiting = false;
        uart_service_indicate_callback(true);
      }
    break;

    /* The is no attribute handle in timeout events, so we need to use   */
    /* is_indication_waiting to know if the timeout is from this service */
    case BLE_GATTS_EVT_TIMEOUT:
      if ( m_uart_srvc.is_indication_waiting )
      {
        /* Clear the flag and fire the indicate callback with failure */
        m_uart_srvc.is_indication_waiting = false;
        uart_service_indicate_callback(false);
      }
    break;

    /* Handle incoming data on the RXD characteristic */
    case BLE_GATTS_EVT_WRITE:
    {
      ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
      if ( p_evt_write->handle == m_uart_srvc.out_handle.value_handle )
      {
        if (uart_service_received_callback)
        {
          uart_service_received_callback(p_evt_write->data, p_evt_write->len);
        }
      }
    }
    break;

    default:
    break;
  }
}

/**************************************************************************/
/*!
    @brief      Helper function to send data out via the UART service,
                using the service's 'IN' characteristic as the carrier.

    @param[in]  p_data    Pointer to the buffer of data to transmit
    @param[in]  length    The number of bytes in the buffer
    
    @returns
    @retval     ERROR_NONE            Everything executed normally
                ERROR_INVALID_PARAM   Length exceeds the maximum size
                                      defined by BLE_UART_MAX_LENGTH
*/
/**************************************************************************/
error_t uart_service_send(uint8_t p_data[], uint16_t length)
{
  ASSERT( m_uart_srvc.conn_handle != BLE_CONN_HANDLE_INVALID, ERROR_INVALID_STATE);
  ASSERT( length <= BLE_UART_MAX_LENGTH, ERROR_INVALID_PARAM);

  ble_gatts_hvx_params_t hvx_params =
  {
      .handle = m_uart_srvc.in_handle.value_handle,
      .type   = BLE_GATT_HVX_INDICATION,
      .p_data = p_data,
      .p_len  = &length,
  };

  m_uart_srvc.is_indication_waiting = true;
  ASSERT_STATUS( sd_ble_gatts_hvx(m_uart_srvc.conn_handle, &hvx_params) );

  return ERROR_NONE;
}

#if BLE_UART_BRIDGE
/**************************************************************************/
/*!
    @brief      Callback function for the UART bridge to send incoming data
                (arriving over the air) out to the HW UART interface

    @param[in]  data    Pointer to the data received
    @param[in]  length  Number of bytes received
*/
/**************************************************************************/
void uart_service_received_callback(uint8_t * data, uint16_t length)
{
  for(uint16_t i=0; i<length; i++)
  {
    app_uart_put( data[i] );
  }
}
#endif

/**************************************************************************/
/*!
    @brief  This callback fires every time an 'indicate' passes or fails
            in the UART service
*/
/**************************************************************************/
void uart_service_indicate_callback(bool is_succeeded)
{
  if ( is_succeeded )
  {
    printf("confirmation received\n");
  }
  else
  {
    printf("confirmation timeout\n");
  }
}

/**************************************************************************/
/*!
    @brief      Task handler when the UART bridge functionality is enabled,
                reading data from the HW UART interface to transmit over the
                air.

    @param[in]  p_context
*/
/**************************************************************************/
void uart_service_bridge_task(void* p_context)
{
  uint8_t buffer[BLE_UART_MAX_LENGTH];
  uint8_t i=0;
  while ( NRF_SUCCESS == app_uart_get(&buffer[i]) && i < BLE_UART_MAX_LENGTH )
  {
    i++;
  }

  if( i > 0)
  {
    (void) uart_service_send(buffer, i);
  }
}
