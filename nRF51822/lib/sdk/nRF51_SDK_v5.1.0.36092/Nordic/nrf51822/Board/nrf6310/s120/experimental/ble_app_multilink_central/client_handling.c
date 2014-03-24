/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */
 
#include "client_handling.h"
#include <string.h>
#include <stdbool.h>
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "debug.h"

#define LED_PIN_NO_OFFSET                 8

#define MULTILINK_PERIPHERAL_BASE_UUID    {0xB3, 0x58, 0x55, 0x40, 0x50, 0x60, 0x11, 0xe3,\
                                           0x8f, 0x96, 0x08, 0x00, 0x00, 0x00, 0x9a, 0x66}
#define MULTILINK_PERIPHERAL_SERVICE_UUID 0x9001
#define MULTILINK_PERIPHERAL_CHAR_UUID    0x900A


typedef enum
{
    IDLE,
    STATE_SERVICE_DISC,
    STATE_CHAR_DISC,
    STATE_DESCR_DISC,
    STATE_NOTIF_ENABLE,
    STATE_RUNNING,
    STATE_ERROR
} client_state_t;


typedef struct
{
    ble_gattc_char_t         gattc_char;
    ble_gattc_handle_range_t descr_range;
    uint16_t                 cccd_handle;
} client_char_t;


typedef struct
{
    uint16_t            conn_handle;
    client_state_t      state;
    ble_gattc_service_t service;
    client_char_t       client_char;
    uint8_t             pin_no;
} client_t;


static client_t m_client[MAX_CLIENTS];
static uint8_t  m_client_count;
static uint8_t  m_base_uuid_type;


static client_t * client_find(uint16_t conn_handle)
{
    int i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (m_client[i].conn_handle == conn_handle)
        {
            return &m_client[i];
        }
    }

    return NULL;
}


/**@brief Funtion for service discovery.
 */
/** @snippet [Service Discovery] */
static void service_discover(client_t * p_client)
{
    uint32_t   err_code;
    uint16_t   start_handle;
    ble_uuid_t uuid;

    p_client->state = STATE_SERVICE_DISC;

    uuid.type    = m_base_uuid_type;
    uuid.uuid    = MULTILINK_PERIPHERAL_SERVICE_UUID;
    start_handle = 0x0001;  
    
    err_code = sd_ble_gattc_primary_services_discover(p_client->conn_handle, start_handle, &uuid);
    APP_ERROR_CHECK(err_code);
}
/** @snippet [Service Discovery] */


/**@brief Funtion for characteristics discovery.
 */
/** @snippet [Characteristics Discovery] */ 
static void char_discover(client_t * p_client)
{
    uint32_t err_code;

    p_client->state = STATE_CHAR_DISC;

    err_code = sd_ble_gattc_characteristics_discover(p_client->conn_handle, &p_client->service.handle_range);
    APP_ERROR_CHECK(err_code);
}
/** @snippet [Characteristics Discovery] */ 


/**@brief Funtion for descriptor discovery.
 */
static void descr_discover(client_t * p_client)
{
    uint32_t err_code;

    p_client->state = STATE_DESCR_DISC;

    err_code = sd_ble_gattc_descriptors_discover(p_client->conn_handle, &p_client->client_char.descr_range);
    APP_ERROR_CHECK(err_code);
}


/**@brief Funtion for handling enabling notifications.
 */
