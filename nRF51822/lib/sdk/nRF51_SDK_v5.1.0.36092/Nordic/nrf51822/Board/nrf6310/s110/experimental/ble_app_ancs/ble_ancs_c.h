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

/** @file
 *
 * @defgroup ble_sdk_srv_ancs_c Apple Notification Service Client
 * @{
 * @ingroup ble_sdk_srv
 * @brief Apple Notification module - Disclaimer: This module (Apple Notification Center Service) can and will be changed at any time by either Apple or Nordic Semiconductor ASA.
 *
 * @details This module implements the Apple Notification Center Service (ANCS) Client.
 *
 * @note The application must propagate BLE stack events to the Apple Notification Client module
 *       by calling ble_ancs_c_on_ble_evt() from the from the @ref ble_stack_handler callback.
 */
#ifndef BLE_ANCS_C_H__
#define BLE_ANCS_C_H__

#include "ble.h"
#include "ble_gatts.h"
#include "ble_types.h"
#include "ble_bondmngr.h"
#include "ble_bondmngr_cfg.h"

#define ANCS_NB_OF_CHARACTERISTICS                  5                                    /**< Number of characteristics as defined by Apple Notification Service specification. */
#define ANCS_NB_OF_SERVICES                         1                                    /**< Number of services supported in one master. */

#define INVALID_SERVICE_HANDLE_BASE                 0xF0                                 /**< Base for indicating invalid service handle. */
#define INVALID_SERVICE_HANDLE                      (INVALID_SERVICE_HANDLE_BASE + 0x0F) /**< Indication that the current service handle is invalid. */
#define INVALID_SERVICE_HANDLE_DISC                 (INVALID_SERVICE_HANDLE_BASE + 0x0E) /**< Indication that the current service handle is invalid but the service has been discovered. */
#define BLE_ANCS_INVALID_HANDLE                     0xFF                                 /**< Indication that the current service handle is invalid. */

#define ANCS_ATTRIBUTE_DATA_MAX                     32                                   /*<< Maximium notification attribute data length. */


#define BLE_UUID_APPLE_NOTIFICATION_CENTER_SERVICE  0xf431                               /*<< ANCS service UUID. */
#define BLE_UUID_ANCS_CONTROL_POINT_CHAR            0xd8f3                               /*<< Control point UUID. */
#define BLE_UUID_ANCS_NOTIFICATION_SOURCE_CHAR      0x120d                               /*<< Notification source UUID. */
#define BLE_UUID_ANCS_DATA_SOURCE_CHAR              0xc6e9                               /*<< Data source UUID. */


// Forward declaration of the ble_ancs_c_t type.
typedef struct ble_ancs_c_s ble_ancs_c_t;

/**@brief Event types that are passed from client to application on an event. */
typedef enum
{
    BLE_ANCS_C_EVT_DISCOVER_COMPLETE,          /**< A successful connection has been established and the characteristics of the server has been fetched. */
    BLE_ANCS_C_EVT_DISCOVER_FAILED,            /**< It was not possible to discover service or characteristics of the connected peer. */
    BLE_ANCS_C_EVT_IOS_NOTIFICATION,           /**< An iOS notification was received on the notification source control point. */
    BLE_ANCS_C_EVT_NOTIF_ATTRIBUTE             /**< A received iOS a received notification attribute has been completely parsed and reassembled. */
} ble_ancs_c_evt_type_t;

/**@brief Category IDs for iOS notifications. */
typedef enum
{
    BLE_ANCS_CATEGORY_ID_OTHER,
    BLE_ANCS_CATEGORY_ID_INCOMING_CALL,
    BLE_ANCS_CATEGORY_ID_MISSED_CALL,
    BLE_ANCS_CATEGORY_ID_VOICE_MAIL,
    BLE_ANCS_CATEGORY_ID_SOCIAL,
    BLE_ANCS_CATEGORY_ID_SCHEDULE,
    BLE_ANCS_CATEGORY_ID_EMAIL,
    BLE_ANCS_CATEGORY_ID_NEWS,
    BLE_ANCS_CATEGORY_ID_HEALTH_AND_FITNESS,
    BLE_ANCS_CATEGORY_ID_BUSINESS_AND_FINANCE,
    BLE_ANCS_CATEGORY_ID_LOCATION,
    BLE_ANCS_CATEGORY_ID_ENTERTAINMENT
} ble_ancs_category_id_values_t;

