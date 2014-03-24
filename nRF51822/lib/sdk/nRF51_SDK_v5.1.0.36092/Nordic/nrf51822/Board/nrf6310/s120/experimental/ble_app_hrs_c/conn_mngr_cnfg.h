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
 * @cond
 * @defgroup cnxn_mngr_cnfg Connection Manager Configuration 
 * @ingroup cnxn_mngr
 * @{
 *
 * @brief Defines application specific configuration for Connection Manager.
 *
 * @details All configurations that are specific to application have been defined
 *          here. Application should configuration that best suits its requirements.
 */
 
#ifndef CONN_MNGR_CNFG_H__
#define CONN_MNGR_CNFG_H__

/**
 * @defgroup cnxn_mngr_inst Connection Manager Instances
 * @{
 */
/**
 * @brief Maximum applications that connection manager can support.
 *
 * @details Maximum application that the connection manager can support.
 *          Currently only one application can be supported.
 *          Minimum value : 1
 *          Maximum value : 1
 *          Dependencies  : None.
 */
#define CONN_MNGR_MAX_APPLICATIONS  1

/**
 * @brief Maximum connections that connection manager should simultaneously manage.
 *
 * @details Maximum connections that connection manager should simultaneously manage.
 *          Minimum value : 1
 *          Maximum value : Maximum links supported by SoftDevice.
 *          Dependencies  : None.
 */
#define CONN_MNGR_MAX_CONNECTIONS   1
/* @} */


/**
 *
 * @defgroup cnxn_mngr_scan_policy Connection Manager Scan and Connect Policy
 * @{
 */
/**
 * @brief Defines policy initiate to connect with a peripheral device.
 *
 * @details This defines the connection criteria to be used to connect to a device.
 *          The value should be set to one of:
 *          - \ref CONN_MNGR_CONNECT_POLICY_RSSI 
 *          - \ref CONN_MNGR_CONNECT_POLICY_UUID16 
 *          - \ref CONN_MNGR_CONNECT_POLICY_DEV_NAME 
 *          - \ref CONN_MNGR_CONNECT_POLICY_DEV_ADDR 
 */
#define CONN_MNGR_CONNECT_POLICY      CONN_MNGR_CONNECT_POLICY_UUID16


/**
 * @brief Defines the target UUID that module should look for in Adv Report.
 *
 * @details  This defines the target UUID that the application should look for in the
 *           advertising report received from a peripheral device.
 *
 *           Dependencies  : \ref CONN_MNGR_CONNECT_POLICY should be defined to
 *           \ref CONN_MNGR_CONNECT_POLICY_UUID16 Else this definition is unused.
 *
 * @note This should be unsigned 16-bit value. 32-bit and 128-bit are not
 *       currently supported.
 */
#define CONN_MNGR_TARGET_UUID          0x180D


/**
 * @brief Defines Target Name of peripheral device that the module should look for in
 *        Adv Report.
 *
 * @details This defines the device name that peripheral device uses in its advertising
 *          report. 
 *          Minimum length : 1
 *          Maximum length : 29 (Maximum that can be fit in Adv report).
 *          Dependencies   : \ref CONN_MNGR_CONNECT_POLICY should be defined to
 *          \ref CONN_MNGR_CONNECT_POLICY_DEV_NAME Else this definition is unused.
 *
 * @note Module performs a memory comparison of the name set here with the one received.
 *       Hence this could be a set a short or partial name.
 */
#define CONN_MNGR_TARGET_DEV_NAME      "Nordic_HRM"

/**
 * @brief Defines Target Address Type of peripheral device that the module should look for in
 *        Adv Report.
 *
 * @details This defines the device address type of target peripheral device. This should be set
 *          to one of the gap address types defined in \ref BLE_GAP_ADDR_TYPES.
 *
 *          Dependencies   : \ref CONN_MNGR_CONNECT_POLICY should be defined to
 *          \ref CONN_MNGR_CONNECT_POLICY_DEV_ADDR Else this definition is unused.
 */
#define CONN_MNGR_TARGET_ADDR_TYPE     BLE_GAP_ADDR_TYPE_PUBLIC


/**
 * @brief Defines Target Address of peripheral device that the module should look for in
 *        Adv Report.
 *
 * @details This defines the device address of peripheral device uses in its advertising
 *          report. This is a 6 octet value.
 *          Minimum        : 6 octets.
 *          Maximum        : 6 octets.
 *          Dependencies   : \ref CONN_MNGR_CONNECT_POLICY should be defined to
 *          \ref CONN_MNGR_CONNECT_POLICY_DEV_ADDR Else this definition is unused.
 */
#define CONN_MNGR_TARGET_ADDR          {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}


/**
 * @brief Defines RSSI value to connect to a device that is within a certain range.
 *
 * @details This defines the minimum RSSI for the peripheral device to be considered
 *          in connectable proximity for the central. This value is unsigned 8 bit
 *          value.
 *          Minimum length : -40
 *          Maximum length : -127
 *
 * @note Currently this is being used by default and does not depend on \ref CONN_MNGR_CONNECT_POLICY
 *       being set to \ref CONN_MNGR_CONNECT_POLICY_RSSI.
 */
#define CONN_MNGR_TARGET_RSSI           -80


/**
 * @brief Define this to 1 if connection on receiving advertising report from target peripheral.
 *
 * @details Define this value to 1 in case connection to peripheral device that matches 
 *          scan or connection policy criteria is met.
 *          Possible values: 0 or 1.
 *
 * @note Defining this to 0 will result in receiving advertising report indication from matching
 *       peripheral and connection will have to be initiated by the application.
 */
#define AUTO_CONNECT_ON_MATCH               1

/* @} */


/**
 * @defgroup cnxn_mngr_scan_connect_param Connection Manager Scan and Connect Parameters
 * @{
 */
/**
 * @defgroup cnxn_mngr_scan_param Scan Parameters
 * @{
 */ 
/**
 * @brief Determines scan interval in units of 0.625 millisecond.
 *
 * @details Determines scan interval in units of 0.625 millisecond. This is an unsigned 16-bit value.
 *          Minimum value  : 0x0004 (2.5ms)
 *          Maximum value  : 0x4000 (10.24ms)
 *          Dependencies   : None.
 */
#define CONN_MNGR_SCAN_INTERVAL             0x00A0
 
/**
 * @brief Determines scan window in units of 0.625 millisecond.
 *
 * @details Determines scan interval in units of 0.625 millisecond. This is an unsigned 16-bit value.
 *          Minimum value  : 0x0004 (2.5ms)
 *          Maximum value  : 0x4000 (10.24ms)
 *          Dependencies   : None.
 */
#define CONN_MNGR_SCAN_WINDOW                0x0050
/* @} */

/**
 * @defgroup cnxn_mngr_cnxn_param Connection Parameters
 * @{
 */
/**
 * @brief Determines minimum connection interval in millisecond.
 *
 * @details Determines minimum connection interval in units of 1.25 ms millisecond. 
 *          This is an unsigned 16-bit value.
 *          Minimum value  : 0x0006 (7.5 ms)
 *          Maximum value  : 0x0C80 (4 s)
 *          Dependencies   : None.
 */
#define CONN_MNGR_MIN_CONNECTION_INTERVAL    MSEC_TO_UNITS(7.5, UNIT_1_25_MS)
 
/**
 * @brief Determines maximum connection interval in millisecond.
 *
 * @details Determines maximum connection interval in units of 1.25 ms millisecond. 
 *          This is an unsigned 16-bit value.
 *          Minimum value  : 0x0006 (7.5 ms)
 *          Maximum value  : 0x0C80 (4 s)
 *          Dependencies   : None.
 */
#define CONN_MNGR_MAX_CONNECTION_INTERVAL     MSEC_TO_UNITS(30, UNIT_1_25_MS)
 
/**
 * @brief Determines slave latency in counts of connection events.
 *
 * @details Determines slave latency in counts of connection events.
 *          This is an unsigned 16-bit value.
 *          Minimum value  : 0x0000 (0)
 *          Maximum value  : 0x03E8 (1000)
 *          Dependencies   : None.
 */
#define CONN_MNGR_SLAVE_LATENCY                    0
 
/**
 * @brief Determines supervision time-out in units of 10 millisecond.
 *
 * @details Determines supervision time-out in units of 10 millisecond.
 *          This is an unsigned 16-bit value.
 *          Minimum value  : 0x000A (100 ms)
 *          Maximum value  : 0x0C80 (32 s)
 *          Dependencies   : None. But the value should be realistic taking into account
 *                           slave latency and connection interval.
 */
#define CONN_MNGR_SUPERVISION_TIMEOUT            4000
/* @} */
/* @} */
/* @} */
/** @endcond */
#endif // CONN_MNGR_CNFG_H__