static void notif_enable(client_t * p_client)
{
    uint32_t                 err_code;
    ble_gattc_write_params_t write_params;
    uint8_t                  buf[2];

    p_client->state = STATE_NOTIF_ENABLE;

    buf[0] = BLE_GATT_HVX_NOTIFICATION;
    buf[1] = 0;

    write_params.write_op = BLE_GATT_OP_WRITE_REQ;
    write_params.handle   = p_client->client_char.cccd_handle;
    write_params.offset   = 0;
    write_params.len      = sizeof(buf);
    write_params.p_value  = buf;

    err_code = sd_ble_gattc_write(p_client->conn_handle, &write_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Funtion for handling service discovery events.
 */
static void on_evt_prim_srvc_disc_rsp(ble_evt_t * p_ble_evt)
{
    client_t * p_client;
    ble_uuid_t uuid;

    p_client = client_find(p_ble_evt->evt.gattc_evt.conn_handle);
    if ((p_client != NULL) && (p_client->state == STATE_SERVICE_DISC))
    {
        if (p_ble_evt->evt.gattc_evt.gatt_status != BLE_GATT_STATUS_SUCCESS)
        {
            // Did not find the appropriate service
            p_client->state = STATE_ERROR;
        }
        else
        {
            int i;

            uuid.type = m_base_uuid_type;
            uuid.uuid = MULTILINK_PERIPHERAL_SERVICE_UUID;

            for (i = 0; i < p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.count; i++)
            {
                if (BLE_UUID_EQ(&uuid, &p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[i].uuid))
                {
                    p_client->service = p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[i];
                    char_discover(p_client);
                    break;
                }
            }

            if (p_client->state == STATE_SERVICE_DISC)
            {
                // Did not find the appropriate service
                p_client->state = STATE_ERROR;
            }
        }
    }
}


/**@brief Funtion for handling characterisctics discovery events.
 */
static void on_evt_char_disc_rsp(ble_evt_t * p_ble_evt)
{
    client_t * p_client;
    ble_uuid_t uuid;

    p_client = client_find(p_ble_evt->evt.gattc_evt.conn_handle);
    if ((p_client != NULL) && (p_client->state == STATE_CHAR_DISC))
    {
        if (p_ble_evt->evt.gattc_evt.gatt_status != BLE_GATT_STATUS_SUCCESS)
        {
            /* Did not find the appropriate characteristic */
            p_client->state = STATE_ERROR;
        }
        else
        {
            int i;

            uuid.type = m_base_uuid_type;
            uuid.uuid = MULTILINK_PERIPHERAL_CHAR_UUID;

            // Iterate trough the characteristics and find the correct one
            for (i = 0; i < p_ble_evt->evt.gattc_evt.params.char_disc_rsp.count; i++)
            {
                if (BLE_UUID_EQ(&uuid, &p_ble_evt->evt.gattc_evt.params.char_disc_rsp.chars[i].uuid))
                {
                    memcpy(&p_client->client_char.gattc_char, &p_ble_evt->evt.gattc_evt.params.char_disc_rsp.chars[i], sizeof(ble_gattc_char_t));
                    p_client->client_char.descr_range.start_handle = p_client->client_char.gattc_char.handle_decl + 2;
                    if (i < p_ble_evt->evt.gattc_evt.params.char_disc_rsp.count - 1)
                    {
                        p_client->client_char.descr_range.end_handle = p_ble_evt->evt.gattc_evt.params.char_disc_rsp.chars[i + 1].handle_decl - 1; //lint !e661 To suppress lint warning Possible access of out-of-bounds pointer. 
                    }
                    else
                    {
                        p_client->client_char.descr_range.end_handle = 0xFFFF;
                    }

                    descr_discover(p_client);
                    break;
                }
            }

            if (p_client->state == STATE_CHAR_DISC)
            {
                // Did not find the appropriate characteristic
                p_client->state = STATE_ERROR;
            }
        }
    }
}


/**@brief Funtion for handling descriptor discover event.
 */
static void on_evt_desc_disc_rsp(ble_evt_t * p_ble_evt)
{
    client_t * p_client;
    ble_uuid_t uuid;

    p_client = client_find(p_ble_evt->evt.gattc_evt.conn_handle);
    if ((p_client != NULL) && (p_client->state == STATE_DESCR_DISC))
    {
        if (p_ble_evt->evt.gattc_evt.gatt_status != BLE_GATT_STATUS_SUCCESS)
        {
            // Did not find the appropriate descriptor
            p_client->state = STATE_ERROR;
        }
        else
        {
            int i;

            uuid.type = BLE_UUID_TYPE_BLE;
            uuid.uuid = BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG;

            // Iterate trough the descriptors and find the correct one
            for (i = 0; i < p_ble_evt->evt.gattc_evt.params.desc_disc_rsp.count; i++)
            {
                if (BLE_UUID_EQ(&uuid, &p_ble_evt->evt.gattc_evt.params.desc_disc_rsp.descs[i].uuid))
                {
                    p_client->client_char.cccd_handle = 
                              p_ble_evt->evt.gattc_evt.params.desc_disc_rsp.descs[i].handle;
                    notif_enable(p_client);
                    break;
                }
            }

            if (p_client->state == STATE_DESCR_DISC)
            {
                // Did not find the appropriate characteristic
                p_client->state = STATE_ERROR;
            }
        }
    }
}


/**@brief Funtion for setting client in a running state once write response is received.
 */
static void on_evt_write_rsp(ble_evt_t * p_ble_evt)
{
    client_t * p_client;

    p_client = client_find(p_ble_evt->evt.gattc_evt.conn_handle);
    if ((p_client != NULL) && (p_client->state == STATE_NOTIF_ENABLE))
    {
        if (p_ble_evt->evt.gattc_evt.params.write_rsp.handle != p_client->client_char.cccd_handle)
        {
            // Got response from unexpected handle
            p_client->state = STATE_ERROR;
        }
        else
        {
            p_client->state = STATE_RUNNING;
        }
    }
}


/**@brief Funtion for toggeling LEDS based on received notifications.
 */
static void on_evt_hvx(ble_evt_t * p_ble_evt)
{
    client_t * p_client;

    p_client = client_find(p_ble_evt->evt.gattc_evt.conn_handle);
    if ((p_client != NULL) && (p_client->state == STATE_RUNNING))
    {
        if ((p_ble_evt->evt.gattc_evt.params.hvx.handle == 
             p_client->client_char.gattc_char.handle_value) 
             && (p_ble_evt->evt.gattc_evt.params.hvx.len == 1))
        {
            if (p_ble_evt->evt.gattc_evt.params.hvx.data[0] == 0)
            {
                nrf_gpio_pin_clear(p_client->pin_no + LED_PIN_NO_OFFSET);
            }
            else
            {
                nrf_gpio_pin_set(p_client->pin_no + LED_PIN_NO_OFFSET);
            }
        }
    }
}


/**@brief Funtion for handling time out events.
 */
static void on_evt_timeout(ble_evt_t * p_ble_evt)
{
    client_t * p_client;

    APP_ERROR_CHECK_BOOL(p_ble_evt->evt.gattc_evt.params.timeout.src 
                         == BLE_GATT_TIMEOUT_SRC_PROTOCOL);

    p_client = client_find(p_ble_evt->evt.gattc_evt.conn_handle);
    if (p_client != NULL)
    {
        p_client->state = STATE_ERROR;
    }
}


/**@brief Funtion for handling client events.
 */
void client_ble_evt_handler(ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
            on_evt_prim_srvc_disc_rsp(p_ble_evt);
            break;

        case BLE_GATTC_EVT_CHAR_DISC_RSP:
            on_evt_char_disc_rsp(p_ble_evt);
            break;

        case BLE_GATTC_EVT_DESC_DISC_RSP:
            on_evt_desc_disc_rsp(p_ble_evt);
            break;

        case BLE_GATTC_EVT_WRITE_RSP:
            on_evt_write_rsp(p_ble_evt);
            break;

        case BLE_GATTC_EVT_HVX:
            on_evt_hvx(p_ble_evt);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            on_evt_timeout(p_ble_evt);
            break;

        default:
            break;
    }
}


/**@brief Funtion for initializing the client handling.
 */
void client_init(void)
{
    uint32_t err_code;
    int i;

    ble_uuid128_t base_uuid = MULTILINK_PERIPHERAL_BASE_UUID;
    
    err_code = sd_ble_uuid_vs_add(&base_uuid, &m_base_uuid_type);
    APP_ERROR_CHECK(err_code);
    
    nrf_gpio_range_cfg_output(8, 15);

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        m_client[i].state  = IDLE;
        m_client[i].pin_no = i;
    }
    m_client_count = 0;
}


