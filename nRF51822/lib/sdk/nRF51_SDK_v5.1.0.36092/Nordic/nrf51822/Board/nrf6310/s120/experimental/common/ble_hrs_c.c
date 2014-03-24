/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

/**@cond To Make Doxygen skip documentation generation for this file.
 * @{
 */

#include <stdint.h>
#include "ble_hrs_c.h"
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "nrf_error.h"
#include "ble_gattc.h"
#include "app_util.h"
#include "debug.h"

#define LOG                     debug_log        /**< Debug logger macro that will be used in this file to do logging of important information over UART. */
#define HRM_FLAG_MASK_HR_16BIT  (0x01 << 0)      /**< Bit mask used to extract the type of heart rate value. This is used to find if the received heart rate is a 16 bit value or an 8 bit value. */

static ble_hrs_c_t *            mp_ble_hrs_c;    /**< Pointer to the current instance of the HRS Client module. The memory for this provided by the application.*/


/**@brief       Function for handling Handle Value Notification received from the SoftDevice.
 *
 * @details     This function will handle the Handle Value Notification received from the SoftDevice
 *              and checks if it is a notification of the heart rate measurement from the peer. If
 *              so, this function will decode the heart rate measurement and send it to the
 *              application.
 *
 * @param[in]   p_ble_hrs_c Pointer to the Heart Rate Client structure.
 * @param[in]   p_ble_evt   Pointer to the BLE event received.
 */
static void on_hvx(ble_hrs_c_t * p_ble_hrs_c, const ble_evt_t * p_ble_evt)
{
    // Check if this is a heart rate notification.
    if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_hrs_c->hrm_handle)
    {
        ble_hrs_c_evt_t ble_hrs_c_evt;
        uint8_t         index = 0;

        ble_hrs_c_evt.evt_type = BLE_HRS_C_EVT_HRM_NOTIFICATION;

        if (!(p_ble_evt->evt.gattc_evt.params.hvx.data[index++] & HRM_FLAG_MASK_HR_16BIT))
        {
            // 8 Bit heart rate value received.
            ble_hrs_c_evt.params.hrm.hr_value = p_ble_evt->evt.gattc_evt.params.hvx.data[index++];  //lint !e415 suppress Lint Warning 415: Likely access out of bond */
        }
        else
        {
            // 16 bit heart rate value received.
            ble_hrs_c_evt.params.hrm.hr_value =
                            uint16_decode(&(p_ble_evt->evt.gattc_evt.params.hvx.data[index]));
        }

        p_ble_hrs_c->evt_handler(p_ble_hrs_c, &ble_hrs_c_evt);
    }
}


/**@brief       Function for handling events from the database discovery module.
 *
 * @details     This function will handle an event from the database discovery module, determine if
 *              it relates to the discovery of heart rate service at the peer. If so, it will
 *              call the application's event handler indicating that the heart rate service has been
 *              discovered at the peer. It also populate the event with the service related
 *              information before providing it to the application.
 *
 * @param[in]   p_evt Pointer to the event received from the database discovery module.
 *
 */
static void db_discover_evt_handler(ble_db_discovery_evt_t * p_evt)
{
    // Check if the Heart Rate Service was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE
        &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_HEART_RATE_SERVICE
        &&
        p_evt->params.discovered_db.srv_uuid.type == BLE_UUID_TYPE_BLE)
    {
        mp_ble_hrs_c->conn_handle = p_evt->conn_handle;

        // Find the CCCD Handle of the Heart Rate Measurement characteristic.
        uint8_t i;

        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid
                ==
                BLE_UUID_HEART_RATE_MEASUREMENT_CHAR)
            {
                // Found Heart Rate characteristic. Store CCCD handle and break.
                mp_ble_hrs_c->hrm_cccd_handle =
                          p_evt->params.discovered_db.charateristics[i].cccd_handle;
                mp_ble_hrs_c->hrm_handle =
                          p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                break;
            }
        }

        LOG("[HRP]: Heart Rate Service discovered at peer.\r\n");

        ble_hrs_c_evt_t evt;

        evt.evt_type = BLE_HRS_C_EVT_DISCOVERY_COMPLETE;

        mp_ble_hrs_c->evt_handler(mp_ble_hrs_c, &evt);
    }
}


uint32_t ble_hrs_c_init(ble_hrs_c_t * p_ble_hrs_c, ble_hrs_c_init_t * p_ble_hrs_c_init)
{
    if ((p_ble_hrs_c == NULL) || (p_ble_hrs_c_init == NULL))
    {
        return NRF_ERROR_NULL;
    }

    ble_uuid_t hrs_uuid;

    hrs_uuid.type                 = BLE_UUID_TYPE_BLE;
    hrs_uuid.uuid                 = BLE_UUID_HEART_RATE_SERVICE;

    mp_ble_hrs_c                  = p_ble_hrs_c;

    mp_ble_hrs_c->evt_handler     = p_ble_hrs_c_init->evt_handler;
    mp_ble_hrs_c->conn_handle     = BLE_CONN_HANDLE_INVALID;
    mp_ble_hrs_c->hrm_cccd_handle = BLE_GATT_HANDLE_INVALID;

    return ble_db_discovery_register(&hrs_uuid,
                                     db_discover_evt_handler);
}


void ble_hrs_c_on_ble_evt(ble_hrs_c_t * p_ble_hrs_c, const ble_evt_t * p_ble_evt)
{
    if ((p_ble_hrs_c == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            p_ble_hrs_c->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GATTC_EVT_HVX:
            on_hvx(p_ble_hrs_c, p_ble_evt);
            break;

        default:
            break;
    }
}


uint32_t ble_hrs_c_hrm_notif_enable(ble_hrs_c_t * p_ble_hrs_c)
{
    if (p_ble_hrs_c == NULL)
    {
        return NRF_ERROR_NULL;
    }

    ble_gattc_write_params_t write_params;
    uint8_t                  enable_notif[] = {BLE_GATT_HVX_NOTIFICATION, 0x00};

    write_params.handle   = p_ble_hrs_c->hrm_cccd_handle;
    write_params.len      = sizeof(enable_notif);
    write_params.offset   = 0;
    write_params.p_value  = enable_notif;
    write_params.write_op = BLE_GATT_OP_WRITE_REQ;

    LOG("[HRP]: Configuring CCCD. CCCD Handle = %d, Connection Handle = %d\r\n",
        p_ble_hrs_c->hrm_cccd_handle,
        p_ble_hrs_c->conn_handle);

    return sd_ble_gattc_write(p_ble_hrs_c->conn_handle, &write_params);
}

/** @}
 *  @endcond
 */