/**@brief Event IDs for iOS notifications. */
typedef enum
{
    BLE_ANCS_EVENT_ID_NOTIFICATION_ADDED,
    BLE_ANCS_EVENT_ID_NOTIFICATION_MODIFIED,
    BLE_ANCS_EVENT_ID_NOTIFICATION_REMOVED
} ble_ancs_event_id_values_t;

/**@brief Event flags for iOS notifications. */
#define BLE_ANCS_EVENT_FLAG_SILENT                (1 << 0)
#define BLE_ANCS_EVENT_FLAG_IMPORTANT             (1 << 1)


/**@brief Control point command IDs. */
typedef enum
{
    BLE_ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES,
    BLE_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES
} ble_ancs_command_id_values_t;

/**@brief Notification attribute IDs. */
typedef enum
{
    BLE_ANCS_NOTIFICATION_ATTRIBUTE_ID_APP_IDENTIFIER,
    BLE_ANCS_NOTIFICATION_ATTRIBUTE_ID_TITLE,
    BLE_ANCS_NOTIFICATION_ATTRIBUTE_ID_SUBTITLE,
    BLE_ANCS_NOTIFICATION_ATTRIBUTE_ID_MESSAGE,
    BLE_ANCS_NOTIFICATION_ATTRIBUTE_ID_MESSAGE_SIZE,
    BLE_ANCS_NOTIFICATION_ATTRIBUTE_ID_DATE,
} ble_ancs_notification_attribute_id_values_t;

typedef struct {
    uint8_t                            event_id;
    uint8_t                            event_flags;
    uint8_t                            category_id;
    uint8_t                            category_count;
    uint8_t                            notification_uid[4];
} ble_ancs_c_evt_ios_notification_t;

typedef struct {
    uint8_t                            command_id;
    uint8_t                            notification_uid[4];
    uint8_t                            attribute_id;
    uint16_t                           attribute_len;
    uint8_t                            data[ANCS_ATTRIBUTE_DATA_MAX];
} ble_ancs_c_evt_notif_attribute_t;

typedef struct {
    uint8_t                            attribute_id;
    uint16_t                           attribute_len;
} ble_ancs_attr_list_t;

/**@brief Apple Notification Event structure
 *
 * @details The structure contains the event that should be handled, as well as
 *          additional information.
 */
typedef struct
{
    ble_ancs_c_evt_type_t               evt_type;                                         /**< Type of event. */
    ble_uuid_t                          uuid;                                             /**< UUID of the event in case of an apple or notification. */
    union
    {
        ble_ancs_c_evt_ios_notification_t   notification;
        ble_ancs_c_evt_notif_attribute_t    attribute;
        uint32_t                        error_code;                                       /**< Additional status/error code if the event was caused by a stack error or gatt status, e.g. during service discovery. */
    } data;
} ble_ancs_c_evt_t;

/**@brief Apple Notification event handler type. */
typedef void (*ble_ancs_c_evt_handler_t) (ble_ancs_c_evt_t * p_evt);

/**@brief Apple Notification structure. This contains various status information for the client. */
typedef struct ble_ancs_c_s
{
    ble_ancs_c_evt_handler_t            evt_handler;                                      /**< Event handler to be called for handling events in the Apple Notification Client Application. */
    ble_srv_error_handler_t             error_handler;                                    /**< Function to be called in case of an error. */
    uint16_t                            conn_handle;                                      /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    int8_t                              master_handle;                                    /**< Handle for the currently connected master if we have a bond in the bond manager. */
    uint8_t                             service_handle;                                   /**< Handle to the service in the database to use for this instance. */
    uint32_t                            message_buffer_size;                              /**< Size of message buffer to hold the additional text messages received on notifications. */
    uint8_t *                           p_message_buffer;                                 /**< Pointer to the buffer to be used for additional text message handling. */
} ble_ancs_c_t;

/**@brief Apple Notification init structure. This contains all options and data needed for
 *        initialization of the client.*/
typedef struct
{
    ble_ancs_c_evt_handler_t            evt_handler;                                      /**< Event handler to be called for handling events in the Battery Service. */
    ble_srv_error_handler_t             error_handler;                                    /**< Function to be called in case of an error. */
    uint32_t                            message_buffer_size;                              /**< Size of buffer to handle messages. */
    uint8_t *                           p_message_buffer;                                 /**< Pointer to buffer for passing messages. */
} ble_ancs_c_init_t;


