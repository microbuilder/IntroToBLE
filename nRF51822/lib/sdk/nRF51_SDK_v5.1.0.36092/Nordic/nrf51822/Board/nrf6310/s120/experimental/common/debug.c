/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
 
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "app_uart.h"
#include "simple_uart.h"
#include "app_gpiote.h"
#include "nordic_common.h"

#include "boards.h"

#ifdef ENABLE_DEBUG_LOG_SUPPORT

#if 0
void uart_evt_handler(app_uart_evt_t * p_app_uart_event)
{
    if (p_app_uart_event->evt_type != APP_UART_TX_EMPTY)
    {
        //nrf_gpio_pin_toggle(14);
    }
}

uint32_t debug_init(void)
{
	uint32_t err_code;
    
    nrf_gpio_cfg_output(14);
	
    app_uart_comm_params_t params;
    params.baud_rate = UART_BAUDRATE_BAUDRATE_Baud38400;
    params.flow_control = APP_UART_FLOW_CONTROL_DISABLED;
    params.tx_pin_no = 17;
    params.rx_pin_no = 16;
    params.use_parity = false;
    
    APP_GPIOTE_INIT(1);
    
    //app_gpiote_user_enable()
    
    APP_UART_INIT(&params, uart_evt_handler, APP_IRQ_PRIORITY_HIGH, err_code);
    return err_code;
}

int fputc(int ch, FILE * p_file) 
{
    const uint32_t return_value = app_uart_put((uint8_t)ch);
    UNUSED_VARIABLE(return_value);
    return ch;
}

uint32_t debug_log(const char * p_file,
                   unsigned int  line_number,
                   char          * format,
                   ...)
{
    char buffer[256];
    uint16_t buffer_size;
    
    va_list args;
    va_start (args, format);
    buffer_size = strlen(p_file);
    buffer_size += 4;
    
    sprintf(buffer,"[%s]:[%d]:",p_file,line_number);
    sprintf(buffer+buffer_size, format, args);
    
    printf ("%s\r\n",buffer);
    
    va_end (args);

    return NRF_SUCCESS;    
}
#else
void debug_init(void)
{
	simple_uart_config(RTS_PIN_NUMBER, TX_PIN_NUMBER, CTS_PIN_NUMBER, RX_PIN_NUMBER, HWFC);
}

int fputc(int ch, FILE * p_file) 
{
    simple_uart_put((uint8_t)ch);
    return ch;
}
#endif 

#endif // ENABLE_DEBUG_LOG_SUPPORT

/**
 *@}
 **/
 
