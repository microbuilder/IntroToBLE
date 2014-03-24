/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 */
 
/*Disclaimer: This module (Apple Notification Center Service) can and will be changed at any time by either Apple or Nordic Semiconductor ASA.*/ 
#include "ble_ancs_c.h"
#include <string.h>
#include <stdbool.h>
#include "ble_err.h"
#include "ble_srv_common.h"
#include "nordic_common.h"
#include "nrf_assert.h"
#include "ble_bondmngr.h"
#include "ble_bondmngr_cfg.h"
#include "ble_flash.h"

#include "nRF6350.h"
#include "nrf_gpio.h"

#define START_HANDLE_DISCOVER            0x0001                                            /**< Value of start handle during discovery. */

#define NOTIFICATION_DATA_LENGTH         2                                                 /**< The mandatory length of notification data. After the mandatory data, the optional message is located. */

#define TX_BUFFER_MASK                   0x07                                              /**< TX Buffer mask, must be a mask of contiguous zeroes, followed by contiguous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE                   (TX_BUFFER_MASK + 1)                              /**< Size of send buffer, which is 1 higher than the mask. */
#define WRITE_MESSAGE_LENGTH             20                                                /**< Length of the write message for CCCD/control point. */
#define BLE_CCCD_NOTIFY_BIT_MASK         0x0001                                            /**< Enable Notification bit. */

#define BLE_ANCS_MAX_DISCOVERED_CENTRALS BLE_BONDMNGR_MAX_BONDED_CENTRALS                  /**< Maximum number of discovered services that can be stored in the flash. This number should be identical to maximum number of bonded masters. */

#define DISCOVERED_SERVICE_DB_SIZE \
    CEIL_DIV(sizeof(apple_service_t) * BLE_ANCS_MAX_DISCOVERED_CENTRALS, sizeof(uint32_t))  /**< Size of bonded masters database in word size (4 byte). */

typedef enum
{
    READ_REQ = 1,                                                                          /**< Type identifying that this tx_message is a read request. */
    WRITE_REQ                                                                              /**< Type identifying that this tx_message is a write request. */
} ancs_tx_request_t;

typedef enum
{
    STATE_UNINITIALIZED,                                                                   /**< Uninitialized state of the internal state machine. */
    STATE_IDLE,                                                                            /**< Idle state, this is the state when no master has connected to this device. */
    STATE_DISC_SERV,                                                                       /**< A BLE master is connected and a service discovery is in progress. */
    STATE_DISC_CHAR,                                                                       /**< A BLE master is connected and characteristic discovery is in progress. */
    STATE_DISC_DESC,                                                                       /**< A BLE master is connected and descriptor discovery is in progress. */
    STATE_RUNNING,                                                                         /**< A BLE master is connected and complete service discovery has been performed. */
    STATE_WAITING_ENC,                                                                     /**< A previously bonded BLE master has re-connected and the service awaits the setup of an encrypted link. */
    STATE_RUNNING_NOT_DISCOVERED,                                                          /**< A BLE master is connected and a service discovery is in progress. */
} ancs_state_t;


/* brief Structure used for holding the characteristic found during discovery process.
 */
typedef struct
{
    ble_uuid_t               uuid;                                                         /**< UUID identifying this characteristic. */
    ble_gatt_char_props_t    properties;                                                   /**< Properties for this characteristic. */
    uint16_t                 handle_decl;                                                  /**< Characteristic Declaration Handle for this characteristic. */
    uint16_t                 handle_value;                                                 /**< Value Handle for the value provided in this characteristic. */
    uint16_t                 handle_cccd;                                                  /**< CCCD Handle value for this characteristic. BLE_ANCS_INVALID_HANDLE if not present in the master. */
} apple_characteristic_t;

/**@brief Structure used for holding the Alert Notification Service found during discovery process.
 */
typedef struct
{
    uint8_t                  handle;                                                       /**< Handle of Alert Notification Service which identifies to which master this discovered service belongs. */
    ble_gattc_service_t      service;                                                      /**< The GATT service holding the discovered Alert Notification Service. */
    apple_characteristic_t   control_point; 
    apple_characteristic_t   notification_source;
    apple_characteristic_t   data_source;
} apple_service_t;

/**@brief Structure for writing a message to the master, i.e. Control Point or CCCD.
 */
