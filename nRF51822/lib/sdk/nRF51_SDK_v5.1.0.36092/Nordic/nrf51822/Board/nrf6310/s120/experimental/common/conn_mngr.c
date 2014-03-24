/* Copyright (C) 2013 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/**@cond
 * @{
 */

/**
 * @file conn_mngr.c
 *
 * @defgroup cnxn_mngr_src Connection Manager Details
 * @ingroup cnxn_mngr
 *
 * @brief Application Interface definition for Connection Manager.
 *
 * @details Connection manager handles connections for the application.
 *          Based on policy definition in conn_mngr_policy.h, scanning
 *          and connection states will be managed for a BLE central device.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "compiler_abstraction.h"
#include "nordic_common.h"
#include "conn_mngr.h"
#include "conn_mngr_cnfg.h"
#include "app_util.h"
#include "debug.h"

/**
 * @defgroup cnxn_mngr_app_states Connection Manager Application States
 * @{
 */
#define APP_IDLE           0x01   /**< Indicates no ongoing activity on application instance. */
#define APP_SCANNING       0x02   /**< Indicates application is scanning and there are no existing connections. */
#define APP_CONNECTING     0x04   /**< Indicates application is attempting to connect and there are no existing connections. */
#define APP_CONNECTED      0x08   /**< Indciated application is connected to one or more devices */ 
#define APP_XCONNECTED     0x88   /**< Indicates application is connected to as many devices it can connect and no more connections can be established. */
/** @} */

/**
 * @defgroup cnxn_mngr_instance_states Connection Manager Instances States.
 * @{
 */
/** Instances states */
#define IDLE             0x01   /**< Indicates connection instance is free. */
#define CONNECTING       0x02   /**< Indicates connection instance is assigned and connection request is in progress */
#define CONNECTED        0x04   /**< Indicates connection is successfully estblished. */
#define DISCONNECTING    0x08   /**< Indicates disconnection is in progress, on BLE_GAP_EVT_DISCONNECTED, connection instance will be freed (set to IDLE). */
/** @} */

#define DONT_CARE_STATE  0xFF   /**< Define used to ignore state informatuin of an instance */
#define UUID16_SIZE      2      /**< Size of 16 bit UUID */


#define UUID16_EXTRACT(DST,SRC)                                                                  \
        do                                                                                       \
        {                                                                                        \
            (*(DST)) = (SRC)[1];                                                                 \
            (*(DST)) <<= 8;                                                                      \
            (*(DST)) |= (SRC)[0];                                                                \
        } while(0)
        
        
/**
 * @defgroup api_param_check API Parameters check macros.
 *
 * @details Macros that verify parameters passed to the module in the APIs. These macros
 *          could be mapped to nothing in final versions of code to save execution and size.
 *
 * @{
 */
/**
 * @brief If parameter is NULL, returns NRF_ERROR_NULL.
 */
#define NULL_PARAM_CHECK(PARAM)                                                                   \
        if ((PARAM) == NULL)                                                                      \
        {                                                                                         \
            return NRF_ERROR_NULL;                                                                \
        }
/**@} */

/**
 * @brief     Verify module's initialization status. Returns NRF_ERROR_INVALID_STATE it is not.
 */
#define VERIFY_MODULE_INITIALIZED()                                                               \
        do                                                                                        \
        {                                                                                         \
            if (!m_module_initialized)                                                            \
            {                                                                                     \
                 return NRF_ERROR_INVALID_STATE;                                                  \
            }                                                                                     \
        } while(0)
        
/**
 * @brief    Verify module's initialization status. Returns in case it is not.
 */
#define VERIFY_MODULE_INITIALIZED_VOID()                                                          \
        do                                                                                        \
        {                                                                                         \
            if (!m_module_initialized)                                                            \
            {                                                                                     \
                 return;                                                                          \
            }                                                                                     \
        } while(0)
        
        
/**
 * @brief  Verify application is registered. Returns NRF_ERROR_INVALID_STATE in case a
 *          module API is called without registering an application with the module.
 */
