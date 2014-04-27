/**************************************************************************/
/*!
    @file     main.c
*/
/**************************************************************************/
#include "common.h"
#include "board.h"
#include "btle.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"

#define CFG_LED_CONNECTION  0
#define CFG_LED_ALERT       1
#define CFG_LED_LINK_LOSS   1

/**************************************************************************/
/*!
    @brief  Handler for the 1s blinky timer declared in main()
*/
/**************************************************************************/
static void blinky_handler(void * p_context)
{
  static bool led_on = false;
  led_on = !led_on;

  boardLED(led_on ? BIT(CFG_LED_CONNECTION) : 0,
           led_on ? 0 : BIT(CFG_LED_CONNECTION) );
}

/**************************************************************************/
/*!
    @brief  This callback fires every time a valid button press occurs
*/
/**************************************************************************/
void boardButtonCallback(uint8_t button_num)
{
  switch (button_num)
  {
    case 0: 
      break;
    case 1: 
      break;
    default: 
      break;
  }
}

/**************************************************************************/
/*!
    @brief  Main application entry point
*/
/**************************************************************************/
int main(void)
{ 
  app_timer_id_t blinky_timer_id, uart_timer_id;
  
  /* Initialize the target HW */
  boardInit();
  
  /* Initialise BLE and start advertising as an iBeacon */
  btle_init();

  /* Initialise a 1 second blinky timer to show that we're alive */
  ASSERT_STATUS ( app_timer_create(&blinky_timer_id, APP_TIMER_MODE_REPEATED, blinky_handler) );
  ASSERT_STATUS ( app_timer_start (blinky_timer_id, APP_TIMER_TICKS(1000, CFG_TIMER_PRESCALER), NULL) );

  ASSERT_STATUS ( app_timer_create(&uart_timer_id, APP_TIMER_MODE_REPEATED, uart_service_bridge_task) );
  ASSERT_STATUS ( app_timer_start (uart_timer_id, APP_TIMER_TICKS(100, CFG_TIMER_PRESCALER), NULL) );

  while(true)
  {
  }
}