typedef struct
{
    uint8_t                  gattc_value[WRITE_MESSAGE_LENGTH];                            /**< The message to write. */
    ble_gattc_write_params_t gattc_params;                                                 /**< GATTC parameters for this message. */
} write_params_t;

/**@brief Structure for holding data to be transmitted to the connected master.
 */
typedef struct
{
    uint16_t                 conn_handle;                                                  /**< Connection handle to be used when transmitting this message. */
    ancs_tx_request_t        type;                                                         /**< Type of this message, i.e. read or write message. */
    union
    {
        uint16_t             read_handle;                                                  /**< Read request message. */
        write_params_t       write_req;                                                    /**< Write request message. */
    } req;
} tx_message_t;

/**@brief ANCS notification attribute parsing states.
 */
typedef enum
{
    COMMAND_ID,
    NOTIFICATION_UID1,
    NOTIFICATION_UID2,
    NOTIFICATION_UID3,
    NOTIFICATION_UID4,
    ATTRIBUTE_ID,
    ATTRIBUTE_LEN1,
    ATTRIBUTE_LEN2,
    ATTRIBUTE_READY
} parse_state_t;

static tx_message_t          m_tx_buffer[TX_BUFFER_SIZE];                                  /**< Transmit buffer for messages to be transmitted to the master. */
static uint32_t              m_tx_insert_index = 0;                                        /**< Current index in the transmit buffer where next message should be inserted. */
static uint32_t              m_tx_index = 0;                                               /**< Current index in the transmit buffer from where the next message to be transmitted resides. */

static ancs_state_t          m_client_state = STATE_UNINITIALIZED;                         /**< Current state of the Alert Notification State Machine. */

static uint32_t              m_service_db[DISCOVERED_SERVICE_DB_SIZE];                     /**< Service database for bonded masters (Word size aligned). */
static apple_service_t *     mp_service_db;                                                /**< Pointer to start of discovered services database. */
static apple_service_t       m_service;                                                    /**< Current service data. */

static parse_state_t         m_parse_state = COMMAND_ID;                                   /**< ANCS notification attribute parsing state. */


const ble_uuid128_t ble_ancs_base_uuid128 =
{
   {
    // 7905F431-B5CE-4E99-A40F-4B1E122D00D0
    0xd0, 0x00, 0x2d, 0x12, 0x1e, 0x4b, 0x0f, 0xa4,
    0x99, 0x4e, 0xce, 0xb5, 0x31, 0xf4, 0x05, 0x79

   }
};

const ble_uuid128_t ble_ancs_cp_base_uuid128 =
{
   {
    // 69d1d8f3-45e1-49a8-9821-9bbdfdaad9d9
    0xd9, 0xd9, 0xaa, 0xfd, 0xbd, 0x9b, 0x21, 0x98,
    0xa8, 0x49, 0xe1, 0x45, 0xf3, 0xd8, 0xd1, 0x69

   }
};

const ble_uuid128_t ble_ancs_ns_base_uuid128 =
{
   {
    // 9FBF120D-6301-42D9-8C58-25E699A21DBD
    0xbd, 0x1d, 0xa2, 0x99, 0xe6, 0x25, 0x58, 0x8c,
    0xd9, 0x42, 0x01, 0x63, 0x0d, 0x12, 0xbf, 0x9f

   }
};

const ble_uuid128_t ble_ancs_ds_base_uuid128 =
{
   {
    // 22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB
    0xfb, 0x7b, 0x7c, 0xce, 0x6a, 0xb3, 0x44, 0xbe,
    0xb5, 0x4b, 0xd6, 0x24, 0xe9, 0xc6, 0xea, 0x22

   }
};

/**@brief Function for passing any pending request from the buffer to the stack.
 */
static void tx_buffer_process(void)
{
    if (m_tx_index != m_tx_insert_index)
    {
        uint32_t err_code;

        if (m_tx_buffer[m_tx_index].type == READ_REQ)
        {
            err_code = sd_ble_gattc_read(m_tx_buffer[m_tx_index].conn_handle,
                                         m_tx_buffer[m_tx_index].req.read_handle,
                                         0);
        }
        else
        {
            err_code = sd_ble_gattc_write(m_tx_buffer[m_tx_index].conn_handle,
                                          &m_tx_buffer[m_tx_index].req.write_req.gattc_params);
        }
        if (err_code == NRF_SUCCESS)
        {
            ++m_tx_index;
            m_tx_index &= TX_BUFFER_MASK;
        }
    }
}