#define VERIFY_APP_REGISTERED()                                                                   \
        do                                                                                        \
        {                                                                                         \
            if (m_app_table[0].ntf_cb == NULL)                                                    \
            {                                                                                     \
                 return NRF_ERROR_INVALID_STATE;                                                  \
            }                                                                                     \
        } while(0)

        
#define CNXN_MNGR_LOG debug_log
        
/**
 * @brief Defines entry for each application instance table.
 */
typedef struct
{
    conn_mngr_evt_ntf_cb     ntf_cb;       /**< Event Handler to be registered to receive asynchronous notification. NULL implies entry is free. */
    uint8_t                  state;        /**< Application State. See  \ref cnxn_mngr_app_states for possible states.  */
    uint8_t                  cnxn_count;   /**< Number of connections established. */
}app_instance_t;


/**
 * @brief Defines entry structure for connection instance table.
 */
typedef struct
{    
    uint8_t     state;         /**< State, indicative of state of connection. See  \ref cnxn_mngr_instance_states for possible states. An instance is free if state is IDLE */
    uint16_t    cnxn_handle;   /**< Connection handle, uniquely identifying a connection.  */
    void        * p_app_context; /**< Application context that can be remembered per connection. */
    
}cnxn_instance_t;

/**@brief Variable length data encapsulation in terms of length and pointer to data */
typedef struct
{
    uint8_t               * p_data;          /**< Pointer to data. */
    uint16_t                data_len;        /**< Length of data. */
}data_t;

static app_instance_t  m_app_table[CONN_MNGR_MAX_APPLICATIONS];       /**< Applications registered with the module */
static cnxn_instance_t m_cnxn_inst_table[CONN_MNGR_MAX_CONNECTIONS];  /**< Connections managed by the module. */
static bool            m_module_initialized = false;                  /**< Flag for checking if module has been initialized. */


/**
 * @brief Scan parameters requested for scanning and connection. 
 */
static const ble_gap_scan_params_t m_scan_param = 
{
     0,                       // Active scanning set.
     0,                       // Selective scanning not set.
     NULL,                    // White-list not set.
     CONN_MNGR_SCAN_INTERVAL, // Scan interval set to 10 ms.
     CONN_MNGR_SCAN_WINDOW,   // Scan window set to 5 ms.
     0                        // Never stop scanning unless explicit asked to.
};


/**
 * @brief Scan parameters requested for connection. 
 */
static const ble_gap_conn_params_t m_cnxn_param = 
{
    MSEC_TO_UNITS(500, UNIT_1_25_MS),                               // Minimum connection 
    MSEC_TO_UNITS(1000, UNIT_1_25_MS),                              // Maximum connection     
    0,                                                              // Slave latency 
    MSEC_TO_UNITS(CONN_MNGR_SUPERVISION_TIMEOUT, UNIT_10_MS)        // Supervision time-out
};


#if(CONN_MNGR_CONNECT_POLICY == CONN_MNGR_CONNECT_POLICY_DEV_ADDR)
/**
 * @breif Target peripheral address. 
 */
static const ble_gap_addr_t m_target_addr = 
{
    CONN_MNGR_TARGET_ADDR_TYPE,  // Target peripheral address type.
    CONN_MNGR_TARGET_ADDR        // Target peripheral address.
};
#endif // (CONN_MNGR_CONNECT_POLICY == CONN_MNGR_CONNECT_POLICY_DEV_ADDR)


/**
 * @brief Initializes application instance identified by 'index'.
 */
static void app_inst_init(uint32_t index)
{
    m_app_table[index].ntf_cb     = NULL;
    m_app_table[index].cnxn_count = 0;
    m_app_table[index].state      = 0;
}


/**
 * @brief Initializes connection instance identified by 'index'.
 */
static void cnxn_inst_init(uint32_t index)
{
    m_cnxn_inst_table[index].state         = IDLE;
	m_cnxn_inst_table[index].p_app_context = NULL;
	m_cnxn_inst_table[index].cnxn_handle   = BLE_CONN_HANDLE_INVALID;	 
}


/**
 * @brief Provides application instance that is free, if no instances are available, 
 *        NRF_ERROR_NO_MEM is returned.
 */
