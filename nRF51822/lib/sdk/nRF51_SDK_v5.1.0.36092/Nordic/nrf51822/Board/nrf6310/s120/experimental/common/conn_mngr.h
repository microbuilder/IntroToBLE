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

/**
 * @file conn_mngr.h
 *
 * @defgroup cnxn_mngr Connection Manager
 * @ingroup experimental_api
 * @{
 * @brief Application Interface definition for Connection Manager.
 *
 * @details Connection manager handles connections for the application.
 *          Based on policy definition in conn_mngr_cnfg.h, scanning and connection states
 *          will be managed for a BLE central device.
 */

#ifndef CONN_MNGR_H__
#define CONN_MNGR_H__

#include "nrf_error.h"
#include "ble.h"
#include "ble_gap.h"
#include "conn_mngr_cnfg.h"

/**
 * @defgroup cnxn_mngr_events Connection Manager Events
 * @brief Events notified by connection Manager to the application.
 * @{
 */

/** Events notified to the application */
#define CONN_MNGR_RFU            0x00  /**< Reserved. */
#define CONN_MNGR_ERROR_EVT      0x01  /**< Error Event. */

/** Events related to connection management */
#define CONN_MNGR_SCAN_START_IND       0x02   /**< Indicates scanning has been started. */
#define CONN_MNGR_SCAN_STOP_IND        0x03   /**< Indicates scanning has been stopped. */
#define CONN_MNGR_ADV_REPORT_IND       0x04   /**< Advertising report received from peripheral devices. */
#define CONN_MNGR_CONN_REQUESTED_IND   0x05   /**< Indicates that a connection is requested by the device. */
#define CONN_MNGR_CONN_COMPLETE_IND    0x06   /**< Indicates connection is established. */
#define CONN_MNGR_DISCONNECT_IND       0x07   /**< Indicates disconnection. */
/** @} */

/**
 * @cond
 * @{
 * @defgroup cnxn_mngr_scan_modes Connection Manager Scan Modes
 * @brief Scanning modes supported by the module.
 * @{
 */
 #define CONN_MNGR_SCAN_MODE_UUID       0x01   /**< Scans for devices with a particular UUID. Currently only 16 bit UUID supported. */
 #define CONN_MNGR_SCAN_MODE_DEV_NAME   0x02   /**< Scan for devices with a particular name. */
 #define CONN_MNGR_SCAN_MODE_DEV_ADDR   0x04   /**< Scan only for a specific device */
 /** @} */
 /** @endcond */

 /**
  * @defgroup cnxn_mngr_connect_policy Connection Manager Connection Policy
  * @brief Defines supported policies to initiate connection to a peer.
  * @{
  */
#define CONN_MNGR_CONNECT_POLICY_RSSI        0x01
#define CONN_MNGR_CONNECT_POLICY_UUID16      0x02
#define CONN_MNGR_CONNECT_POLICY_DEV_NAME    0x04
#define CONN_MNGR_CONNECT_POLICY_DEV_ADDR    0x08
/** @} */

 /**
  * @defgroup cnxn_mngr_data_struct Connection Manager Data Structures
  * @brief Data structures defined by the module.
  * @{
  */
/**@brief Defines identifier/handle to uniquely identify application, peer associated with an event or request. */
typedef struct
{
    uint16_t   conn_handle;        /**< Connection handle, uniquely identifying a connection. */
}conn_mngr_handle_t;


/**@brief Defines Event notified to the application */
typedef struct
{
    uint8_t      event_id;         /**< Identifies the event. See \ref cnxn_mngr_events for details on event types and their significance. */
    uint8_t    * p_event_param;    /**< Event parameters, can be null if the event does not have any parameters. */
    uint16_t     event_paramlen;   /**< Length of event parameters, is zero if event does not have any parameters. */
} conn_mngr_event_t;


/**
 * @brief Event notification callback registered by application with the module.
 *
 * @details Event notification callback registered by application with the module when initializing
 *          the module using \ref conn_mngr_register API.
 *
 * @param handle[in]         Application and peer reference handle.
 * @param event[in]          Event and its parameters, if any. See \ref conn_mngr_event_t for details.
 *                           types and their significance.
 * @param event_status[in]   Event status to notify if a procedure was successful or not.
*                            NRF_SUCCESS indicates procedure was successful.
 *
 */
typedef void (*conn_mngr_evt_ntf_cb)
             (
                 conn_mngr_handle_t    * p_handle,
                 conn_mngr_event_t     * p_event,
                 uint32_t                event_status
             );

/**
 * @brief Application Registration Parameters.
 *
 * @details Parameters that application furnishes to the module when registering with it.
 */
typedef struct
{
    conn_mngr_evt_ntf_cb   ntf_cb; /**< Event Handler to be registered to receive asynchronous notification from the module, see \ref cnxn_mngr_events for asynchronous events */
} conn_mngr_app_param_t;

/**
 * @brief Application Registration Parameters.
 *
 * @details Parameters that application furnishes to the module when registering with it.
 */
