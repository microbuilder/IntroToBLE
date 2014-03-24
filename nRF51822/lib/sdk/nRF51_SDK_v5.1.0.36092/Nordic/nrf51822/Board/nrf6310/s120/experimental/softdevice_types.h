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

 /** @cond To make doxygen skip this file */

/** @file
 *
 * @defgroup sd_type_definitions SoftDevice defines
 * @{
 * @ingroup app_common
 * @brief   Defines needed to work with SoftDevices.
 *
 * @details This module defines the various preprocessor defines needed by the applications to
 *          be able to work with the various available SoftDevices.
 */

#ifndef SOFTDEVICE_TYPES_H__
#define SOFTDEVICE_TYPES_H__

// Check if one of the SoftDevice preprocessor defines are defined during compilation.
#if defined (S110_SUPPORT_REQD)

// S110 SoftDevice supports only BLE functionality.

#define BLE_STACK_SUPPORT_REQD 

#elif defined (S120_SUPPORT_REQD)

// S120 SoftDevice supports only BLE functionality.

#define BLE_STACK_SUPPORT_REQD 

#elif defined (S210_SUPPORT_REQD)

// S210 SoftDevice supports only ANT functionality.

#define ANT_STACK_SUPPORT_REQD

#elif defined (S310_SUPPORT_REQD)

// S310 SoftDevice supports both BLE and ANT functionalities.
#define BLE_STACK_SUPPORT_REQD
#define ANT_STACK_SUPPORT_REQD

#else

#warning "No SoftDevice type defined. Define atleast one of S110_SUPPORT_REQD, S120_SUPPORT_REQD, S210_SUPPORT_REQD, and S310_SUPPORT_REQD to enable the definitions of SoftDevice specific types."

#endif

#endif // SOFTDEVICE_TYPES_H__