static __INLINE uint32_t free_app_inst_get (uint32_t * p_inst)
{
    uint32_t retval;
    
    retval = NRF_ERROR_NO_MEM;

    // Currently only one application is supported, hence no loops needed to find free instance.
    if(m_app_table[0].ntf_cb == NULL)
    {
        (*p_inst) = 0;
        retval = NRF_SUCCESS;
    }
    
    return retval;
}


/**
 * @brief Searches for connection instance matching the connection handle requested or state 
 *        requested.
 */
static uint32_t cnxn_inst_find(uint16_t cnxn_handle, uint8_t state, uint32_t * instance)
{
    uint32_t retval;
    uint32_t index;
    
    retval = NRF_ERROR_INVALID_STATE;

    for (index = 0; index < CONN_MNGR_MAX_CONNECTIONS; index++)
    {
        // Search only based on state.
        if (state &  m_cnxn_inst_table[index].state)
        {
            retval = NRF_ERROR_NOT_FOUND;
            
            // Search for matching connection handle.
            if (cnxn_handle == m_cnxn_inst_table[index].cnxn_handle)
            {
                (*instance) = index;
                retval = NRF_SUCCESS;
                break;
            }
        }
    }    
    
    return retval;
}


/**
 * @breif Allocates instance, instance identifier is provided in parameter 'instance' in case
 *        routine succeeds.
 */
static __INLINE uint32_t cnxn_inst_alloc(uint32_t * instance)
{
    uint32_t retval;
        
    retval = cnxn_inst_find(BLE_CONN_HANDLE_INVALID,IDLE,instance);
    if (retval == NRF_SUCCESS)
    {
        m_cnxn_inst_table[*instance].state = CONNECTING;
        m_app_table[0].cnxn_count++;
    }
    else
    {
        retval = NRF_ERROR_NO_MEM;
    }
    
    return retval;
}


/**
 * @breif Frees instance, instance identifier is provided in parameter 'instance' in case
 *        routine succeeds.
 */
static __INLINE void cnxn_inst_free(uint32_t * instance)
{
    if (m_cnxn_inst_table[*instance].state != IDLE)
    {
        cnxn_inst_init(*instance);
        m_app_table[0].cnxn_count--;
    }   
}


/**
 * @brief Parses advertisement data, providing length and location of the field in case
 *        matching data is found.
 *
 * @param[in]  Type of data to be looked for in advertisement data.
 * @param[in]  Advertisement report length and pointer to report.
 * @param[out] If data type requested is found in the data report, type data length and 
 *             pointer to data will be populated here.
 *
 * @retval NRF_SUCCESS if the data type is found in the report.
 * @retval NRF_ERROR_NOT_FOUND if the data type could not be found.
 */
uint32_t adv_report_parse(uint8_t type, data_t * p_advdata, data_t * p_typedata)
{
    uint32_t index = 0;
    uint8_t * p_data;
    
    p_data = p_advdata->p_data;
    
    while (index < p_advdata->data_len)
    {        
        uint8_t field_length = p_data[index];
        uint8_t field_type = p_data[index+1];
        
        if (field_type == type)
        {
            p_typedata->p_data = &p_data[index+2];
            p_typedata->data_len = field_length-1;
            return NRF_SUCCESS;
        }
        index += field_length+1;
    }
    return NRF_ERROR_NOT_FOUND;
}


/**
 * @breif Notifies connection manager event to application.
 */
static __INLINE void app_evt_notify(conn_mngr_handle_t  * p_handle,
                                    conn_mngr_event_t   * p_event,
                                    uint32_t            event_result)
{
    conn_mngr_evt_ntf_cb app_cb = m_app_table[0].ntf_cb;
    
    app_cb(p_handle,p_event,event_result);
}


/**
 * @breif Notifies scan stop event to application.
 */
static __INLINE void app_no_param_evt_notify(uint8_t event_id,uint32_t event_result)
{
    conn_mngr_handle_t handle;
    conn_mngr_event_t  event;
    
    event.event_id = event_id;
    event.event_paramlen = 0;
    event.p_event_param = NULL;
    
    app_evt_notify(&handle,&event,event_result);
}


