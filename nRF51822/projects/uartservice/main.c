/**************************************************************************/
/*!
    @file     main.c
*/
/**************************************************************************/
#include "projectconfig.h"
#include "common/common.h"
#include "boards/board.h"
#include "btle.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"

/* ---------------------------------------------------------------------- */
/* HEART RATE SERVICE                                                      */
/* ---------------------------------------------------------------------- */
/*------------- Measurement Characteristic -------------*/
btle_characteristic_t hrm_char_measure =
{
    .uuid       = BLE_UUID_HEART_RATE_MEASUREMENT_CHAR,
    .properties = { .notify = 1 },
    .len_min    = 2,
    .len_max    = 2,
    .init_value = NULL
};

/*------------- Body Location Characteristic -------------*/
btle_characteristic_t hrm_char_body_location =
{
    .uuid       = BLE_UUID_BODY_SENSOR_LOCATION_CHAR,
    .properties = { .read = 1 },
    .len_min    = 1,
    .len_max    = 1,
    .init_value = (uint8_t []) { 3 }
};

btle_service_t btle_service[] =
{
    {
        .uuid       = BLE_UUID_HEART_RATE_SERVICE,
        .char_count = 2, // TODO remove
        .char_pool  = { &hrm_char_measure, &hrm_char_body_location }
    }
};

enum {
  SERVICE_COUNT = sizeof(btle_service) / sizeof(btle_service_t)
};

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
    case 0: break;
    case 1: break;
    default: break;
  }
}

/**************************************************************************/
/*!
    @brief  This callback fires every time an 'indicate' passes or fails
            in the UART service
*/
/**************************************************************************/
void uart_service_indicate_callback(bool is_succeeded)
{
  if ( is_succeeded )
  {
    printf("confirmation received\n");
  }
  else
  {
    printf("confirmation timeout\n");
  }
}

// periodically invoked by timer
void heart_rate_measurement_task(void * p_context)
{
  // https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.heart_rate_measurement.xml
  static struct {
    uint8_t flag;
    uint8_t value;
  }measurement = { 0, 70 };

  // get timer tick for random changes from -1 0 +1
  uint32_t tick;
  app_timer_cnt_get(&tick);

  measurement.value += (tick%3) - 1;

  btle_characteristic_update(&hrm_char_measure, &measurement, 2);
}

/**************************************************************************/
/*!
    @brief  Main application entry point
*/
/**************************************************************************/
int main(void)
{ 
  app_timer_id_t blinky_timer_id, hrm_timer_id;
  
  /* Initialize the target HW */
  boardInit();
  
  /* Initialise BLE and start advertising as an iBeacon */
  btle_init(btle_service, SERVICE_COUNT);

  /* Initialise a 1 second blinky timer to show that we're alive */
  ASSERT_STATUS ( app_timer_create(&blinky_timer_id, APP_TIMER_MODE_REPEATED, blinky_handler) );
  ASSERT_STATUS ( app_timer_start (blinky_timer_id, APP_TIMER_TICKS(1000, CFG_TIMER_PRESCALER), NULL) );

  ASSERT_STATUS ( app_timer_create(&hrm_timer_id, APP_TIMER_MODE_REPEATED, heart_rate_measurement_task) );
  ASSERT_STATUS ( app_timer_start (hrm_timer_id, APP_TIMER_TICKS(1000, CFG_TIMER_PRESCALER), NULL) );

  ASSERT_STATUS( app_button_enable() );

  while(true)
  {
  }
}
