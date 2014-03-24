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
 *
 * @defgroup spi_master_example_main spi_master_config
 * @{
 * @ingroup spi_master_example
 *
 * @brief Defintions needed for the SPI master driver.
 */
 
#ifndef SPI_MASTER_CONFIG_H__
#define SPI_MASTER_CONFIG_H__

#define SPI_OPERATING_FREQUENCY (0x02000000uL << (uint32_t)Freq_1Mbps)  /**< Slave clock frequency. */

// SPI0. 
#define SPI_PSELSCK0            23u                                     /**< SPI clock GPIO pin number. */
#define SPI_PSELMOSI0           20u                                     /**< SPI Master Out Slave In GPIO pin number. */
#define SPI_PSELMISO0           22u                                     /**< SPI Master In Slave Out GPIO pin number. */
#define SPI_PSELSS0             21u                                     /**< SPI Slave Select GPIO pin number. */

// SPI1.
#define SPI_PSELSCK1            23u                                     /**< SPI clock GPIO pin number. */
#define SPI_PSELMOSI1           20u                                     /**< SPI Master Out Slave In GPIO pin number. */
#define SPI_PSELMISO1           22u                                     /**< SPI Master In Slave Out GPIO pin number. */
#define SPI_PSELSS1             21u                                     /**< SPI Slave Select GPIO pin number. */

#define TIMEOUT_COUNTER         0x3000uL                                /**< Timeout for SPI transaction in units of loop iterations. */

#endif // SPI_MASTER_CONFIG_H__

/** @} */