/**@brief Function for updating the current state and sending an event on discovery failure.
 */
static void handle_discovery_failure(const ble_ancs_c_t * p_ancs, uint32_t code)
{
    ble_ancs_c_evt_t event;

    m_client_state        = STATE_RUNNING_NOT_DISCOVERED;
    event.evt_type        = BLE_ANCS_C_EVT_DISCOVER_FAILED;
    event.data.error_code = code;

    p_ancs->evt_handler(&event);
}


/**@brief Function for executing the Service Discovery Procedure.
 */
static void service_disc_req_send(const ble_ancs_c_t * p_ancs)
{
    uint16_t   handle = START_HANDLE_DISCOVER;
    ble_uuid_t ancs_uuid;
    uint32_t   err_code;

    // Discover services on uuid for ANCS.
    BLE_UUID_BLE_ASSIGN(ancs_uuid, BLE_UUID_APPLE_NOTIFICATION_CENTER_SERVICE);
    ancs_uuid.type = BLE_UUID_TYPE_VENDOR_BEGIN;

    err_code = sd_ble_gattc_primary_services_discover(p_ancs->conn_handle, handle, &ancs_uuid);
    if (err_code != NRF_SUCCESS)
    {
        handle_discovery_failure(p_ancs, err_code);
    }
    else
    {
        m_client_state = STATE_DISC_SERV;
    }
}


/**@brief Function for executing the Characteristic Discovery Procedure.
 */
static void characteristic_disc_req_send(const ble_ancs_c_t * p_ancs,
                                         const ble_gattc_handle_range_t * p_handle)
{
    uint32_t err_code;

    // With valid service, we should discover characteristics.
    err_code = sd_ble_gattc_characteristics_discover(p_ancs->conn_handle, p_handle);

    if (err_code == NRF_SUCCESS)
    {
        m_client_state = STATE_DISC_CHAR;
    }
    else
    {
        handle_discovery_failure(p_ancs, err_code);
    }
}


/**@brief Function for executing the Characteristic Descriptor Discovery Procedure.
 */
static void descriptor_disc_req_send(const ble_ancs_c_t * p_ancs)
{
    ble_gattc_handle_range_t descriptor_handle;
    uint32_t                 err_code = NRF_SUCCESS;

    // If we have not discovered descriptors for all characteristics,
    // we will discover next descriptor.
    if (m_service.notification_source.handle_cccd == BLE_ANCS_INVALID_HANDLE)
    {
        descriptor_handle.start_handle = m_service.notification_source.handle_value + 1;
        descriptor_handle.end_handle = m_service.notification_source.handle_value + 1;

        err_code = sd_ble_gattc_descriptors_discover(p_ancs->conn_handle, &descriptor_handle);
    }
    else if (m_service.data_source.handle_cccd == BLE_ANCS_INVALID_HANDLE)
    {
        descriptor_handle.start_handle = m_service.data_source.handle_value + 1;
        descriptor_handle.end_handle = m_service.data_source.handle_value + 1;

        err_code = sd_ble_gattc_descriptors_discover(p_ancs->conn_handle, &descriptor_handle);
    }


    if (err_code == NRF_SUCCESS)
    {
        m_client_state = STATE_DISC_DESC;
    }
    else
    {
        handle_discovery_failure(p_ancs, err_code);
    }
}


/**@brief Function for indicating that a connection has successfully been established. 
 *        Either when the Service Discovery Procedure completes or a re-connection has been 
 *        established to a bonded master.
 *
 * @details This function is executed when a service discovery or a re-connect to a bonded master
 *          occurs. In the event of re-connection to a bonded master, this function will ensure
 *          writing of the control point according to the Alert Notification Service Client
 *          specification.
 *          Finally an event is passed to the application:
 *          BLE_ANCS_C_EVT_RECONNECT         - When we are connected to an existing master and the
 *                                            apple notification control point has been written.
 *          BLE_ANCS_C_EVT_DISCOVER_COMPLETE - When we are connected to a new master and the Service
 *                                            Discovery has been completed.
 */
static void connection_established(const ble_ancs_c_t * p_ancs)
{
    ble_ancs_c_evt_t event;
  
    m_client_state = STATE_RUNNING;

    event.evt_type = BLE_ANCS_C_EVT_DISCOVER_COMPLETE;
    p_ancs->evt_handler(&event);
}


