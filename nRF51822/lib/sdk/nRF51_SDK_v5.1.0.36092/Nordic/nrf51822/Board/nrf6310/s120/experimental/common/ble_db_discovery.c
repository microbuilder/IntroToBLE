/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 */

/**@cond To Make Doxygen skip documentation generation for this file.
 * @{
 */

#include "ble_db_discovery.h"
#include "nrf_error.h"
#include "ble.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

#define DB_LOG debug_log

#define SRV_DISC_START_HANDLE         0x0001           /**< Start handle value used during service discovery. */

/**@brief Array of structures containing information about the registered application modules. */
static struct
{
    uint16_t                          srv_uuid;        /**< UUID of the service for which the application module had registered itself.*/
    ble_db_discovery_evt_handler_t    evt_handler;     /**< Event handler of the application module to be called in case there are any events.*/
} m_registered_modules[BLE_DB_DISCOVERY_MAX_USERS];

static ble_uuid_t m_srv_uuid;
static uint8_t    m_num_of_modules_reg = 0;            /**< Number of modules registered with the DB Discovery module. */
static bool       m_initialized        = false;        /**< Variable to indicate that the module is initialized. */

/**@brief     Function for fetching the event handler and the associated event memory provided by
 *            a registered application module.
 *
 * @param[in]       srv_uuid                 UUID of the service.
 * @param[in, out]  pp_evt_mem_alloc_by_app  Pointer to the memory to be used for populating the
 *                                           event.
 *
 * @return    Event handler of the module registered for the given service UUID.
 */
static ble_db_discovery_evt_handler_t registered_handler_get(uint16_t srv_uuid)
{
    uint8_t i;

    for (i = 0; i < m_num_of_modules_reg; i++)
    {
        if (m_registered_modules[i].srv_uuid == srv_uuid)
        {
            return (m_registered_modules[i].evt_handler);
        }
    }

    return NULL;
}


/**@brief     Function for storing the event handler and the associated event memory provided by
 *            a registered application module.
 *
 * @param[in] srv_uuid                UUID of the service.
 * @param[in] p_evt_handler           Event handler provided by the application.
 * @param[in] p_evt_mem_alloc_by_app  Memory provided by the application module to store and
 *                                    forward events.
 *
 * @retval    NRF_SUCCESS if the handler and memory was stored.
 * @retval    NRF_ERROR_NO_MEM if there is no space left to store the handler.
 */
static uint32_t registered_handler_set(uint16_t                       srv_uuid,
                                       ble_db_discovery_evt_handler_t p_evt_handler)
{
    if (m_num_of_modules_reg < BLE_DB_DISCOVERY_MAX_USERS)
    {
        m_registered_modules[m_num_of_modules_reg].srv_uuid               = srv_uuid;
        m_registered_modules[m_num_of_modules_reg].evt_handler            = p_evt_handler;

        m_num_of_modules_reg++;

        return NRF_SUCCESS;
    }
    else
    {
        return NRF_ERROR_NO_MEM;
    }
}


static void indicate_error_to_app(ble_db_discovery_t * const   p_db_discovery,
                                  const ble_gattc_evt_t* const p_ble_gattc_evt,
                                  uint32_t                     err_code)
{
    // Indicate the error to the user application registered for the service being
    // discovered.
    ble_db_discovery_evt_t         evt;
    ble_db_discovery_evt_handler_t p_evt_handler;

    p_evt_handler = registered_handler_get(p_db_discovery->srv_being_discovered.srv_uuid.uuid);

    evt.conn_handle     = p_ble_gattc_evt->conn_handle;
    evt.evt_type        = BLE_DB_DISCOVERY_ERROR;
    evt.params.err_code = err_code;

    p_evt_handler(&evt);
}


static uint32_t characteristics_discover(ble_db_discovery_t * const p_db_discovery)
{
    ble_gattc_service_t * p_curr_srv = &(p_db_discovery->services[p_db_discovery->curr_srv_ind]);
    ble_gattc_handle_range_t handle_range;

    if (p_db_discovery->curr_char_ind != 0)
    {
        // This is not the first characteristic being discovered. Hence the 'start handle' to be
        // be used must be computed using the handle_value of the previous characteristic.
        ble_gattc_char_t * p_prev_char;
        uint8_t            prev_char_ind = p_db_discovery->curr_char_ind - 1;

        p_prev_char = &(p_db_discovery->srv_being_discovered.charateristics[prev_char_ind].
                        characteristic);

        handle_range.start_handle = p_prev_char->handle_value + 1;
    }
    else
    {
        // This is the first characteristic of this service being discovered.
        handle_range.start_handle = p_curr_srv->handle_range.start_handle;
    }

    handle_range.end_handle = p_curr_srv->handle_range.end_handle;

    return sd_ble_gattc_characteristics_discover(p_db_discovery->conn_handle,
                                                 &handle_range);
}