/**@brief Funtion for returning the current number of clients.
 */
uint8_t client_count(void)
{
    return m_client_count;
}


/**@brief Funtion for creating a new client.
 */
uint32_t client_create(conn_mngr_handle_t * p_handle)
{
    uint32_t retval;
    int i;

    retval = NRF_ERROR_NO_MEM;
    
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (m_client[i].state == IDLE)
        {   
            retval = conn_mngr_app_context_set(p_handle,(void *)&m_client[i]);
            if (retval == NRF_SUCCESS)
            {
                m_client[i].state       = STATE_SERVICE_DISC;
                m_client[i].conn_handle = p_handle->conn_handle;
                m_client_count++;
                service_discover(&m_client[i]);
                retval = NRF_SUCCESS;
            }
            break;
        }
    }

    return retval;
}


/**@brief Funtion for freeing up a client by setting its state to idle.
 */
uint32_t client_destroy(conn_mngr_handle_t * p_handle)
{
    uint32_t retval;
    client_t * p_client;
    
    retval = conn_mngr_app_context_get(p_handle,(const void**)&p_client);
    if (retval ==  NRF_SUCCESS)
    {
        if (p_client->state != IDLE)
        {
            m_client_count--;
            p_client->state = IDLE;
        }
        else
        {
            retval = NRF_ERROR_INVALID_STATE;
        }
    }
    return retval;
}