/**@brief Function for waiting until an encrypted link has been established to a bonded master.
 */
static void encrypted_link_setup_wait(const ble_ancs_c_t * p_ancs)
{
    m_client_state = STATE_WAITING_ENC;
}


/**@brief Function for handling the connect event when a master connects.
 *
 * @details This function will check if bonded master connects, and do the following
 *          Bonded master  - enter wait for encryption state.
 *          Unknown master - Initiate service discovery procedure.
 */
static void event_connect(ble_ancs_c_t * p_ancs, const ble_evt_t * p_ble_evt)
{
    p_ancs->conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;

    if (p_ancs->master_handle != INVALID_CENTRAL_HANDLE)
    {
        m_service = mp_service_db[p_ancs->master_handle];
        encrypted_link_setup_wait(p_ancs);
    }
    else
    {
        m_service.handle = INVALID_SERVICE_HANDLE;
        service_disc_req_send(p_ancs);
    }
}


/**@brief Function for handling the encrypted link event when a secure
 *        connection has been established with a master.
 *
 * @details This function will check if the service for the master is known.
 *          Service known   - Execute action running / switch to running state.
 *          Service unknown - Initiate Service Discovery Procedure.
 */
static void event_encrypted_link(ble_ancs_c_t * p_ancs, const ble_evt_t * p_ble_evt)
{
    // If we are setting up a bonded connection and the UUID of the service
    // is unknown, a new discovery must be performed.
    if (m_service.service.uuid.uuid != BLE_UUID_APPLE_NOTIFICATION_CENTER_SERVICE)
    {
        m_service.handle = INVALID_SERVICE_HANDLE;
        service_disc_req_send(p_ancs);
    }
    else
    {
        connection_established(p_ancs);
    }
}


/**@brief Function for handling the response on service discovery.
 *
 * @details This function will validate the response.
 *          Invalid response - Handle discovery failure.
 *          Valid response   - Initiate Characteristic Discovery Procedure.
 */
static void event_discover_rsp(ble_ancs_c_t * p_ancs, const ble_evt_t * p_ble_evt)
{
    if (p_ble_evt->evt.gattc_evt.gatt_status != BLE_GATT_STATUS_SUCCESS)
    {
        // We have received an  unexpected result.
        // As we are in a connected state, but can not continue
        // our service discovery, we will go to running state.
        handle_discovery_failure(p_ancs, p_ble_evt->evt.gattc_evt.gatt_status);
    }
    else
    {
        BLE_UUID_COPY_INST(m_service.service.uuid,
                           p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[0].uuid);

        if (p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.count > 0)
        {
            const ble_gattc_service_t * p_service;

            p_service = &(p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[0]);

            m_service.service.handle_range.start_handle = p_service->handle_range.start_handle;
            m_service.service.handle_range.end_handle   = p_service->handle_range.end_handle;

            characteristic_disc_req_send(p_ancs, &(m_service.service.handle_range));
        }
        else
        {
            // If we receive an answer, but it contains no service, we just go to state: Running.
            handle_discovery_failure(p_ancs, p_ble_evt->evt.gattc_evt.gatt_status);
        }
    }
}


/**@brief Function for setting the discovered characteristics in the apple service.
 */
static void characteristics_set(apple_characteristic_t * p_characteristic,
                                const ble_gattc_char_t * p_char_resp)
{
    BLE_UUID_COPY_INST(p_characteristic->uuid, p_char_resp->uuid);
    
    p_characteristic->properties   = p_char_resp->char_props;
    p_characteristic->handle_decl  = p_char_resp->handle_decl;
    p_characteristic->handle_value = p_char_resp->handle_value;
    p_characteristic->handle_cccd  = BLE_ANCS_INVALID_HANDLE;
}


/**@brief Function for handling a characteristic discovery response event.
 *
 * @details This function will validate and store the characteristics received.
 *          If not all characteristics are discovered it will continue the characteristic discovery
 *          procedure, otherwise it will continue with the descriptor discovery procedure.
 *          If we receive a GATT Client error, we will go to handling of discovery failure.
 */
