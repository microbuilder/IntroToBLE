/**************************************************************************/
/*!
    @file     btle_advertising.c
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

#include "ble_advdata.h"
#include "btle.h"

/* ---------------------------------------------------------------------- */
/* MACRO CONSTANT TYPEDEF                                                 */
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
/* INTERNAL OBJECT & FUNCTION DECLARATION                                 */
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
/* IMPLEMENTATION											                                    */
/* ---------------------------------------------------------------------- */

/**************************************************************************/
/*!
    @brief  Initialises and sets the advertising data in the SoftDevice

    @note   Advertising data's length is restricted to 31. Standard Service
            is preferred than custom service UUIDs.

    @returns
*/
/**************************************************************************/
error_t btle_advertising_init(btle_service_t const service_list[], uint16_t const service_count)
{
  enum {
    ADV_UUID_MAX = 20,
    ADV_FIELD_HEADER_LENGTH = 2 /* header size for each field in adv data */
  };

  /*------------- Flag, Appearance, Tx power field -------------*/
  uint8_t const flags         = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
  int8_t const tx_power_level = CFG_BLE_TX_POWER_LEVEL;

  int8_t byte_left = BLE_GAP_ADV_MAX_SIZE;

  byte_left -= (ADV_FIELD_HEADER_LENGTH + 1); // flags
  byte_left -= (ADV_FIELD_HEADER_LENGTH + 1); // tx power
  byte_left -= (CFG_GAP_APPEARANCE != BLE_APPEARANCE_UNKNOWN) ? (ADV_FIELD_HEADER_LENGTH + 2) : 0; // appearance

  /*------------- UUID list -------------*/
  ble_uuid_t adv_uuids[ADV_UUID_MAX];
  ASSERT( ADV_UUID_MAX >= service_count, ERROR_NO_MEM); // the total service count exceed 20, need to increase ADV_COUNT_MAX

  uint16_t uuid_count;

  bool encounter_16_uuid_already  = false;
  bool encounter_128_uuid_already = false;

  /* Standard Services are added first (higher priority), modify to your own need if required */
  for(uuid_count=0; uuid_count < service_count && byte_left > 0; uuid_count++)
  {
    adv_uuids[uuid_count].type = service_list[uuid_count].uuid_type;
    adv_uuids[uuid_count].uuid = service_list[uuid_count].uuid;

    /*------------- Standard 16-bit UUID -------------*/
    if ( BLE_UUID_TYPE_BLE == adv_uuids[uuid_count].type )
    {
      if( !encounter_16_uuid_already )
      {
        byte_left -= ADV_FIELD_HEADER_LENGTH;
        encounter_16_uuid_already = true;
      }
      byte_left -= sizeof(uint16_t); // size of 16-bit uuid
    }
    /*------------- Custom 128-bit UUID -------------*/
    else if ( adv_uuids[uuid_count].type > BLE_UUID_TYPE_BLE)
    {
      if( !encounter_128_uuid_already )
      {
        byte_left -= ADV_FIELD_HEADER_LENGTH;
        encounter_128_uuid_already = true;
      }
      byte_left -= sizeof(ble_uuid128_t); // size of 128-bit uuid
    }else
    {
      ASSERT(false, 0); // uuid type is not initialized
    }
  }

  if ( byte_left < 0) uuid_count--; // remove the last uuid from the adv list since it overflows

  /*------------- Advertising Data -------------*/
  ble_advdata_t advdata =
  {
      .flags.size              = 1                                            ,
      .flags.p_data            = (uint8_t*) &flags                            ,
      .include_appearance      = CFG_GAP_APPEARANCE != BLE_APPEARANCE_UNKNOWN ,
      .p_tx_power_level        = (int8_t*) &tx_power_level                    ,

      .uuids_complete.uuid_cnt = uuid_count                                   ,
      .uuids_complete.p_uuids  = adv_uuids
  };

  /* left data is used for name if possible */
  if ( byte_left > ADV_FIELD_HEADER_LENGTH)
  {
    byte_left -= ADV_FIELD_HEADER_LENGTH;

    advdata.name_type      = (byte_left < strlen(CFG_GAP_LOCAL_NAME)) ? BLE_ADVDATA_SHORT_NAME : BLE_ADVDATA_FULL_NAME;
    advdata.short_name_len = (advdata.name_type == BLE_ADVDATA_SHORT_NAME) ? byte_left : 0;

    byte_left -= min8_of(byte_left, strlen(CFG_GAP_LOCAL_NAME));
  }

  ASSERT_STATUS( ble_advdata_set(&advdata, NULL) );

  return ERROR_NONE;
}

/**************************************************************************/
/*!
    @brief      Starts the advertising process

    @returns
*/
/**************************************************************************/
error_t btle_advertising_start(void)
{
  ble_gap_adv_params_t adv_para =
  {
      .type        = BLE_GAP_ADV_TYPE_ADV_IND      ,
      .p_peer_addr = NULL                          , // Undirected advertisement
      .fp          = BLE_GAP_ADV_FP_ANY            ,
      .p_whitelist = NULL                          ,
      .interval    = (CFG_GAP_ADV_INTERVAL_MS*8)/5 , // advertising interval (in units of 0.625 ms)
      .timeout     = CFG_GAP_ADV_TIMEOUT_S
  };

  ASSERT_STATUS( sd_ble_gap_adv_start(&adv_para) );

	return ERROR_NONE;
}
