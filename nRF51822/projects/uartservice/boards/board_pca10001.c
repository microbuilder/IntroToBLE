/**************************************************************************/
/*!
    @file     board_pca10001.c
*/
/**************************************************************************/
#include "board.h"

#if CFG_BOARD == CFG_BOARD_PCA10001

const static uint8_t led_gpio [BOARD_LED_NUM] = { BOARD_LED_PIN_ARRAY };
const static uint8_t button_gpio [BOARD_BUTTON_NUM] = { BOARD_BUTTON_PIN_ARRAY };


static inline uint32_t led_mask_to_gpio(uint8_t led_mask) ATTR_ALWAYS_INLINE ATTR_PURE;
static inline uint8_t button_gpio_to_number(uint8_t pin_no) ATTR_ALWAYS_INLINE ATTR_PURE;
static inline uint32_t get_baudrate(uint32_t decimal_value) ATTR_ALWAYS_INLINE ATTR_CONST;


/**************************************************************************/
/*!
    @brief      Enables or disables the on board LEDs
    
    @param[in]  mask_on   Bit mask indicating which pins to turn on
    @param[in]  mask_off  Bit mask indicating which pins to turn off
    
    @note       If a pin is set to both on and off in the two mask values,
                on will take priority over off
*/
/**************************************************************************/
void boardLED(uint8_t mask_on, uint8_t mask_off)
{
  NRF_GPIO->OUTCLR = led_mask_to_gpio(mask_off);
  NRF_GPIO->OUTSET = led_mask_to_gpio(mask_on ); // mask_on take over priority
}


/**************************************************************************/
/*!
    @brief  This function captures any successful button events, and then
            redirects them to the higher level callback so that they can
            be handled in application code.
*/
/**************************************************************************/
static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
  (void) button_action;
  boardButtonCallback ( button_gpio_to_number(pin_no) );
}


/**************************************************************************/
/*!
    @brief Get current state of the specific button
*/
/**************************************************************************/
bool boardButtonCheck(uint8_t button_num)
{
  bool is_pushed;

  ASSERT_INT( ERROR_NONE, app_button_is_pushed(button_gpio[button_num], &is_pushed), false );

  return is_pushed;
}

/**************************************************************************/
/*!
    @brief  Helper function that handles UART events (errors, etc.)
*/
/**************************************************************************/
void board_uart_event_handler(app_uart_evt_t* p_event)
{
  ASSERT(p_event->evt_type != APP_UART_FIFO_ERROR &&
         p_event->evt_type != APP_UART_COMMUNICATION_ERROR , (void) 0);
}


/**************************************************************************/
/*!

*/
/**************************************************************************/
void boardInit(void)
{
  uint32_t error;
  static app_button_cfg_t button_cfg[BOARD_BUTTON_NUM];

  /* Configure and enable the timer app */
	APP_TIMER_INIT(CFG_TIMER_PRESCALER, CFG_TIMER_MAX_INSTANCE, CFG_TIMER_OPERATION_QUEUE_SIZE, CFG_SCHEDULER_ENABLE);

  /* Initialise GPIOTE */
  APP_GPIOTE_INIT(CFG_GPIOTE_MAX_USERS);

  /* LED Settings */
  for(uint8_t i=0; i<BOARD_LED_NUM; i++)
  {
    nrf_gpio_cfg_output( led_gpio[i] );
  }

  /* Button Settings */
  for(uint8_t i=0; i<BOARD_BUTTON_NUM; i++)
  {
    /* Pins are configued with pullups since none are present on the PCB */
    button_cfg[i] = (app_button_cfg_t)
                    {
                      .pin_no = button_gpio[i],
                      .active_state = BOARD_BUTTON_ACTIVE_STATE ? true : false,
                      .pull_cfg = NRF_GPIO_PIN_PULLUP,
                      .button_handler = button_event_handler
                    };
  };

  APP_BUTTON_INIT(button_cfg, BOARD_BUTTON_NUM,
                  APP_TIMER_TICKS(BOARD_BUTTON_DETECTION_INTERVAL_MS, CFG_TIMER_PRESCALER), false);

  /* UART Settings */
  app_uart_comm_params_t para_uart =
  {
    .rx_pin_no    = BOARD_UART_RXD_PIN,
    .tx_pin_no    = BOARD_UART_TXD_PIN,
    .rts_pin_no   = BOARD_UART_RTS_PIN,
    .cts_pin_no   = BOARD_UART_CTS_PIN,
    .flow_control = APP_UART_FLOW_CONTROL_ENABLED,
    .use_parity   = false,
    .baud_rate    = get_baudrate(CFG_UART_BAUDRATE)
  };

  /* Initialise the UART FIFO */
  APP_UART_FIFO_INIT( &para_uart, CFG_UART_BUFSIZE, CFG_UART_BUFSIZE, board_uart_event_handler, APP_IRQ_PRIORITY_LOW, error);
  ASSERT_STATUS_RET_VOID( (error_t) error );
}

/* ---------------------------------------------------------------------- */
/* HELPER FUNCTIONS                                                       */
/* ---------------------------------------------------------------------- */

/**************************************************************************/
/*!
    @brief  Helper function to convert baud rates to a proper enum value
*/
/**************************************************************************/
static inline uint32_t get_baudrate(uint32_t decimal_value)
{
  switch(decimal_value)
  {
    case 1200    : return UART_BAUDRATE_BAUDRATE_Baud1200   ;
    case 2400    : return UART_BAUDRATE_BAUDRATE_Baud2400   ;
    case 4800    : return UART_BAUDRATE_BAUDRATE_Baud4800   ;
    case 9600    : return UART_BAUDRATE_BAUDRATE_Baud9600   ;
    case 14400   : return UART_BAUDRATE_BAUDRATE_Baud14400  ;
    case 19200   : return UART_BAUDRATE_BAUDRATE_Baud19200  ;
    case 28800   : return UART_BAUDRATE_BAUDRATE_Baud28800  ;
    case 38400   : return UART_BAUDRATE_BAUDRATE_Baud38400  ;
    case 57600   : return UART_BAUDRATE_BAUDRATE_Baud57600  ;
    case 76800   : return UART_BAUDRATE_BAUDRATE_Baud76800  ;
    case 115200  : return UART_BAUDRATE_BAUDRATE_Baud115200 ;
    case 230400  : return UART_BAUDRATE_BAUDRATE_Baud230400 ;
    case 250000  : return UART_BAUDRATE_BAUDRATE_Baud250000 ;
    case 460800  : return UART_BAUDRATE_BAUDRATE_Baud460800 ;
    case 921600  : return UART_BAUDRATE_BAUDRATE_Baud921600 ;
    case 1000000 : return UART_BAUDRATE_BAUDRATE_Baud1M     ;
  }

  ASSERT(false, 0);
  return 0;
}

/**************************************************************************/
/*!
    @brief  Correlates a pin and button numbers
*/
/**************************************************************************/
static inline uint8_t button_gpio_to_number(uint8_t pin_no)
{
  for(uint8_t i=0; i<BOARD_BUTTON_NUM; i++)
  {
    if ( button_gpio[i] == pin_no ) return i;
  }

  return 0;
}

/**************************************************************************/
/*!
    @brief  Helper function that enables or disables the LED pins
*/
/**************************************************************************/
static inline uint32_t led_mask_to_gpio(uint8_t led_mask)
{
  uint32_t result = 0;
  for(uint32_t i=0; i<BOARD_LED_NUM; i++)
  {
    if ( BIT_TEST(led_mask,i) )
    {
      result = BIT_SET(result, led_gpio[i]);
    }
  }

  return result;
}

#endif
