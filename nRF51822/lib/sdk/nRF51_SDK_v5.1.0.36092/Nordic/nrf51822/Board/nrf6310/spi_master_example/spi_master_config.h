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
#ifndef SPI_MASTER_CONFIG_H
#define SPI_MASTER_CONFIG_H

#define SPI_OPERATING_FREQUENCY  ( 0x02000000UL << (uint32_t)Freq_1Mbps )  /*!< Slave clock frequency. */

/*  SPI0 */
#define SPI_PSELSCK0              31   /*!< GPIO pin number for SPI clock (note that setting this to 31 will only work for loopback purposes as it not connected to a pin) */
#define SPI_PSELMOSI0             20   /*!< GPIO pin number for Master Out Slave In    */
#define SPI_PSELMISO0             22   /*!< GPIO pin number for Master In Slave Out    */
#define SPI_PSELSS0               30   /*!< GPIO pin number for Slave Select           */

/*  SPI1 */
#define SPI_PSELSCK1              29   /*!< GPIO pin number for SPI clock              */
#define SPI_PSELMOSI1             21   /*!< GPIO pin number for Master Out Slave In    */
#define SPI_PSELMISO1             23   /*!< GPIO pin number for Master In Slave Out    */
#define SPI_PSELSS1               28   /*!< GPIO pin number for Slave Select           */

//#define DEBUG
#ifdef DEBUG
#define DEBUG_EVENT_READY_PIN0    10    /*!< when DEBUG is enabled, this GPIO pin is toggled everytime READY_EVENT is set for SPI0, no toggling means something has gone wrong */
#define DEBUG_EVENT_READY_PIN1    11    /*!< when DEBUG is enabled, this GPIO pin is toggled everytime READY_EVENT is set for SPI1, no toggling means something has gone wrong */
#endif

#define NUMBER_OF_TEST_BYTES     2    /*!< number of bytes to send to slave to test if Initialization was successful */
#define TEST_BYTE                0xBB /*!< Randomly chosen test byte to transmit to spi slave */
#define TIMEOUT_COUNTER          0x3000UL  /*!< timeout for getting rx bytes from slave */

/** @def  TX_RX_MSG_LENGTH
 * number of bytes to transmit and receive. This amount of bytes will also be tested to see that
 * the received bytes from slave are the same as the transmitted bytes from the master */
#define TX_RX_MSG_LENGTH   100

/** @def ERROR_PIN_SPI0
 * This pin is set active high when there is an error either in TX/RX for SPI0 or if the received bytes does not totally match the transmitted bytes.
 * This functionality can be tested by temporarily disconnecting the MISO pin while running this example.
 */
#define ERROR_PIN_SPI0   8UL

/** @def ERROR_PIN_SPI1
 * This pin is set active high when there is an error either in TX/RX for SPI1 or if the received bytes does not totally match the transmitted bytes.
 * This functionality can be tested by temporarily disconnecting the MISO pin while running this example.
 */
#define ERROR_PIN_SPI1   9UL

#endif /* SPI_MASTER_CONFIG_H */