static void event_characteristic_rsp(ble_ancs_c_t * p_ancs, const ble_evt_t * p_ble_evt)
{
    if (p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND ||
        p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_INVALID_HANDLE)
    {
        if ((m_service.notification_source.handle_value == 0)    ||                                 
            (m_service.control_point.handle_value == 0)    ||
            (m_service.data_source.handle_value == 0) )
        {
            // At least one required characteristic is missing on the server side.
            handle_discovery_failure(p_ancs, NRF_ERROR_NOT_FOUND);
        }
        else 
        {
            descriptor_disc_req_send(p_ancs);
        }
    }
    else if (p_ble_evt->evt.gattc_evt.gatt_status)
    {
        // We have received an  unexpected result.
        // As we are in a connected state, but can not continue
        // our service discovery, we must go to running state.
        handle_discovery_failure(p_ancs, p_ble_evt->evt.gattc_evt.gatt_status);
    }
    else
    {
        uint32_t                 i;
        const ble_gattc_char_t * p_char_resp = NULL;

        // Iterate trough the characteristics and find the correct one.
        for (i = 0; i < p_ble_evt->evt.gattc_evt.params.char_disc_rsp.count; i++)
        {
            p_char_resp = &(p_ble_evt->evt.gattc_evt.params.char_disc_rsp.chars[i]);
            switch (p_char_resp->uuid.uuid)
            {
                case BLE_UUID_ANCS_CONTROL_POINT_CHAR:
                    characteristics_set(&m_service.control_point, p_char_resp);
                    break;

                case BLE_UUID_ANCS_NOTIFICATION_SOURCE_CHAR:
                    characteristics_set(&m_service.notification_source, p_char_resp);
                    break;

                case BLE_UUID_ANCS_DATA_SOURCE_CHAR:
                    characteristics_set(&m_service.data_source, p_char_resp);
                    break;

                default:
                    break;
            }
        }


        if (p_char_resp != NULL)
        {
            // If not all characteristics are received, we do a 2nd/3rd/... round.
            ble_gattc_handle_range_t char_handle;

            char_handle.start_handle = p_char_resp->handle_value + 1;
            char_handle.end_handle   = m_service.service.handle_range.end_handle;

            characteristic_disc_req_send(p_ancs, &char_handle);
        }
        else
        {
            characteristic_disc_req_send(p_ancs, &(m_service.service.handle_range));
        }
    }
}


/**@brief Function for setting the discovered descriptor in the apple service.
 */
static void descriptor_set(apple_service_t * p_service, const ble_gattc_desc_t * p_desc_resp)
{
    if (p_service->control_point.handle_value == (p_desc_resp->handle - 1))
    {
        if (p_desc_resp->uuid.uuid == BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG)
        {
            p_service->control_point.handle_cccd = p_desc_resp->handle;
        }
    }

    else if (p_service->notification_source.handle_value == (p_desc_resp->handle - 1))
    {
        if (p_desc_resp->uuid.uuid == BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG)
        {
            p_service->notification_source.handle_cccd = p_desc_resp->handle;
        }
    }
    else if (p_service->data_source.handle_value == (p_desc_resp->handle - 1))
    {
        if (p_desc_resp->uuid.uuid == BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG)
        {
            p_service->data_source.handle_cccd = p_desc_resp->handle;
        }
    }
}

/**@brief Function for handling of descriptor discovery responses.
 *
 * @details This function will validate and store the descriptor received.
 *          If not all descriptors are discovered it will continue the descriptor discovery
 *          procedure, otherwise it will continue to running state.
 *          If we receive a GATT Client error, we will go to handling of discovery failure.
 */
static void event_descriptor_rsp(ble_ancs_c_t * p_ancs, const ble_evt_t * p_ble_evt)
{
    if (p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND ||
        p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_INVALID_HANDLE)
    {
        handle_discovery_failure(p_ancs, NRF_ERROR_NOT_FOUND);
    }
    else if (p_ble_evt->evt.gattc_evt.gatt_status)
    {
        // We have received an unexpected result.
        // As we are in a connected state, but can not continue
        // our descriptor discovery, we will go to running state.
        handle_discovery_failure(p_ancs, p_ble_evt->evt.gattc_evt.gatt_status);
    }
    else
    {
        if (p_ble_evt->evt.gattc_evt.params.desc_disc_rsp.count > 0)
        {
            descriptor_set(&m_service, &(p_ble_evt->evt.gattc_evt.params.desc_disc_rsp.descs[0]));
        }

        if (m_service.notification_source.handle_cccd == BLE_ANCS_INVALID_HANDLE ||
            m_service.data_source.handle_cccd == BLE_ANCS_INVALID_HANDLE)
        {
            descriptor_disc_req_send(p_ancs);
        }
        else
        {
            connection_established(p_ancs);
        }
    }
}