/**@brief   Function for handling primary service discovery response.
 *
 * @param[in] p_db_discovery    Pointer to the DB Discovery structure.
 * @param[in] p_ble_gattc_evt   Pointer to the GATT Client event.
 */
static void on_prim_srv_disc_rsp(ble_db_discovery_t * const     p_db_discovery,
                                 const ble_gattc_evt_t * const  p_ble_gattc_evt)
{
    uint32_t                                   err_code;
    const ble_gattc_evt_prim_srvc_disc_rsp_t * p_prim_srvc_disc_rsp_evt;

    p_prim_srvc_disc_rsp_evt = &(p_ble_gattc_evt->params.prim_srvc_disc_rsp);

    // Store the service handles in the p_db_discovery structure for future reference.
    p_db_discovery->srv_count = p_prim_srvc_disc_rsp_evt->count;

    uint8_t i;

    for (i = 0; i < p_db_discovery->srv_count; i++)
    {
        p_db_discovery->services[i] = p_prim_srvc_disc_rsp_evt->services[i];
    }

    p_db_discovery->curr_srv_ind = 0;

    err_code = characteristics_discover(p_db_discovery);

    if (err_code != NRF_SUCCESS)
    {
        // Indicate the error to the user application registered for the service being discovered.
        indicate_error_to_app(p_db_discovery, p_ble_gattc_evt, err_code);
    }
}


static bool is_desc_discover_reqd(ble_db_discovery_t *       p_db_discovery,
                                  ble_db_discovery_char_t *  p_curr_char,
                                  ble_db_discovery_char_t *  p_next_char,
                                  ble_gattc_handle_range_t * p_handle_range)
{
    if (p_next_char == NULL)
    {
        // Check if the value handle of the current characteristic is equal to the service end
        // handle.
        if (
            p_curr_char->characteristic.handle_value
            ==
            p_db_discovery->services[p_db_discovery->curr_srv_ind].handle_range.end_handle
           )
        {
            // No descriptors can be found for the current characteristic.
            return false;
        }

        p_handle_range->start_handle = p_curr_char->characteristic.handle_value + 1;
        // The current characteristic is the last characteristic in the service. Hence the end
        // handle should be the end handle of the service.
        p_handle_range->end_handle   =
                     p_db_discovery->services[p_db_discovery->curr_srv_ind].handle_range.end_handle;

        return true;
    }
    if (
        (p_curr_char->characteristic.handle_value + 1)
         ==
         p_next_char->characteristic.handle_decl
    )
    {
        // No descriptors can exist between the two characteristic.
        return false;
    }

    p_handle_range->start_handle = p_curr_char->characteristic.handle_value + 1;
    p_handle_range->end_handle   = p_next_char->characteristic.handle_decl - 1;

    return true;
}


static void discovery_complete_evt_trigger(ble_db_discovery_t * const p_db_discovery)
{
    ble_db_discovery_evt_t          evt;
    ble_db_discovery_evt_handler_t  p_evt_handler;

    // No (more) descriptors found. Send an event to the application.
    p_evt_handler = registered_handler_get(p_db_discovery->srv_being_discovered.srv_uuid.uuid);
    
    if (p_evt_handler != NULL)
    {
        evt.evt_type             = BLE_DB_DISCOVERY_COMPLETE;
        evt.conn_handle          = p_db_discovery->conn_handle;
        evt.params.discovered_db = p_db_discovery->srv_being_discovered;

        uint8_t                   i = 0;
        ble_db_discovery_char_t * p_chars_being_discovered =
                                          &(p_db_discovery->srv_being_discovered.charateristics[0]);

        for (i = 0; i < evt.params.discovered_db.char_count; i++)
        {
            evt.params.discovered_db.charateristics[i].cccd_handle    =
                            p_chars_being_discovered[i].cccd_handle;
            evt.params.discovered_db.charateristics[i].characteristic =
                            p_chars_being_discovered[i].characteristic;
        }

        // Send the event to the registered user module.
        p_evt_handler(&evt);
    }
}