/**
 * @breif Function to start scan and notify application that scanning has been started.
 */
static uint32_t scan_start(void)
{    
    uint32_t retval = sd_ble_gap_scan_start(&m_scan_param);

    if(retval == NRF_SUCCESS)
    {
        m_app_table[0].state |= APP_SCANNING;
        app_no_param_evt_notify(CONN_MNGR_SCAN_START_IND, NRF_SUCCESS);
    }
    else
    {
        CNXN_MNGR_LOG("[CM]: Scan start failed, reason %d\r\n",retval);
    }
    
    return retval;
}


/**
 * @breif Function to stop scan and notify application that scanning has been started.
 */
static uint32_t scan_stop(void)
{    
    uint32_t retval = sd_ble_gap_scan_stop();

    if (retval == NRF_SUCCESS)
    {
        m_app_table[0].state &= (~APP_SCANNING);
        app_no_param_evt_notify(CONN_MNGR_SCAN_STOP_IND, NRF_SUCCESS);
    }
    else
    {
        CNXN_MNGR_LOG("[CM]: Scan start failed, reason %d\r\n",retval);
    }
    
    return retval;
}


/**
 * @brief Module initialization API.
 */
uint32_t conn_mngr_init(void)
{
    uint32_t index;
    
    for (index = 0; index < CONN_MNGR_MAX_APPLICATIONS; index++)
    {
        app_inst_init(index);
    }
    
    for (index = 0; index < CONN_MNGR_MAX_CONNECTIONS; index++)
    {
        cnxn_inst_init(index);
    }
    
    m_module_initialized = true;
    
    return NRF_SUCCESS;
}


/**
 * @brief Application registration API.
 */
uint32_t conn_mngr_register(const conn_mngr_app_param_t * p_param)
{
    uint32_t retval;
    uint32_t index;
    
    VERIFY_MODULE_INITIALIZED();
    NULL_PARAM_CHECK(p_param);
    NULL_PARAM_CHECK(p_param->ntf_cb);
    
    retval = free_app_inst_get(&index);
    
    if (retval == NRF_SUCCESS)
    {
        m_app_table[index].ntf_cb = p_param->ntf_cb;
        m_app_table[index].state = APP_IDLE;
    }
    
    return retval;
}


#if(AUTO_CONNECT_ON_MATCH!= 1)

/**
 * @brief Scan start API.
 */
uint32_t conn_mngr_scan_start(const conn_mngr_scan_param_t * p_param)
{
    VERIFY_MODULE_INITIALIZED();
    VERIFY_APP_REGISTERED();
    NULL_PARAM_CHECK(p_param);
    
    return scan_start();
}


/**
 * @brief Scan stop API.
 */
uint32_t conn_mngr_scan_stop(void)
{
    VERIFY_MODULE_INITIALIZED();
    VERIFY_APP_REGISTERED();
    
    return scan_stop();
}


/**
 * @brief Request Connection API.
 */
uint32_t conn_mngr_connect(const conn_mngr_scan_param_t * p_param)
{
    uint32_t retval;
    uint32_t index;
    
    VERIFY_MODULE_INITIALIZED();
    VERIFY_APP_REGISTERED();
    NULL_PARAM_CHECK(p_param);
    
    retval = NRF_SUCCESS;
    
    // Verify if a connection is already requested.
    if (m_app_table[0].state == APP_CONNECTING)
    {
        // Currently only one connection can be requested at a time.
        retval = NRF_ERROR_FORBIDDEN;
    }
    
    // Verify is maximum connections that can be managed is reached.
    if (m_app_table[0].state == APP_XCONNECTED)
    {
        // No more instances left to support a new connection.
        retval = NRF_ERROR_NO_MEM;
    }
    
    // All necessary conditions met, allocate an instance and request connection.
    retval = cnxn_inst_alloc(&index);
    
    // Connection instance allocation succeeded.
    if (retval == NRF_SUCCESS)
    {   
        // TODO: Should connection parameters be accepted compile time in this case?
        retval = sd_ble_gap_connect(NULL,&m_scan_param,&m_cnxn_param);
        
        if(retval != NRF_SUCCESS)
        {
            CNXN_MNGR_LOG("[CM]: Connection Request Failed, reason %d\r\n", retval);
            // Connection request failed, free connection instance.
            cnxn_inst_free(&index);
        }
        else
        {
            conn_mngr_event_t event;
            conn_mngr_handle_t handle;
            
            handle.conn_handle = BLE_CONN_HANDLE_INVALID;
            
            // Set global application state to ensure another connection is not
            // requested before this request concludes.
            m_app_table[0].state |= APP_CONNECTING;
            event.event_id = CONN_MNGR_CONN_REQUESTED_IND;
            event.p_event_param = NULL;
            event.event_paramlen = 0;
            app_evt_notify(&handle,&event,NRF_SUCCESS);
        }
    }
    
       
    return retval;
}
#else //AUTO_CONNECT_ON_MATCH


