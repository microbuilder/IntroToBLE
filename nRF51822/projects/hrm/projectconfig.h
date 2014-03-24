/**************************************************************************/
/*!
    @file     projectconfig.h
*/
/**************************************************************************/
#ifndef _PROJECTCONFIG_H_
#define _PROJECTCONFIG_H_

/*=========================================================================
    MCU & BOARD SELCTION

    CFG_BOARD is one of the value defined in board.h
    -----------------------------------------------------------------------*/
    #ifndef CFG_BOARD // CFG_BOARD can be overwrite by compiler option
      #define CFG_BOARD                                CFG_BOARD_PCA10001
    #endif

    #define CFG_MCU_STRING                             "nRF51822"
/*=========================================================================*/


/*=========================================================================
    CODE BASE VERSION SETTINGS

    Please do not modify this version number.  To set a version number
    for your project or firmware, change the values in your 'boards/'
    config file.
    -----------------------------------------------------------------------*/
    #define CFG_CODEBASE_VERSION_MAJOR                 0
    #define CFG_CODEBASE_VERSION_MINOR                 1
    #define CFG_CODEBASE_VERSION_REVISION              0
/*=========================================================================*/


/*=========================================================================
    FIRMWARE VERSION SETTINGS
    -----------------------------------------------------------------------*/
    #define CFG_FIRMWARE_VERSION_MAJOR                 0
    #define CFG_FIRMWARE_VERSION_MINOR                 0
    #define CFG_FIRMWARE_VERSION_REVISION              0
/*=========================================================================*/


/*=========================================================================
    DEBUG LEVEL
    -----------------------------------------------------------------------

    CFG_DEBUG                 Level 3: Full debug output, any failed assert
                                       will produce a breakpoint for the
                                       debugger
                              Level 2: ATTR_ALWAYS_INLINE is null, ASSERT
                                       has text
                              Level 1: ATTR_ALWAYS_INLINE is an attribute,
                                       ASSERT has no text
                              Level 0: No debug information generated

    -----------------------------------------------------------------------*/
    #define CFG_DEBUG                                  (2)

    #if (CFG_DEBUG > 3) || (CFG_DEBUG < 0)
      #error "CFG_DEBUG must be a value between 0 (no debug) and 3"
    #endif
/*=========================================================================*/


/*=========================================================================
    PRINTF REDIRECTION
    -----------------------------------------------------------------------

    CFG_PRINTF_NEWLINE        Should be "\r\n" (Windows style) or "\n"
                              (*nix style)
    CFG_PRINTF_UART           Redirects printf output to UART

    -----------------------------------------------------------------------*/
    #define CFG_PRINTF_NEWLINE                         "\r\n"
    #define CFG_PRINTF_UART
    #define CFG_PRINTF_MAXSTRINGSIZE                    256
/*=========================================================================*/


/*=========================================================================
    UART
    -----------------------------------------------------------------------

    CFG_UART_BAUDRATE         The default UART speed.  This value is used
                              when initialising UART, and should be a
                              standard value like 57600, 9600, etc.
                              NOTE: This value may be overridden if
                              another value is stored in EEPROM!
    CFG_UART_BUFSIZE          The length in bytes of the UART RX FIFO. This
                              will determine the maximum number of received
                              characters to store in memory.
    CFG_UART_ECHO             If this is set to 1, any characters coming
                              in via UART will be echo'ed on the output
                              channel. Set to 0 to disable echo.

    -----------------------------------------------------------------------*/
    #define CFG_UART_BAUDRATE                          (9600)
    #define CFG_UART_BUFSIZE                           (256)
    #define CFG_UART_ECHO                              (1)
/*=========================================================================*/


/*=========================================================================
    GENERAL NRF51 PERIPHERAL SETTINGS
    -----------------------------------------------------------------------

    CFG_SCHEDULER_ENABLE      Set this to 'true' or 'false' depending on
                              if you use the event scheduler or not

    -----------------------------------------------------------------------*/
    #define CFG_SCHEDULER_ENABLE                       false

    /*------------------------------- GPIOTE ------------------------------*/
    #define CFG_GPIOTE_MAX_USERS                       1                        /**< Maximum number of users of the GPIOTE handler. */

    /*-------------------------------- TIMER ------------------------------*/
    #define CFG_TIMER_PRESCALER                        0                        /**< Value of the RTC1 PRESCALER register. freq = (32768/(PRESCALER+1)) */
    #define CFG_TIMER_MAX_INSTANCE                     8                        /**< Maximum number of simultaneously created timers. */
    #define CFG_TIMER_OPERATION_QUEUE_SIZE             5                        /**< Size of timer operation queues. */
