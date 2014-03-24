/**************************************************************************/
/*!
    @file     heart_rate.h
*/
/**************************************************************************/
#ifndef _HEART_RATE_H_
#define _HEART_RATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ble.h"

error_t heart_rate_init    ( void );
void    heart_rate_handler ( ble_evt_t * p_ble_evt );

#ifdef __cplusplus
}
#endif

#endif /* _HEART_RATE_H_ */
