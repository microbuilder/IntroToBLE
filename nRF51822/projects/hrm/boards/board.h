/**************************************************************************/
/*!
    @file     board.h
    @brief    Mandatory functions for the board/HW abstraction layer

    @details

    Each board that uses this code base is required to implement the
    functions defined in this header.  This allows the core board-specific
    functions to be abstracted away during startup, and when entering the
    sleep modes, without having to be aware of what the target HW is.
*/
/**************************************************************************/
#ifndef __BOARD_H__
#define __BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#include "nrf_gpio.h"
#include "app_timer.h"
#include "app_gpiote.h"
#include "app_uart.h"
#include "app_button.h"
#include "twi_master.h"

/**************************************************************************/
/*!
    @brief Supported Boards
*/
/**************************************************************************/
#define CFG_BOARD_PCA10000  1
#define CFG_BOARD_PCA10001  2

#if CFG_BOARD == 0
  #error CFG_BOARD is not defined or supported yet
#elif CFG_BOARD == CFG_BOARD_PCA10000
  #include "boards/board_pca10000.h"
#elif CFG_BOARD == CFG_BOARD_PCA10001
  #include "boards/board_pca10001.h"
#else
  #error CFG_BOARD is not defined or supported yet
#endif

// Any board support file needs to implement these common methods, which
// allow the low-level board-specific details to be abstracted away from
// the higher level stacks and shared peripheral code

/**************************************************************************/
/*!
    @brief Initialises the HW and configures the board for normal operation
*/
/**************************************************************************/
void boardInit(void);

/**************************************************************************/
/*!
    @brief Turns the LED(s) on or off
*/
/**************************************************************************/
void boardLED(uint8_t mask_on, uint8_t mask_off);

static inline void boardLED_On(uint8_t mask_on) ATTR_ALWAYS_INLINE;
static inline void boardLED_On(uint8_t mask_on)
{
  boardLED(mask_on, 0);
}

static inline void boardLED_Off(uint8_t mask_off) ATTR_ALWAYS_INLINE;
static inline void boardLED_Off(uint8_t mask_off)
{
  boardLED(0, mask_off);
}

/**************************************************************************/
/*!
    @brief Get current state of Buttons
*/
/**************************************************************************/
bool boardButtonCheck(uint8_t button_num);

// implement in application, this function invoked whenever a button is press
// button_num is range from 0 to BOARD_BUTTON_NUM
void boardButtonCallback(uint8_t button_num);

/**************************************************************************/
/*!
    @brief Configure the board for low power and enter sleep mode
*/
/**************************************************************************/
void boardSleep(void);

/**************************************************************************/
/*!
    @brief  Restores parts and system peripherals to an appropriate
            state after waking up from sleep mode
*/
/**************************************************************************/
void boardWakeup(void);

#ifdef __cplusplus
}
#endif

#endif