/*=========================================================================*/


/*=========================================================================
    BTLE SETTINGS
    -----------------------------------------------------------------------*/

    #define CFG_BLE_TX_POWER_LEVEL                     4                        /**< in dBm (Valid values are -40, -20, -16, -12, -8, -4, 0, 4) */

    /*---------------------------- BOND MANAGER ---------------------------*/
    #define CFG_BLE_BOND_FLASH_PAGE_BOND               (BLE_FLASH_PAGE_END-1)   /**< Flash page used for bond manager bonding information.*/
    #define CFG_BLE_BOND_FLASH_PAGE_SYS_ATTR           (BLE_FLASH_PAGE_END-3)   /**< Flash page used for bond manager system attribute information. TODO check if we can use BLE_FLASH_PAGE_END-2*/
    #define CFG_BLE_BOND_DELETE_BUTTON_NUM             0

    /*--------------------------------- GAP -------------------------------*/
    #define CFG_GAP_APPEARANCE                         BLE_APPEARANCE_GENERIC_TAG
    #define CFG_GAP_LOCAL_NAME                         "HRM"

    #define CFG_GAP_CONNECTION_MIN_INTERVAL_MS         500                      /**< Minimum acceptable connection interval */
    #define CFG_GAP_CONNECTION_MAX_INTERVAL_MS         1000                     /**< Maximum acceptable connection interval */
    #define CFG_GAP_CONNECTION_SUPERVISION_TIMEOUT_MS  4000                     /**< Connection supervisory timeout */
    #define CFG_GAP_CONNECTION_SLAVE_LATENCY           0                        /**< Slave Latency in number of connection events. */

    #define CFG_GAP_ADV_INTERVAL_MS                    25                       /**< The advertising interval in miliseconds, should be multiply of 0.625 */
    #define CFG_GAP_ADV_TIMEOUT_S                      180                      /**< The advertising timeout in units of seconds. */

    /*--------------------- DEVICE INFORMATION SERVICE --------------------*/
    #define CFG_BLE_DEVICE_INFORMATION                 0
    #define CFG_BLE_DEVICE_INFORMATION_NAME            "Bluetooth LE code base"
    #define CFG_BLE_DEVICE_INFORMATION_MANUFACTURER    "microBuilder.eu"        /**< Manufacturer. Will be passed to Device Information Service. */
    #define CFG_BLE_DEVICE_INFORMATION_MODEL_NUMBER    BOARD_NAME
    #define CFG_BLE_DEVICE_INFORMATION_HARDWARE_REV    "A"

    /*-------------------------- BATTERY SERVICE --------------------------*/
    #define CFG_BLE_BATTERY                            0

    /*------------------------- HEART RATE ------------------------*/
    #define CFG_BLE_HEART_RATE                         1

    /*-------------------------- PROXIMITY ------------------------*/
    #define CFG_BLE_IMMEDIATE_ALERT                    0
    #define CFG_BLE_TX_POWER                           0
    #define CFG_BLE_LINK_LOSS                          0

    /*------------------------ CUSTOM UART SERVICE ------------------------*/
    #define CFG_BLE_UART                               0
    #define CFG_BLE_UART_BRIDGE                        0
    #define CFG_BLE_UART_UUID_BASE                     "\x6E\x40\x00\x00\xB5\xA3\xF3\x93\xE0\xA9\xE5\x0E\x24\xDC\xCA\x9E"
/*=========================================================================*/


/*=========================================================================
    CONFIG FILE VALIDATION
    -----------------------------------------------------------------------*/
    #if CFG_BLE_TX_POWER_LEVEL != -40 && CFG_BLE_TX_POWER_LEVEL != -20 && CFG_BLE_TX_POWER_LEVEL != -16 && CFG_BLE_TX_POWER_LEVEL != -12 && CFG_BLE_TX_POWER_LEVEL != -8  && CFG_BLE_TX_POWER_LEVEL != -4  && CFG_BLE_TX_POWER_LEVEL != 0   && CFG_BLE_TX_POWER_LEVEL != 4
        #error "CFG_BLE_TX_POWER_LEVEL must be -40, -20, -16, -12, -8, -4, 0 or 4"
    #endif    
    
    #if CFG_BLE_IBEACON
      #if CFG_PROTOCOL
        /* ToDo: Refactor later to enable CFG_PROTOCOL to control iBeacon */
        #error "CFG_BLE_IBEACON can not be used with other services (CFG_PROTOCOL)"
      #endif
    #endif
/*=========================================================================*/

#endif /* _PROJECTCONFIG_H_ */