/**@brief Function for parsing received notification attribute response data.
 */
static void parse_get_notification_attributes_response(ble_ancs_c_t * p_ancs, const uint8_t *data, int len)
{
    static uint8_t *ptr;
    static uint16_t current_len;
    static ble_ancs_c_evt_t event;
    int i;

    for (i = 0; i < len; i++)
    {
        switch (m_parse_state)
        {
        case COMMAND_ID:
            event.data.attribute.command_id = data[i];
            m_parse_state = NOTIFICATION_UID1;
            break;

        case NOTIFICATION_UID1:
            event.data.attribute.notification_uid[0] = data[i];
            m_parse_state = NOTIFICATION_UID2;
            break;

        case NOTIFICATION_UID2:
            event.data.attribute.notification_uid[1] = data[i];
            m_parse_state = NOTIFICATION_UID3;
            break;

        case NOTIFICATION_UID3:
            event.data.attribute.notification_uid[2] = data[i];
            m_parse_state = NOTIFICATION_UID4;
            break;

        case NOTIFICATION_UID4:
            event.data.attribute.notification_uid[3] = data[i];
            m_parse_state = ATTRIBUTE_ID;
            break;

        case ATTRIBUTE_ID:
            event.data.attribute.attribute_id = data[i];
            m_parse_state = ATTRIBUTE_LEN1;
            break;

        case ATTRIBUTE_LEN1:
            event.data.attribute.attribute_len = data[i];
            m_parse_state = ATTRIBUTE_LEN2;
            break;

        case ATTRIBUTE_LEN2:
            event.data.attribute.attribute_len |= (data[i] << 8);
            m_parse_state = ATTRIBUTE_READY;
            ptr = event.data.attribute.data;
            current_len = 0;
            break;

        case ATTRIBUTE_READY:
            *ptr++ = data[i];
            current_len++;

            if (current_len == event.data.attribute.attribute_len)
            {
                *ptr++ = 0;
                event.evt_type = BLE_ANCS_C_EVT_NOTIF_ATTRIBUTE;
                p_ancs->evt_handler(&event);
                m_parse_state = ATTRIBUTE_ID;
            }
            break;
        }
    }
}

/**@brief Function for receiving and validating notifications received from the master.
 */
static void event_notify(ble_ancs_c_t * p_ancs, const ble_evt_t * p_ble_evt)
{
    ble_ancs_c_evt_t               event;
    const ble_gattc_evt_hvx_t *    p_notification = &p_ble_evt->evt.gattc_evt.params.hvx;

    // Message is not valid -> ignore.
    if (p_notification->len < NOTIFICATION_DATA_LENGTH)
    {
        return;
    }

    if (p_notification->handle == m_service.notification_source.handle_value)
    {
        BLE_UUID_COPY_INST(event.uuid, m_service.notification_source.uuid);
    }
    else if (p_notification->handle == m_service.data_source.handle_value)
    {
        BLE_UUID_COPY_INST(event.uuid, m_service.data_source.uuid);
    }
    else
    {
        // Nothing to process.
        return;
    }

    if (p_notification->handle == m_service.notification_source.handle_value)
    {
        event.data.notification.event_id       = p_notification->data[0];                 /*lint -e{*} suppress Warning 415: possible access out of bond */
        event.data.notification.event_flags    = p_notification->data[1];                 /*lint -e{*} suppress Warning 415: possible access out of bond */
        event.data.notification.category_id    = p_notification->data[2];                 /*lint -e{*} suppress Warning 415: possible access out of bond */
        event.data.notification.category_count = p_notification->data[3];                 /*lint -e{*} suppress Warning 415: possible access out of bond */
        memcpy(event.data.notification.notification_uid, &p_notification->data[4], 4);    /*lint -e{*} suppress Warning 415: possible access out of bond */
        event.evt_type = BLE_ANCS_C_EVT_IOS_NOTIFICATION;

        p_ancs->evt_handler(&event);

        if (event.data.notification.event_id == BLE_ANCS_EVENT_ID_NOTIFICATION_ADDED)
        {
            ble_ancs_attr_list_t attr_list[2];
            attr_list[0].attribute_id = BLE_ANCS_NOTIFICATION_ATTRIBUTE_ID_TITLE;
            attr_list[0].attribute_len = 32;
            attr_list[1].attribute_id = BLE_ANCS_NOTIFICATION_ATTRIBUTE_ID_MESSAGE;
            attr_list[1].attribute_len = 32;
            uint32_t err_code = ble_ancs_get_notification_attributes(p_ancs, event.data.notification.notification_uid, 2, attr_list);
            APP_ERROR_CHECK(err_code);
            m_parse_state = COMMAND_ID;
        }
    }
    else if (p_notification->handle == m_service.data_source.handle_value)
    {
        parse_get_notification_attributes_response(p_ancs, p_notification->data, p_notification->len);    
    }
}

