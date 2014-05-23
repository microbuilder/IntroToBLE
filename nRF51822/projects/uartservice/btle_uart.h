/**************************************************************************/
/*!
    @file     btle_uart.h
    @author   hathach (tinyusb.org)

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

#ifndef _BTLE_UART_H_
#define _BTLE_UART_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "common/common.h"
#include "ble.h"

/*=========================================================================
    UART SERVICE CONFIGURATION
    -----------------------------------------------------------------------
    BLE_UART_BRIDGE                   Set this to 1 to enabled the UART
                                      bridge, which will read characters
                                      from the HW UART port and push them
                                      out over the air, and send any
                                      incoming characters back out on UART
    BLE_UART_UUID_BASE                The base 128-bit UUID to use for this
                                      service. Set bytes 3+4 to 0x00.
    BLE_UART_MAX_LENGTH               The maximum payload length
    BLE_UART_UUID_PRIMARY_SERVICE     The UUID fragment for the primary
                                      service (normally 1)
    BLE_UART_UUID_IN                  The UUID fragment for the TXD char
    BLE_UART_UUID_OUT                 The UUID fragment for the RXD char
    -----------------------------------------------------------------------*/
    #define BLE_UART_BRIDGE                 (1)
    #define BLE_UART_UUID_BASE              "\x6E\x40\x00\x00\xB5\xA3\xF3\x93\xE0\xA9\xE5\x0E\x24\xDC\xCA\x9E"
    #define BLE_UART_MAX_LENGTH             (20)
    #define BLE_UART_UUID_PRIMARY_SERVICE   (1)
    #define BLE_UART_UUID_IN                (3)
    #define BLE_UART_UUID_OUT               (2)
    #define BLE_UART_SEND_INDICATION        (0)
/*=========================================================================*/

error_t uart_service_init              ( uint8_t uuid_base_type );
void    uart_service_handler           ( ble_evt_t * p_ble_evt );
error_t uart_service_send              ( uint8_t data[], uint16_t length );
void    uart_service_received_callback ( uint8_t * data, uint16_t length ) ATTR_WEAK;
void    uart_service_indicate_callback ( bool is_succeeded );
void    uart_service_bridge_task       ( void* p_context );

#ifdef __cplusplus
 }
#endif

#endif /* _BTLE_UART_H_ */
