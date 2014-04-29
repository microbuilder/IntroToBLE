/**************************************************************************/
/*!
    @file     btle_gap.c
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

/* ---------------------------------------------------------------------- */
/* INCLUDE				                                                        */
/* ---------------------------------------------------------------------- */
#include "common/common.h"
#include "boards/board.h"

#include "ble_gap.h"
#include "ble_conn_params.h"

/* ---------------------------------------------------------------------- */
/* MACRO CONSTANT TYPEDEF                                                 */
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
/* INTERNAL OBJECT & FUNCTION DECLARATION                                 */
/* ---------------------------------------------------------------------- */
static inline uint32_t msec_to_1_25msec(uint32_t interval_ms) ATTR_ALWAYS_INLINE ATTR_CONST;
static void error_callback(uint32_t nrf_error);

/* ---------------------------------------------------------------------- */
/* IMPLEMENTATION											                                    */
/* ---------------------------------------------------------------------- */

/**************************************************************************/
/*!
    @brief      Initialise GAP in the underlying SoftDevice

    @returns
*/
/**************************************************************************/
error_t btle_gap_init(void)
{
  ble_gap_conn_params_t   gap_conn_params =
  {
      .min_conn_interval = msec_to_1_25msec(CFG_GAP_CONNECTION_MIN_INTERVAL_MS) , // in 1.25ms unit
      .max_conn_interval = msec_to_1_25msec(CFG_GAP_CONNECTION_MAX_INTERVAL_MS) , // in 1.25ms unit
      .slave_latency     = CFG_GAP_CONNECTION_SLAVE_LATENCY                     ,
      .conn_sup_timeout  = CFG_GAP_CONNECTION_SUPERVISION_TIMEOUT_MS / 10         // in 10ms unit
  };

  ble_gap_conn_sec_mode_t sec_mode;
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode); // no security is needed

  ASSERT_STATUS( sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *) CFG_GAP_LOCAL_NAME, strlen(CFG_GAP_LOCAL_NAME)) );
  ASSERT_STATUS( sd_ble_gap_appearance_set(CFG_GAP_APPEARANCE) );
  ASSERT_STATUS( sd_ble_gap_ppcp_set(&gap_conn_params) );
  ASSERT_STATUS( sd_ble_gap_tx_power_set(CFG_BLE_TX_POWER_LEVEL) );

  /*------------- Connection Parameter -------------*/
  enum {
    FIRST_UPDATE_DELAY = APP_TIMER_TICKS(5000, CFG_TIMER_PRESCALER),
    NEXT_UPDATE_DELAY  = APP_TIMER_TICKS(5000, CFG_TIMER_PRESCALER),
    MAX_UPDATE_COUNT   = 3
  };

  ble_conn_params_init_t cp_init =
  {
      .p_conn_params                  = NULL                    ,
      .first_conn_params_update_delay = FIRST_UPDATE_DELAY      ,
      .next_conn_params_update_delay  = NEXT_UPDATE_DELAY       ,
      .max_conn_params_update_count   = MAX_UPDATE_COUNT        ,
      .start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID ,
      .disconnect_on_fail             = true                    ,
      .evt_handler                    = NULL                    ,
      .error_handler                  = error_callback
  };

  ASSERT_STATUS ( ble_conn_params_init(&cp_init) );

  return ERROR_NONE;
}

void btle_gap_handler(ble_evt_t * p_ble_evt)
{
  static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
      m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    break;

    case BLE_GAP_EVT_DISCONNECTED:
      m_conn_handle = BLE_CONN_HANDLE_INVALID;
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

    default: break;
  }
}

/**************************************************************************/
/*!
    @brief      Converts msecs to an integer representing 1.25ms units

    @param[in]  ms
                The number of milliseconds to conver to 1.25ms units

    @returns    The number of 1.25ms units in the supplied number of ms
*/
/**************************************************************************/
static inline uint32_t msec_to_1_25msec(uint32_t interval_ms)
{
  return (interval_ms * 4) / 5 ;
}

static void error_callback(uint32_t nrf_error)
{
  ASSERT_STATUS_RET_VOID( nrf_error );
}
