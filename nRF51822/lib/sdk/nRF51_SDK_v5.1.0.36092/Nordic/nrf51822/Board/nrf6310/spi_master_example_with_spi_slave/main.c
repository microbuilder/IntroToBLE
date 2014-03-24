/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
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

/**@file
 * @defgroup spi_master_example_main main.c
 * @{
 * @ingroup spi_master_example
 *
 * @brief SPI master example application to be used with the SPI slave example application.
 */

#include <stdio.h>
#include "spi_master.h"
#include "spi_master_config.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"

#define TX_RX_BUF_LENGTH 1u     /**< SPI transaction buffer length. */
#define DELAY_MS         100u   /**< Timer delay in milliseconds. */


/**@brief Function for executing a single SPI transaction.
 *
 * @retval true  Operation success.
 * @retval false Operation failure. 
 */
static bool test_spi_tx_rx(void)
{    
    uint8_t tx_data[TX_RX_BUF_LENGTH] = {0}; 
    uint8_t rx_data[TX_RX_BUF_LENGTH] = {0};  

    uint32_t * p_spi_base_address = spi_master_init(SPI0, SPI_MODE0, true);
    if (p_spi_base_address == NULL)
    {
        return false;
    }
      
    return spi_master_tx_rx(p_spi_base_address, TX_RX_BUF_LENGTH, tx_data, rx_data);    
}


/**@brief Function for application main entry. Does not return.
 */ 
int main(void)
{
    bool is_test_success;
    
    // Configure all LEDs as outputs. 
    nrf_gpio_range_cfg_output(LED_START, LED_STOP);
        
    // Set LED_0 high to indicate that the application is running. 
    nrf_gpio_pin_set(LED_0);    
    
    for (;;)
    {
        is_test_success = test_spi_tx_rx();
        if (!is_test_success)        
        {
            // Set LED2 high to indicate that error has occurred. 
            nrf_gpio_pin_set(LED_2);            
            for (;;)
            {
                // No implementation needed.           
            }
        }
        
        nrf_gpio_pin_toggle(LED_1);
        nrf_delay_ms(DELAY_MS);   
    }
}

/** @} */