/**@brief Function for handling write response events.
 */
static void event_write_rsp(ble_ancs_c_t * p_ancs, const ble_evt_t * p_ble_evt)
{
    tx_buffer_process();
}

/**@brief Function for disconnecting and cleaning the current service.
 */
static void event_disconnect(ble_ancs_c_t * p_ancs)
{
    m_client_state = STATE_IDLE;

    if (m_service.handle == INVALID_SERVICE_HANDLE_DISC &&
        p_ancs->master_handle != INVALID_CENTRAL_HANDLE)
    {
        m_service.handle = p_ancs->master_handle;
    }

    if (m_service.handle < BLE_ANCS_MAX_DISCOVERED_CENTRALS)
    {
        mp_service_db[m_service.handle] = m_service;
    }

    memset(&m_service, 0, sizeof(apple_service_t));

    m_service.handle       = INVALID_SERVICE_HANDLE;
    p_ancs->service_handle = INVALID_SERVICE_HANDLE;
    p_ancs->conn_handle    = BLE_CONN_HANDLE_INVALID;
    p_ancs->master_handle  = INVALID_CENTRAL_HANDLE;
}


/**@brief Function for handling of Bond Manager events. 
 */
void ble_ancs_c_on_bondmgmr_evt(ble_ancs_c_t * p_ancs, const ble_bondmngr_evt_t * p_bond_mgmr_evt)
{
    switch (p_bond_mgmr_evt->evt_type)
    {
        case BLE_BONDMNGR_EVT_NEW_BOND:
            p_ancs->master_handle = p_bond_mgmr_evt->central_handle;
            break;

        case BLE_BONDMNGR_EVT_CONN_TO_BONDED_CENTRAL:
            p_ancs->master_handle = p_bond_mgmr_evt->central_handle;
            break;

        default:
            // Do nothing.
            break;
    }
}


/**@brief Function for handling of BLE stack events.
 */
void ble_ancs_c_on_ble_evt(ble_ancs_c_t * p_ancs, const ble_evt_t * p_ble_evt)
{
    uint16_t event = p_ble_evt->header.evt_id;

    switch (m_client_state)
    {
        case STATE_UNINITIALIZED:
            // Initialization is handle in special case, thus if we enter here, it means that an
            // event is received even though we are not initialized --> ignore.
            break;

        case STATE_IDLE:
            if (event == BLE_GAP_EVT_CONNECTED)
            {
                event_connect(p_ancs, p_ble_evt);
            }
            break;

        case STATE_WAITING_ENC:
            if ((event == BLE_GAP_EVT_AUTH_STATUS)|| (event == BLE_GAP_EVT_SEC_INFO_REQUEST))
            {
                event_encrypted_link(p_ancs, p_ble_evt);
            }
            else if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ancs);
            }
            break;

        case STATE_DISC_SERV:
            if (event == BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP)
            {
                event_discover_rsp(p_ancs, p_ble_evt);
            }
            else if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ancs);
            }
            break;

        case STATE_DISC_CHAR:
            if (event == BLE_GATTC_EVT_CHAR_DISC_RSP)
            {
                event_characteristic_rsp(p_ancs, p_ble_evt);
            }
            else if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ancs);
            }
            break;

        case STATE_DISC_DESC:
            if (event == BLE_GATTC_EVT_DESC_DISC_RSP)
            {
                event_descriptor_rsp(p_ancs, p_ble_evt);
            }
            else if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ancs);
            }
            break;

        case STATE_RUNNING:
            if (event == BLE_GATTC_EVT_HVX)
            {
                event_notify(p_ancs, p_ble_evt);
            }
            else if (event == BLE_GATTC_EVT_WRITE_RSP)
            {
                event_write_rsp(p_ancs, p_ble_evt);
            }
            else if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ancs);
            }
            else
            {
                // Do nothing, event not handled in this state.
            }
            break;

        case STATE_RUNNING_NOT_DISCOVERED:
        default:
            if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ancs);
            }
            break;
    }
}