/**
 * @brief Start scanning and connection as many peripherals as it can.
 */
uint32_t conn_mngr_start(void)
{
    VERIFY_MODULE_INITIALIZED();
    VERIFY_APP_REGISTERED();
    
    return scan_start();
}
#endif // AUTO_CONNECT_ON_MATCH


/**
 * @brief Disconnection API.
 */
uint32_t conn_mngr_disconnect(const conn_mngr_handle_t        * p_handle,
                              const conn_mngr_disc_param_t    * p_param)
{
    uint32_t retval;
    uint32_t index;
    
    VERIFY_MODULE_INITIALIZED();
    VERIFY_APP_REGISTERED();
    NULL_PARAM_CHECK(p_handle);
    NULL_PARAM_CHECK(p_param);
    
    retval = cnxn_inst_find(p_handle->conn_handle, CONNECTED, &index);
    
    if (retval == NRF_SUCCESS)
    {        
        retval = sd_ble_gap_disconnect(p_handle->conn_handle, p_param->reason);
        if (retval == NRF_SUCCESS)
        {
            m_cnxn_inst_table[index].state = DISCONNECTING;            
        }
    }
    
    return retval;
}


/**
 * @brief Routine to set an application context associated with a link.
 */
uint32_t conn_mngr_app_context_set(const conn_mngr_handle_t    * p_handle,
                                   const void                  * p_context)
{
    uint32_t retval;
    uint32_t index;
    
    VERIFY_MODULE_INITIALIZED();
    VERIFY_APP_REGISTERED();
    NULL_PARAM_CHECK(p_handle);
    NULL_PARAM_CHECK(p_context);

    retval = cnxn_inst_find(p_handle->conn_handle, CONNECTED, &index);
    
    if (retval == NRF_SUCCESS)
    {
        m_cnxn_inst_table[index].p_app_context =  (void *)p_context;
    }

    return retval;    
}


/**
 * @brief Routine to get the application context associated with a link.
 */
uint32_t conn_mngr_app_context_get(const conn_mngr_handle_t    *  p_handle,
                                   const void                  ** pp_context)
{
    uint32_t retval;
    uint32_t index;
    
    VERIFY_MODULE_INITIALIZED();
    VERIFY_APP_REGISTERED();
    NULL_PARAM_CHECK(p_handle);
    NULL_PARAM_CHECK(pp_context);

    retval = cnxn_inst_find(p_handle->conn_handle, CONNECTED | DISCONNECTING, &index);
    
    if (retval == NRF_SUCCESS)
    {
        (*pp_context) = m_cnxn_inst_table[index].p_app_context;
    }
    
    return retval;
}


