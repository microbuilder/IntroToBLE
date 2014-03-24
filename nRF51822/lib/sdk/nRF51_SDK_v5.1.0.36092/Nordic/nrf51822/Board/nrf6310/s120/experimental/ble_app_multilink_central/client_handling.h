/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

#ifndef CLIENT_RELATED_H__
#define CLIENT_RELATED_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "conn_mngr.h"

#define MAX_CLIENTS 8

void client_init(void);
uint8_t client_count(void);
uint32_t client_create(conn_mngr_handle_t * p_handle);
uint32_t client_destroy(conn_mngr_handle_t * p_handle);
void client_ble_evt_handler(ble_evt_t * p_ble_evt);

#endif // CLIENT_RELATED_H__
