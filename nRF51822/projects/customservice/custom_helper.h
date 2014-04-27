/**************************************************************************/
/*!
    @file     custom_helper.h
*/
/**************************************************************************/
#ifndef _CUSTOM_HELPER_H_
#define _CUSTOM_HELPER_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "common/common.h"
#include "ble.h"

uint8_t custom_add_uuid_base(uint8_t const * const p_uuid_base);
error_t custom_decode_uuid(uint8_t const * const p_uuid_base, ble_uuid_t * p_uuid);

error_t custom_add_in_characteristic(uint16_t service_handle, ble_uuid_t* p_uuid, ble_gatt_char_props_t properties,
                                     uint8_t *p_data, uint16_t min_length, uint16_t max_length,
                                     ble_gatts_char_handles_t* p_char_handle);

#ifdef __cplusplus
}
#endif

#endif