/**@breif BLE event handler */
void conn_mngr_ble_evt_handler(ble_evt_t * p_ble_evt)
{
    uint32_t           retval;
    uint32_t           index;
    bool               notify_app = false;
    conn_mngr_handle_t handle;
    conn_mngr_event_t  event;
    uint32_t           event_result;
    
    VERIFY_MODULE_INITIALIZED_VOID();
    
    handle.conn_handle = BLE_CONN_HANDLE_INVALID;
    event_result = NRF_SUCCESS;
    retval = NRF_SUCCESS;
    
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            // Verify if the event is relevant to the application.
            if ((m_app_table[0].state & APP_CONNECTING) == APP_CONNECTING)
            {
                // Only one connection request is pending at a time and connection handle is not known
                // to the device yet.
                retval = cnxn_inst_find(BLE_CONN_HANDLE_INVALID, CONNECTING, &index);
                
                if (retval == NRF_SUCCESS)
                {
                    // Reset connecting state.
                    m_app_table[0].state  &= (~APP_CONNECTING);
                    
                    // Application notification related information.
                    notify_app = true;
                    handle.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
                    event.event_id = CONN_MNGR_CONN_COMPLETE_IND;
                    event.p_event_param = (uint8_t *)(&p_ble_evt->evt.gap_evt.params.connected);
                    event.event_paramlen = sizeof (p_ble_evt->evt.gap_evt.params.connected);
                    event_result = NRF_SUCCESS;
                    
                    m_cnxn_inst_table[index].cnxn_handle = p_ble_evt->evt.gap_evt.conn_handle;
                    m_cnxn_inst_table[index].state = CONNECTED;               
                    
                    notify_app = true;
                    if (m_app_table[0].cnxn_count == CONN_MNGR_MAX_CONNECTIONS)
                    {
                        // Maximum instances that can be managed by the application reached,
                        // stop scanning.
                        m_app_table[0].state = APP_XCONNECTED;
#if 0
                        // Stopping scan when already stopped results in fault. Hence commented out.
                        scan_stop();
#endif // 0
                    }
                    else
                    {                        
                        // Start scanning again.
                        retval = scan_start();
                    }
                }
                else
                {
                     // Unrelated Connection Indication.                    
                }                
            }
            break;
        case BLE_GAP_EVT_DISCONNECTED:
            // Disconnection could be peer or self initiated hence disconnecting and connecting 
            // both states are permitted, however, connection handle should be known.
            retval = cnxn_inst_find(p_ble_evt->evt.gap_evt.conn_handle,
                                    DISCONNECTING | CONNECTED ,
                                    &index);            
            if (retval == NRF_SUCCESS)
            {
                CNXN_MNGR_LOG(
                "[CM]: Disconnect Reaon 0x%04X\r\n",p_ble_evt->evt.gap_evt.params.disconnected.reason);
                m_cnxn_inst_table[index].state = DISCONNECTING;                
                notify_app = true;
                handle.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
                event.event_id = CONN_MNGR_DISCONNECT_IND;
                event.p_event_param = (uint8_t *)(&p_ble_evt->evt.gap_evt.params.disconnected);
                event.event_paramlen = sizeof (p_ble_evt->evt.gap_evt.params.disconnected);
                
                
            }
            else
            {
                // Disconnection unrelated to application managed connection.
                CNXN_MNGR_LOG(
                "[CM]: Failed to find matching connection instance, dropping event.\r\n");
            }
            break;
        case BLE_GAP_EVT_TIMEOUT:
            if(p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN)
            {
                CNXN_MNGR_LOG("[CM]: Scan Timedout.\r\n");
                if ((m_app_table[0].state & APP_SCANNING) == APP_SCANNING)
                {
                    m_app_table[0].state &= (~APP_SCANNING);
                    // Application notification related information.
                    notify_app = true;
                    handle.conn_handle = BLE_CONN_HANDLE_INVALID;
                    event.event_id = CONN_MNGR_SCAN_STOP_IND;
                    event.p_event_param = NULL;
                    event.event_paramlen = 0;
                    event_result = NRF_ERROR_TIMEOUT;
                }
            }
            else if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                CNXN_MNGR_LOG("[CM]: Connection Request Timedout.\r\n");
                // Only one connection request is pending at a time and connection handle is not
                // known to the device yet.
                retval = cnxn_inst_find(BLE_CONN_HANDLE_INVALID, CONNECTING, &index);
                if (retval == NRF_SUCCESS)
                {
                    // Free instance.
                    cnxn_inst_free(&index);
                    
                    // Application notification related information.
                    notify_app = true;
                    handle.conn_handle = BLE_CONN_HANDLE_INVALID;
                    event.event_id = CONN_MNGR_CONN_COMPLETE_IND;
                    event.p_event_param = NULL;
                    event.event_paramlen = 0;
                    event_result = NRF_ERROR_TIMEOUT;
                }
            }
            break;
        case BLE_GAP_EVT_ADV_REPORT:
        {
            int8_t expected_rssi = CONN_MNGR_TARGET_RSSI;  
            
            // If already attempting to connect, do not attempt to create a new connection.
            // Reason: Not permitted by soft device.
            if (((m_app_table[0].state & APP_SCANNING) == APP_SCANNING) &&
                ((m_app_table[0].state & APP_CONNECTING) != APP_CONNECTING) &&
                ((m_app_table[0].state & APP_XCONNECTED) != APP_XCONNECTED) &&
                ((p_ble_evt->evt.gap_evt.params.adv_report.type == BLE_GAP_ADV_TYPE_ADV_IND) ||
                (p_ble_evt->evt.gap_evt.params.adv_report.type == BLE_GAP_ADV_TYPE_ADV_DIRECT_IND)) &&
                (expected_rssi <= p_ble_evt->evt.gap_evt.params.adv_report.rssi))
            {                
                               
                bool initiate_connection = false;                
                 
                #if(CONN_MNGR_CONNECT_POLICY == CONN_MNGR_CONNECT_POLICY_UUID16)
                {                    
                    data_t adv_data;
                    data_t type_data;
                    
                    // Initialize advertisement report for parsing.
                    adv_data.p_data = p_ble_evt->evt.gap_evt.params.adv_report.data;
                    adv_data.data_len = p_ble_evt->evt.gap_evt.params.adv_report.dlen;
                    
                    retval = adv_report_parse(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE,
                                              &adv_data,
                                              &type_data);
                    if (retval != NRF_SUCCESS)
                    {
                        retval = adv_report_parse(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE,
                                                  &adv_data,
                                                  &type_data);
                    }
                    
                    // In case more or complete 16 bit UUID search succeeds, look for target UUID in the data.
                    if (retval == NRF_SUCCESS)
                    {
                        CNXN_MNGR_LOG("[CM]: Adv Report contains 16bit UUID, RSSI %d\r\n",
                        p_ble_evt->evt.gap_evt.params.adv_report.rssi);
                        uint32_t u_index;
                        uint16_t extracted_uuid;
                        
                        // UUIDs found, look for matching UUID
                        for (u_index = 0; u_index < (type_data.data_len/UUID16_SIZE); u_index++)
                        {                            
                            UUID16_EXTRACT(&extracted_uuid,&type_data.p_data[u_index * UUID16_SIZE]);
                            
                            debug_log("\t[CM]: %x\r\n",extracted_uuid);
                            
                            if(extracted_uuid == CONN_MNGR_TARGET_UUID)
                            {
                                // Initiate connection.
                                initiate_connection = true;
                                break;
                            }                            
                        }
                    }                    
                }
                #elif(CONN_MNGR_CONNECT_POLICY == CONN_MNGR_CONNECT_POLICY_DEV_NAME)
                {
                    data_t adv_data;
                    data_t type_data;
                    
                    // Initialize advertisement report for parsing.
                    adv_data.p_data = p_ble_evt->evt.gap_evt.params.adv_report.data;
                    adv_data.data_len = p_ble_evt->evt.gap_evt.params.adv_report.dlen;
                    
                    retval = adv_report_parse(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
                                              &adv_data,
                                              &type_data);
                    if (retval != NRF_SUCCESS) 
                    {
                        // Compare short local name in case complete name does not match.
                        retval = adv_report_parse(BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME,
                                                  &adv_data,
                                                  &type_data);
                    }
                    
                    // Verify if short or complete name matches target.
                    if ((retval == NRF_SUCCESS) &&
                       (0 == memcmp(CONN_MNGR_TARGET_DEV_NAME,type_data.p_data,type_data.data_len)))
                    {
                        // Initiate connection.
                        initiate_connection = true;
                    }
                    
                }
                #elif(CONN_MNGR_CONNECT_POLICY == CONN_MNGR_CONNECT_POLICY_DEV_ADDR)
                {                    
                    if (0 == memcmp(&p_ble_evt->evt.gap_evt.params.adv_report.peer_addr,
                                    &m_target_addr,
                                    sizeof(ble_gap_addr_t))) 
                    {
                        // Initiate connection.
                        initiate_connection = true;
                    }                    
                }
                #endif // CONN_MNGR_CONNECT_POLICY           

                // Initiate connection.
                if (initiate_connection == true)
                {
#if(AUTO_CONNECT_ON_MATCH == 1)                    
                    CNXN_MNGR_LOG("[CM]: Initiating connection\r\n");
                    // Allocate connection instance for new connection.
                    retval = cnxn_inst_alloc(&index);                    
                    if (retval == NRF_SUCCESS)
                    {                        
                        retval = scan_stop();                        
                        
                        retval = sd_ble_gap_connect(&p_ble_evt->evt.gap_evt.params.adv_report.\
                                                    peer_addr,
                                                    &m_scan_param,
                                                    &m_cnxn_param);
                        
                        if (retval != NRF_SUCCESS)
                        {
                            CNXN_MNGR_LOG("[CM]: Connection Request Failed, reason %d\r\n", retval);
                            // Connection request failed, free connection instance.
                            cnxn_inst_free(&index);
                        }
                        else
                        {
                            // Set global application state to ensure another connection is not
                            // requested before this request concludes.
                            m_app_table[0].state |= APP_CONNECTING;
                            notify_app = true;
                            event.event_id = CONN_MNGR_CONN_REQUESTED_IND;
                            event.p_event_param = 
                            (uint8_t *)(&p_ble_evt->evt.gap_evt.params.adv_report.peer_addr);
                            event.event_paramlen = sizeof (ble_gap_addr_t);
                            event_result = NRF_SUCCESS;
                        }
                    }
                    else
                    {
                        // Cannot initiate a new connection, no instances free.
                        // Should never reach this state as XCONNECTED state should take care of this.
                    }
#else
                    notify_app = true;
                    event.event_id = CONN_MNGR_ADV_REPORT_IND;
                    event.p_event_param = (uint8_t *)(&p_ble_evt->evt.gap_evt.params.adv_report);
                    event.event_paramlen = sizeof (p_ble_evt->evt.gap_evt.params.adv_report);
                    event_result = NRF_SUCCESS;
#endif // (AUTO_CONNECT_ON_MATCH == 1)
                }
            }            
        }
        break;
        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
            CNXN_MNGR_LOG("[CM]: Connection Parameter Update request received, accepting!\r\n");
            retval = sd_ble_gap_conn_param_update(p_ble_evt->evt.gap_evt.conn_handle,
                                                   &p_ble_evt->evt.gap_evt.params.\
                                                   conn_param_update.conn_params);
            if(retval != NRF_SUCCESS)
            {
                CNXN_MNGR_LOG(
                "[CM]: Connection parameter update request failed, reason %d\r\n",retval);
            }
            break;
        default:
            break;
    }
    
    if (retval != NRF_SUCCESS)
    {
        // TODO: Notify error event to application?
    }

    if (notify_app)
    {
        app_evt_notify(&handle,&event,event_result);
        // Freeing the instance after event is notified so that application can get the context.
        if (event.event_id == CONN_MNGR_DISCONNECT_IND)
        {
            // Free instance.
            cnxn_inst_free(&index);
            
            if (m_app_table[0].state == APP_XCONNECTED)
            {
                m_app_table[0].state &= (~APP_XCONNECTED);
                
                // Initiate scanning.
                retval = scan_start();
                if (retval != NRF_SUCCESS)
                {
                     CNXN_MNGR_LOG(
                     "[CM]: Scan start failed, reason %d\r\n", retval);
                }
            }                        
        }
    }      
}

/** @}
 *  @endcond
 */
