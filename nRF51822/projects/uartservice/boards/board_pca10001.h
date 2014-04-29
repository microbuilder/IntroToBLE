/**************************************************************************/
/*!
    @file     board_pca10001.h
*/
/**************************************************************************/
#ifndef _BOARD_PCA10001_H_
#define _BOARD_PCA10001_H_

#ifdef __cplusplus
 extern "C" {
#endif

#define BOARD_NAME                          "PCA10001"

/*=========================================================================
  LED SETTINGS
  -------------------------------------------------------------------------
  BOARD_LED_NUM         The number of LEDs present on the board
  BOARD_LED_PIN_ARRAY   Comma separated list of LED pins (ex. "3, 4")
  BOARD_LED_ON          The logic state to turn the LED on
  -------------------------------------------------------------------------*/
  #define BOARD_LED_NUM                       2
  #define BOARD_LED_PIN_ARRAY                 18, 19
  #define BOARD_LED_ON                        1
/*=========================================================================*/


/*=========================================================================
  BUTTONS
  -------------------------------------------------------------------------
  BOARD_BUTTON_NUM                The number of buttons present
  BOARD_BUTTON_PIN_ARRAY          Comma separated list of pins ("2, 3")
  BOARD_BUTTON_ACTIVE_STATE       Pin state when the button is pressed
  -------------------------------------------------------------------------*/
  #define BOARD_BUTTON_NUM                    2
  #define BOARD_BUTTON_PIN_ARRAY              16, 17
  #define BOARD_BUTTON_ACTIVE_STATE           0
  #define BOARD_BUTTON_DETECTION_INTERVAL_MS  50
/*=========================================================================*/


/*=========================================================================
  UART SETTINGS
  -------------------------------------------------------------------------
  BOARD_UART_JLINK      Set this to 1 to use the J-Link/CDC interface
                        on the nRF51822-EK board (PCA10001), otherwise
                        set it to 0 to use the external HW UART interface.
  BOARD_UART_RXD_PIN    Pin location for RXD
  BOARD_UART_TXD_PIN    Pin location for TXD
  BOARD_UART_CTS_PIN    Pin location for CTS
  BOARD_UART_RTS_PIN    Pin location for RTS
  -------------------------------------------------------------------------*/
  #define BOARD_UART_JLINK                    1

  #if BOARD_UART_JLINK
    #define BOARD_UART_RXD_PIN                11
    #define BOARD_UART_TXD_PIN                9
    #define BOARD_UART_CTS_PIN                10
    #define BOARD_UART_RTS_PIN                8
  #else
    #define BOARD_UART_RXD_PIN                22
    #define BOARD_UART_TXD_PIN                23
    #define BOARD_UART_CTS_PIN                21
    #define BOARD_UART_RTS_PIN                20
  #endif
/*=========================================================================*/


/*=========================================================================
  I2C SETTINGS
  -------------------------------------------------------------------------
  BOARD_I2C_SCL_PIN   Pin location for SCL
  BOARD_I2C_SDA_PIN   Pin location for SDA
  -------------------------------------------------------------------------*/
  #define BOARD_I2C_SCL_PIN                   12
  #define BOARD_I2C_SDA_PIN                   13
/*=========================================================================*/

#ifdef __cplusplus
 }
#endif

#endif /* _BOARD_PCA10001_H_ */