static uint32_t descriptors_discover(ble_db_discovery_t * const p_db_discovery)
{
    ble_gattc_handle_range_t    handle_range;
    ble_db_discovery_char_t  *  p_curr_char_being_discovered;
    bool                        is_discovery_reqd = false;

    p_curr_char_being_discovered =
                &p_db_discovery->srv_being_discovered.charateristics[p_db_discovery->curr_char_ind];

    if (p_db_discovery->curr_char_ind + 1 == p_db_discovery->srv_being_discovered.char_count)
    {
         is_discovery_reqd = is_desc_discover_reqd(p_db_discovery,
                                                   p_curr_char_being_discovered,
                                                   NULL,
                                                   &handle_range);
    }
    else
    {
        uint8_t                   i;
        ble_db_discovery_char_t * p_next_char;

        for (i = p_db_discovery->curr_char_ind;
             i < p_db_discovery->srv_being_discovered.char_count;
             i++)
        {

            if (i == (p_db_discovery->srv_being_discovered.char_count - 1))
            {
                // The current characteristic is the last characteristic in the service.
                p_next_char = NULL;
            }
            else
            {
                p_next_char = &(p_db_discovery->srv_being_discovered.charateristics[i + 1]);
            }

            // Check if there is a possibility of a descriptor existing for the current
            // characteristic.
            if (is_desc_discover_reqd(p_db_discovery,
                                      p_curr_char_being_discovered,
                                      p_next_char,
                                      &handle_range))
            {
                is_discovery_reqd = true;
                break;
            }
            else
            {
                // No descriptors can exist.

                p_curr_char_being_discovered = p_next_char;
                p_db_discovery->curr_char_ind++;
            }
        }
    }

    if (!is_discovery_reqd)
    {
        // No more descriptor discovery required.
        // Discovery is complete.
        // Send a discovery complete event to the user application.
        
        DB_LOG("[DB]: DB Discovery complete \r\n");
        
        discovery_complete_evt_trigger(p_db_discovery);

        return NRF_SUCCESS;
    }

    return sd_ble_gattc_descriptors_discover(p_db_discovery->conn_handle,
                                             &handle_range);
}


/**@brief   Function for handling characteristic discovery response.
 *
 * @param[in] p_db_discovery    Pointer to the DB Discovery structure.
 * @param[in] p_ble_gattc_evt   Pointer to the GATT Client event.
 */
static void on_char_disc_rsp(ble_db_discovery_t * const     p_db_discovery,
                             const ble_gattc_evt_t * const  p_ble_gattc_evt)
{
    uint32_t                                   err_code;
    const ble_gattc_evt_char_disc_rsp_t *      p_char_disc_rsp_evt;

    p_char_disc_rsp_evt = &(p_ble_gattc_evt->params.char_disc_rsp);

    if (p_char_disc_rsp_evt->count <= BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV)
    {
        p_db_discovery->srv_being_discovered.char_count = p_char_disc_rsp_evt->count;
    }
    else
    {
        // The number of characteristics discovered at the peer is more than the supported
        // maximum. This module will store only the first found supported number of characteristic.
        p_db_discovery->srv_being_discovered.char_count = BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV;
    }

    uint16_t i;

    for (i = 0; i < p_char_disc_rsp_evt->count; i++)
    {
        p_db_discovery->srv_being_discovered.charateristics[i].characteristic =
                                                                      p_char_disc_rsp_evt->chars[i];

        p_db_discovery->srv_being_discovered.charateristics[i].cccd_handle    =
                                                                      BLE_GATT_HANDLE_INVALID;
    }

    p_db_discovery->curr_char_ind = 0;

    err_code = descriptors_discover(p_db_discovery);

    if (err_code != NRF_SUCCESS)
    {
        indicate_error_to_app(p_db_discovery, p_ble_gattc_evt, err_code);
    }
}


/**@brief   Function for handling descriptor discovery response.
 *
 * @param[in] p_db_discovery    Pointer to the DB Discovery structure.
 * @param[in] p_ble_gattc_evt   Pointer to the GATT Client event.
 */