typedef struct
{
   ble_uuid_t  * uuid_list;    /**< UUID list describing Primary UUIDs application is interested in.  */
   uint8_t       no_of_uuid;   /**< Number of UUIDs application requests. Currently only one UUID is supported. */
} conn_mngr_uuid_list;


/**
 * @brief Scan Preference Mode Parameters.
 *
 * @details Defines parameters associated with supported scan modes.
 */
typedef union
{
    unsigned char         * p_dev_name;      /**< Null terminated string containing device name of target peripheral device. */
    ble_gap_addr_t        * p_target_addr;   /**< Device address of the target peripheral device. */
    conn_mngr_uuid_list   * p_uuid_list;     /**< UUID list of target peripheral device. */
} conn_mngr_scan_mode_param_t;

/** \cond */
/**
 * @brief Defines Scan Preference.
 *
 * @details Defines scan preference that application request when initiating scanning
 *          using the \ref conn_mngr_scan_start.
 */
 typedef struct
 {
    uint8_t                        mode;    /**< Mode to be used for scanning, see \ref cnxn_mngr_scan_modes for supported modes. */
    conn_mngr_scan_mode_param_t    param;   /**< Parameters associated with the mode to be used for scanning. */
 } conn_mngr_scan_param_t;
 
 /**
 * @brief Defines Connection Preference.
 *
 * @details Defines connection preference that application request when initiating scanning
 *          using the \ref conn_mngr_scan_start.
 */
 typedef conn_mngr_scan_param_t conn_mngr_conn_param_t;
 
/** \endcond */
 
 /**
 * @brief Defines Disconnection Parameters.
 *
 * @details Defines disconnection parameters to be used when disconnecting the link.
 */
 typedef struct
 {
    uint8_t   reason; /**< Reason for disconnection. \ref BLE_HCI_STATUS_CODES for possible reasons. */
 } conn_mngr_disc_param_t;

 /** @} */
 
/**
 * @defgroup cnxn_mngr_api Connection Manager APIs
 *
 * @brief This section describes APIs offered by the module.
 *
 * @details This section describes APIs offered by the module; APIs have been categorized to
 *          provide better and specific look up to developers. Categories are:
 *          - a. Set-up APIs.
 *          - b. Connection Management APIs.
 *          - c. Utility APIs.
 * @{
 */

/**
 * @defgroup cnxn_mngr_setup_api Set-up APIs
 *
 * @brief This section describes Module Initialization & Registration APIs needed before
 *        device manager can start managing connections for the application.
 *
 * @{
 */

/**
 * @brief Module Initialization Routine.
 *
 * @details Initializes module. To be called once before any other APIs of the module are used.
 *
 * @return NRF_SUCCESS or appropriate error code, indicating reason for failure.
 */
uint32_t conn_mngr_init(void);

/**
 * @brief Application registration routine.
 *
 * @details This routine is used by the application to register callback for asynchronous events
 *          with the connection manager. Applications shall register before connection management
 *          can be performed for the application.
 *
 * @param[in]  p_param Application parameters.
 *
 * @retval \ref NRF_SUCCESS or an appropriate error code, indicating reason for failure.
 * @retval \ref NRF_ERROR_INVALID_STATE is returned if this API is called before module initialization.
 * @retval \ref NRF_ERROR_NULL is returned if NULL parameter is passed or notification call back is NULL.
 * @retval \ref NRF_ERROR_NO_MEM in case no more registrations can be supported.
 *
 * @note Currently only application can be registered with the module.
 */
uint32_t conn_mngr_register(const conn_mngr_app_param_t * p_param);

/** @} */

/**
 * @defgroup cnxn_mngr_management_api Connection Management APIs
 *
 * @brief This section describes APIs to manage connections.
 * @{
 */

/**
 * @brief Scan & Connect as many connections as module can manage.
 *
 * @details This API scans for peripheral device and initiates connections to
 *          peripheral devices that match the criteria defined in conn_mgr.h.
 *          Number of connections that the device will attempt to establish is
 *          limited to the maximum possible connections the module can managed.
 *          This maximum number is determined by CONN_MNGR_MAX_CONNECTIONS in
 *          conn_mngr_cnfg.h.
 *
 * @return \ref NRF_SUCCESS or appropriate error code, indicating reason for failure.
 */
uint32_t conn_mngr_start(void);


/** \cond */
/**
 * @brief Initiate scanning for peripheral devices.
 *
 * @details Initiates scanning based on preference indicated by application in
 *          scan preference parameters and scan policy setting in conn_mngr_cnfg.h.
 *          For example, in case application wants to perform search based on UUID only,
 *          module will search for primary UUID list described in the conn_mngr_cnfg.h.
 *          Current supported modes for scanning are described in \ref cnxn_mngr_scan_modes.
 *          This API should be used only when application intends to scan but not connect.
 *          Application should use \ref conn_mngr_connect when it intends to scan as well as
 *          connect. CONN_MNGR_SCAN_START_IND will be notified to the application. Application
 *          can take an action to inform the user that local device is scanning.
 *          If scanning stops, application will be notified of CONN_MNGR_SCAN_STOP_IND.
 *          These events will not be received if the API returns a failure.
 *
 * @param[in] p_param Describes scan mode and parameters relevant to the mode.
 *
 * @return ::NRF_SUCCESS or appropriate error code, indicating reason for failure.
 */