/**@brief Apple Notification Center Service UUIDs */
extern const ble_uuid128_t ble_ancs_base_uuid128;                                         /**< Service UUID. */
extern const ble_uuid128_t ble_ancs_cp_base_uuid128;                                      /**< Control point UUID. */
extern const ble_uuid128_t ble_ancs_ns_base_uuid128;                                      /**< Notification source UUID. */
extern const ble_uuid128_t ble_ancs_ds_base_uuid128;                                      /**< Data source UUID. */


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the ANCS Client.
 *
 * @param[in]   p_ancs     ANCS Client structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
void ble_ancs_c_on_ble_evt(ble_ancs_c_t * p_ancs, const ble_evt_t * p_ble_evt);


/**@brief Function for handling the ANCS Client - Bond Manager stack event.
 *
 * @details Handles all events from the Bond Manager of interest to the ANCS Client.
 *          The ANCS Client will use the events of re-connection to existing master
 *          and creation of new bonds for handling of service discovery and writing of the Apple
 *          Notification Control Point for re-send of New Apple and Unread Apple notifications.
 *
 * @param[in]   p_ancs           ANCS Client structure.
 * @param[in]   p_bond_mgmr_evt  Event received from the Bond Manager.
 */
void ble_ancs_c_on_bondmgmr_evt(ble_ancs_c_t * p_ancs, const ble_bondmngr_evt_t * p_bond_mgmr_evt);


/**@brief Function for initializing the ANCS Client.
 *
 * @param[out]  p_ancs       ANCS Client structure. This structure will have to be
 *                           supplied by the application. It will be initialized by this function,
 *                           and will later be used to identify this particular client instance.
 * @param[in]   p_ancs_init  Information needed to initialize the client.
 *
 * @return      NRF_SUCCESS on successful initialization of client, otherwise an error code.
 */
uint32_t ble_ancs_c_init(ble_ancs_c_t * p_ancs, const ble_ancs_c_init_t * p_ancs_init);


/**@brief Function for writing the to CCCD to enable notifications from the Apple Notification Service.
 *
 * @param[in]  p_ancs      Apple Notification structure. This structure will have to be supplied by
 *                         the application. It identifies the particular client instance to use.
 *
 * @return     NRF_SUCCESS on successful writing of the CCCD, otherwise an error code.
 */
uint32_t ble_ancs_c_enable_notif_notification_source(const ble_ancs_c_t * p_ancs);


/**@brief Function for writing to the CCCD to enable data souce notifications from the Apple Notification Service.
 *
 * @param[in]  p_ancs      Apple Notification structure. This structure will have to be supplied by
 *                         the application. It identifies the particular client instance to use.
 *
 * @return     NRF_SUCCESS on successful writing of the CCCD, otherwise an error code.
 */
uint32_t ble_ancs_c_enable_notif_data_source(const ble_ancs_c_t * p_ancs);


/**@brief Function for writing to the CCCD to notifications from the Apple Notification Service.
 *
 * @param[in]  p_ancs      Apple Notification structure. This structure will have to be supplied by
 *                         the application. It identifies the particular client instance to use.
 *
 * @return     NRF_SUCCESS on successful writing of the CCCD, otherwise an error code.
 */
uint32_t ble_ancs_c_disable_notif_notification_source(const ble_ancs_c_t * p_ancs);


/**@brief Function for writing to the CCCD to disable data source notifications from the Apple Notification Service.
 *
 * @param[in]  p_ancs      Apple Notification structure. This structure will have to be supplied by
 *                         the application. It identifies the particular client instance to use.
 *
 * @return     NRF_SUCCESS on successful writing of the CCCD, otherwise an error code.
 */
uint32_t ble_ancs_c_disable_notif_data_source(const ble_ancs_c_t * p_ancs);

/**@brief Function to send a Get Notification Attributes command to the ANCS control point.
 *
 * @param[in]  p_ancs      Apple Notification structure. This structure will have to be supplied by
 *                         the application. It identifies the particular client instance to use.
 *
 * @param[in]  p_uid       UID of the notification.
 *
 * @param[in]  num_attr    Number of attributes.
 *
 * @param[in]  p_attr      Attribute list.
 *
 * @return     NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_ancs_get_notification_attributes(const ble_ancs_c_t * p_ancs, uint8_t * p_uid,
                                              uint8_t num_attr, ble_ancs_attr_list_t *p_attr);

#endif // BLE_ANCS_C_H__

/** @} */