uint32_t ble_ancs_c_init(ble_ancs_c_t * p_ancs, const ble_ancs_c_init_t * p_ancs_init)
{
    if (p_ancs_init->evt_handler == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    p_ancs->evt_handler         = p_ancs_init->evt_handler;
    p_ancs->error_handler       = p_ancs_init->error_handler;
    p_ancs->service_handle      = INVALID_SERVICE_HANDLE;
    p_ancs->master_handle       = INVALID_CENTRAL_HANDLE;
    p_ancs->service_handle      = 0;
    p_ancs->message_buffer_size = p_ancs_init->message_buffer_size;
    p_ancs->p_message_buffer    = p_ancs_init->p_message_buffer;
    p_ancs->conn_handle         = BLE_CONN_HANDLE_INVALID;

    memset(&m_service, 0, sizeof(apple_service_t));
    memset(m_tx_buffer, 0, TX_BUFFER_SIZE);

    m_service.handle = INVALID_SERVICE_HANDLE;
    m_client_state   = STATE_IDLE;
    mp_service_db    = (apple_service_t *) (m_service_db);

    return NRF_SUCCESS;
}


/**@brief Function for creating a TX message for writing a CCCD.
 */
static uint32_t cccd_configure(uint16_t conn_handle, uint16_t handle_cccd, bool enable)
{
    tx_message_t * p_msg;
    uint16_t       cccd_val = enable ? BLE_CCCD_NOTIFY_BIT_MASK : 0;

    if (m_client_state != STATE_RUNNING)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = handle_cccd;
    p_msg->req.write_req.gattc_params.len      = 2;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    p_msg->req.write_req.gattc_value[0]        = LSB(cccd_val);
    p_msg->req.write_req.gattc_value[1]        = MSB(cccd_val);
    p_msg->conn_handle                         = conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    return NRF_SUCCESS;
}


uint32_t ble_ancs_c_enable_notif_notification_source(const ble_ancs_c_t * p_ancs)
{
    return cccd_configure(p_ancs->conn_handle,
                          m_service.notification_source.handle_cccd,
                          true);
}


uint32_t ble_ancs_c_disable_notif_notification_source(const ble_ancs_c_t * p_ancs)
{
    return cccd_configure(p_ancs->conn_handle,
                          m_service.notification_source.handle_cccd,
                          false);
}


uint32_t ble_ancs_c_enable_notif_data_source(const ble_ancs_c_t * p_ancs)
{
    return cccd_configure(p_ancs->conn_handle,
                          m_service.data_source.handle_cccd,
                          true);
}


uint32_t ble_ancs_c_disable_notif_data_source(const ble_ancs_c_t * p_ancs)
{
    return cccd_configure(p_ancs->conn_handle,
                          m_service.data_source.handle_cccd,
                          false);
}

uint32_t ble_ancs_get_notification_attributes(const ble_ancs_c_t * p_ancs, uint8_t * p_uid,
                                              uint8_t num_attr, ble_ancs_attr_list_t *p_attr)
{
    tx_message_t * p_msg;
    uint32_t i = 0;
    
    if (m_client_state != STATE_RUNNING)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = m_service.control_point.handle_value;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;

    p_msg->req.write_req.gattc_value[i++]      = BLE_ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES;
    memcpy(&p_msg->req.write_req.gattc_value[1], p_uid, 4);
    i += 4;
    
    while (num_attr > 0)
    {
      p_msg->req.write_req.gattc_value[i++] = p_attr->attribute_id;
      if (p_attr->attribute_len > 0)
      {
        p_msg->req.write_req.gattc_value[i++] = (uint8_t) (p_attr->attribute_len);
        p_msg->req.write_req.gattc_value[i++] = (uint8_t) (p_attr->attribute_len << 8);
      }
      p_attr++;
      num_attr--;
    }
    
    p_msg->req.write_req.gattc_params.len      = i;
    p_msg->conn_handle                         = p_ancs->conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    return NRF_SUCCESS;

}