uint32_t conn_mngr_scan_start(const conn_mngr_scan_param_t * p_param);

 /**
 * @brief API to Stop scanning for peripheral devices.
 *
 * @details This API stops scanning for peripheral devices. CONN_MNGR_SCAN_START_IND is notified
 *          to the application once scanning has stopped. This event will not be received if the
 *          API returns a failure.
 *
 * @return \ref NRF_SUCCESS or appropriate error code, indicating reason for failure.
 */
uint32_t conn_mngr_scan_stop(void);

/**
 * @brief Routine to request connection to a peripheral device.
 *
 * @details This API attempts to establish a connection with a peripheral. Connection will be
 *          initiated to a peripheral device based on preference indicated by the application.
 *          Modes supported are described in \ref cnxn_mngr_scan_modes. In case scanning devices
 *          is need before connection can be initiated, application is notified of
 *          CONN_MNGR_SCAN_START_IND event. Scanning will be stopped or continued based on
 *          configuration described in the conn_mngr_cnfg.h. Connection parameters to initiate
 *          the connection are also as defined in this file.
 *          CONN_MNGR_CONNECT_IND indicates the end of this connection procedure. Event result
 *          notified to the application along with the event indicates whether connection was
 *          successfully established or not. Handle notified along with the event should be
 *          remembered by the application to initiate any future procedures on the link.
 *          The application can set a reference to be associated with this link using the
 *          utility API \ref conn_mngr_app_context_set.
 *
 * @param[in] p_param Describes scan mode and parameters relevant to the mode.
 *
 * @return \ref NRF_SUCCESS or appropriate error code, indicating reason for failure.
 */
uint32_t conn_mngr_connect(const conn_mngr_conn_param_t * p_param);

/** \endcond */


/**
 * @brief Routine to request disconnection of a link.
 *
 * @details This API initiates disconnection with a peripheral device. CONN_MNGR_DISCONNECT_IND
 *          event is notified to the application when the procedure is complete. Scanning could be
 *          initiated based on configuration in conn_mngr_cnfg.h. If scanning is initiated,
 *          application is notified of CONN_MNGR_SCAN_START_IND.
 *
 * @param[in] p_handle   Identifies the link to be disconnected.
 * @param[in] p_param    Disconnection parameters.
 *
 * @return \ref NRF_SUCCESS or appropriate error code, indicating reason for failure.
 */
uint32_t conn_mngr_disconnect(const conn_mngr_handle_t        * p_handle,
                              const conn_mngr_disc_param_t    * p_param);
/** @} */

/**
 * @defgroup cnxn_mngr_utility_api Utility APIs
 *
 * @brief This section describes utility APIs offered to application. These API are not necessarily
 *        needed for functionality of the module but provide helper APIs that can be useful for the
 *        application such as setting/getting application blob associated with a connection.
 * @{
 */

/**
 * @brief Routine to set an application context associated with a link.
 *
 * @details This utility API allows application to set context that is associated with a link.
 *          Setting this reference is an easy way for application to map a link instance with
 *          the associated context, avoiding searches to establish the  mapping itself.
 *          Module can remember only one context per link, hence application should
 *          not be calling this API to set multiple contexts. However, it could update the
 *          context associated with a link if need be.
 *          The module treats the context set by the application as an opaque blob and does not
 *          associate any meaning with it. Hence no validation on the context is performed apart
 *          from the value not being NULL. Connection manager will remember the pointer to the
 *          context and information pointed to by the pointer.
 *
 * @param[in] p_handle    Identifies the link for which application.
 * @param[in] p_context   Pointer to application context to be mapped with the link.
 *                        Cannot be NULL.
 *
 * @return \ref NRF_SUCCESS or appropriate error code, indicating reason for failure.
 */
uint32_t conn_mngr_app_context_set(const conn_mngr_handle_t    * p_handle,
                                   const void                  * p_context);

/**
 * @brief Routine to get the application context associated with a link.
 *
 * @details This utility API allows application to get a reference to information that is
 *          associated with the link. Application should have set this reference in order
 *          to get it. Therefore, it is a must that application has successfully set
 *          the information using the \ref conn_mngr_app_context_set API.
 *
 * @param[in]  p_handle    Identifies the link for which application.
 * @param[out] pp_context  Application context associated with the link is set if API
 *                         returns NRF_SUCCESS.
 *
 * @return \ref NRF_SUCCESS or appropriate error code, indicating reason for failure.
 */
uint32_t conn_mngr_app_context_get(const conn_mngr_handle_t    *  p_handle,
                                   const  void                 ** pp_context);
/** @} */
/** @} */


/**
 * @brief BLE Event Handler.
 *
 * @details This is the event handle for BLE event and should be called from the
 *          BLE event dispatcher of the application.
 *
 * @param[in]  p_ble_evt BLE Event being notified to the module.
 */
void conn_mngr_ble_evt_handler(ble_evt_t * p_ble_evt);

/** @} */
#endif // CONN_MNGR_H__