static void on_desc_disc_rsp(ble_db_discovery_t * const     p_db_discovery,
                             const ble_gattc_evt_t * const  p_ble_gattc_evt)
{
    const ble_gattc_evt_desc_disc_rsp_t * p_desc_disc_rsp_evt;

    p_desc_disc_rsp_evt = &(p_ble_gattc_evt->params.desc_disc_rsp);

    ble_db_discovery_char_t * p_char_being_discovered =
              &(p_db_discovery->srv_being_discovered.charateristics[p_db_discovery->curr_char_ind]);

    if (p_ble_gattc_evt->gatt_status == BLE_GATT_STATUS_SUCCESS)
    {
        // The descriptor was found at the peer.
        // If the descriptor was a CCCD, then the cccd_handle needs to be populated.

        uint8_t i;

        // Loop through all the descriptors to find the CCCD.
        for (i = 0; i < p_desc_disc_rsp_evt->count; i++)
        {
            if (
                p_desc_disc_rsp_evt->descs[i].uuid.uuid
                ==
                BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG
               )
            {
                // CCCD found. Store the CCCD handle.
                DB_LOG("[DB]: Storing CCCD Handle %d\r\n", p_desc_disc_rsp_evt->descs[i].handle);
                
                p_char_being_discovered->cccd_handle = p_desc_disc_rsp_evt->descs[i].handle;

                break;
            }
        }
    }

    if ((p_db_discovery->curr_char_ind + 1) == p_db_discovery->srv_being_discovered.char_count)
    {
        // No more characteristics and descriptors need to be discovered. Discovery is complete.
        // Send a discovery complete event to the user application.
        DB_LOG("[DB]: DB Discovery complete \r\n");
        
        discovery_complete_evt_trigger(p_db_discovery);
    }
    else
    {
        // Begin discovery of descriptors of the next characteristic.
        uint32_t err_code;

        p_db_discovery->curr_char_ind++;

        err_code = descriptors_discover(p_db_discovery);

        if (err_code != NRF_SUCCESS)
        {
            // Indicate the error to the user application registered for the service being
            // discovered.
            indicate_error_to_app(p_db_discovery, p_ble_gattc_evt, err_code);
        }
    }
}


uint32_t ble_db_discovery_init(ble_db_discovery_init_t * p_db_discovery_init)
{
    if (p_db_discovery_init == NULL)
    {
        return NRF_ERROR_NULL;
    }

    m_num_of_modules_reg = 0;
    m_initialized        = true;

    return NRF_SUCCESS;
}


uint32_t ble_db_discovery_close()
{
    m_num_of_modules_reg = 0;
    m_initialized        = false;

    return NRF_SUCCESS;
}


uint32_t ble_db_discovery_register(const ble_uuid_t * const              p_uuid,
                                   const ble_db_discovery_evt_handler_t  evt_handler)
{
    if ((p_uuid == NULL) || (evt_handler == NULL))
    {
        return NRF_ERROR_NULL;
    }

    if (!m_initialized)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    if (m_num_of_modules_reg == BLE_DB_DISCOVERY_MAX_USERS)
    {
        // The limit on the maximum number of users supported has reached. Return error.
        return NRF_ERROR_NOT_SUPPORTED;
    }

    BLE_UUID_COPY_INST(m_srv_uuid, *p_uuid);

    return registered_handler_set(p_uuid->uuid, evt_handler);
}


uint32_t ble_db_discovery_start(ble_db_discovery_t * const p_db_discovery,
                                uint16_t                   conn_handle)
{
    if (p_db_discovery == NULL)
    {
        return NRF_ERROR_NULL;
    }

    if (!m_initialized)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    p_db_discovery->srv_being_discovered.srv_uuid = m_srv_uuid;
    p_db_discovery->conn_handle                   = conn_handle;
    
    DB_LOG("[DB]: Starting service discovery\r\n");
    
    return sd_ble_gattc_primary_services_discover(p_db_discovery->conn_handle,
                                                  SRV_DISC_START_HANDLE,
                                                  &m_srv_uuid);
}


void ble_db_discovery_on_ble_evt(ble_db_discovery_t * const p_db_discovery,
                                 const ble_evt_t * const    p_ble_evt)
{
    if ((p_db_discovery == NULL) || (p_ble_evt == NULL))
    {
        // Null pointer provided. Return.
        return;
    }

    if (!m_initialized)
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            p_db_discovery->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;
        
        case BLE_GAP_EVT_DISCONNECTED:
            memset(p_db_discovery, 0 , sizeof(ble_db_discovery_t));
            p_db_discovery->conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
            on_prim_srv_disc_rsp(p_db_discovery, &(p_ble_evt->evt.gattc_evt));
            break;

        case BLE_GATTC_EVT_CHAR_DISC_RSP:
            on_char_disc_rsp(p_db_discovery, &(p_ble_evt->evt.gattc_evt));
            break;

        case BLE_GATTC_EVT_DESC_DISC_RSP:
            on_desc_disc_rsp(p_db_discovery, &(p_ble_evt->evt.gattc_evt));
            break;

        default:
            break;
    }
}

/** @}
 *  @endcond
 */
