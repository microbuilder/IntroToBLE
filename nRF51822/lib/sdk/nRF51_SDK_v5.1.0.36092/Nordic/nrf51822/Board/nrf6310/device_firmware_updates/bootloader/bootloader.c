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

#include "bootloader.h"
#include <string.h>
#include "bootloader_types.h"
#include "bootloader_util.h"
#include "dfu.h"
#include "dfu_transport.h"
#include "nrf51.h"
#include "app_error.h"
#include "nrf_sdm.h"
#include "ble_flash.h"
#include "nordic_common.h"
#include "crc16.h"
#include "pstorage.h"
#include "app_scheduler.h"

#define IRQ_ENABLED             0x01                    /**< Field identifying if an interrupt is enabled. */
#define MAX_NUMBER_INTERRUPTS   32                      /**< Maximum number of interrupts available. */

typedef enum
{
    BOOTLOADER_UPDATING,
    BOOTLOADER_SETTINGS_SAVING,
    BOOTLOADER_COMPLETE,
    BOOTLOADER_TIMEOUT,
    BOOTLOADER_RESET,
} bootloader_status_t;

static pstorage_handle_t        m_bootsettings_handle;  /**< Pstorage handle to use for registration and identifying the bootloader module on subsequent calls to the pstorage module for load and store of bootloader setting in flash. */
static bootloader_status_t      m_update_status;        /**< Current update status for the bootloader module to ensure correct behaviour when updating settings and when update completes. */


static void pstorage_callback_handler(pstorage_handle_t * handle, uint8_t op_code, uint32_t result, uint8_t * p_data, uint32_t data_len)
{
    // If we are in BOOTLOADER_SETTINGS_SAVING state and we receive an PSTORAGE_STORE_OP_CODE 
    // response then settings has been saved and update has completed.
    if ((m_update_status == BOOTLOADER_SETTINGS_SAVING) && (op_code == PSTORAGE_STORE_OP_CODE))
    {
        m_update_status = BOOTLOADER_COMPLETE;
    }
    APP_ERROR_CHECK(result);
}


/**@brief   Function for waiting for events.
 *
 * @details This function will place the chip in low power mode while waiting for events from
 *          the S110 SoftDevice or other peripherals. When interrupted by an event, it will call the
 *          @ref app_sched_execute function to process the received event. This function will return
 *          when the final state of the firmware update is reached OR when a tear down is in
 *          progress.
 */
static void wait_for_events(void)
{
    for (;;)
    {
        // Wait in low power state for any events.
        uint32_t err_code = sd_app_evt_wait();
        APP_ERROR_CHECK(err_code);

        // Event received. Process it from the scheduler.
        app_sched_execute();

        if ((m_update_status == BOOTLOADER_COMPLETE) || 
            (m_update_status == BOOTLOADER_TIMEOUT)  ||
            (m_update_status == BOOTLOADER_RESET))
        {
            // When update has completed or a timeout/reset occured we will return.
            return;
        }
    }
}


bool bootloader_app_is_valid(uint32_t app_addr)
{
    const bootloader_settings_t * p_bootloader_settings;

    // There exists an application in CODE region 1.
    if (DFU_BANK_0_REGION_START == EMPTY_FLASH_MASK)
    {
        return false;
    }
    
    bool success = false;
    
    switch (app_addr)
    {
        case DFU_BANK_0_REGION_START:
            bootloader_util_settings_get(&p_bootloader_settings);

            // The application in CODE region 1 is flagged as valid during update.
            if (p_bootloader_settings->bank_0 == BANK_VALID_APP)
            {
                uint16_t image_crc = 0;

                // A stored crc value of 0 indicates that CRC checking is not used.
                if (p_bootloader_settings->bank_0_crc != 0)
                {
                    image_crc = crc16_compute((uint8_t*)DFU_BANK_0_REGION_START,
                                              p_bootloader_settings->bank_0_size,
                                              NULL);
                }

                success = (image_crc == p_bootloader_settings->bank_0_crc);
            }
            break;
            
        case DFU_BANK_1_REGION_START:
        default:
            // No implementation needed.
            break;
    }

    return success;
}


static void bootloader_settings_save(bootloader_settings_t * p_settings)
{
    uint32_t err_code = pstorage_clear(&m_bootsettings_handle, sizeof(bootloader_settings_t));
    APP_ERROR_CHECK(err_code);

    err_code = pstorage_store(&m_bootsettings_handle, 
                              (uint8_t *)p_settings, 
                              sizeof(bootloader_settings_t),
                              0);
    APP_ERROR_CHECK(err_code);
}


void bootloader_dfu_update_process(dfu_update_status_t update_status)
{
    static bootloader_settings_t settings;
    const bootloader_settings_t * p_bootloader_settings;

    bootloader_util_settings_get(&p_bootloader_settings);

    if (update_status.status_code == DFU_UPDATE_COMPLETE)
    {
        settings.bank_0_crc  = update_status.app_crc;
        settings.bank_0_size = update_status.app_size;
        settings.bank_0      = BANK_VALID_APP;
        settings.bank_1      = BANK_INVALID_APP;
        
        m_update_status      = BOOTLOADER_SETTINGS_SAVING;
        bootloader_settings_save(&settings);
    }
    else if (update_status.status_code == DFU_TIMEOUT)
    {
        // Timeout has occurred. Close the connection with the DFU Controller.
        uint32_t err_code = dfu_transport_close();
        APP_ERROR_CHECK(err_code);

        m_update_status      = BOOTLOADER_TIMEOUT;
    }
    else if (update_status.status_code == DFU_BANK_0_ERASED)
    {
        settings.bank_0_crc  = 0;
        settings.bank_0_size = 0;
        settings.bank_0      = BANK_ERASED;
        settings.bank_1      = p_bootloader_settings->bank_1;
        
        bootloader_settings_save(&settings);
    }
    else if (update_status.status_code == DFU_BANK_1_ERASED)
    {
        settings.bank_0      = p_bootloader_settings->bank_0;
        settings.bank_0_crc  = p_bootloader_settings->bank_0_crc;
        settings.bank_0_size = p_bootloader_settings->bank_0_size;
        settings.bank_1      = BANK_ERASED;
        
        bootloader_settings_save(&settings);
    }
    else if (update_status.status_code == DFU_RESET)
    {
        // Timeout has occurred. Close the connection with the DFU Controller.
        uint32_t err_code = dfu_transport_close();
        APP_ERROR_CHECK(err_code);

        m_update_status      = BOOTLOADER_RESET;
    }
    else
    {
        // No implementation needed.
    }
}


uint32_t bootloader_dfu_start(void)
{
    uint32_t                err_code = NRF_SUCCESS;
    pstorage_module_param_t storage_params;

    storage_params.cb          = pstorage_callback_handler;
    storage_params.block_size  = sizeof(bootloader_settings_t);
    storage_params.block_count = 1;
    
    err_code = pstorage_init();
    if (err_code != NRF_SUCCESS)    
    {
        return err_code;
    }

    err_code = pstorage_register(&storage_params, &m_bootsettings_handle);
    if (err_code != NRF_SUCCESS)    
    {
        return err_code;
    }

    // Clear swap if banked update is used.    
    err_code = dfu_init(); 
    if (err_code != NRF_SUCCESS)    
    {
        return err_code;
    }
    
    err_code = dfu_transport_update_start();

    wait_for_events();
    
    return err_code;
}


/**@brief Function for disabling all interrupts before jumping from bootloader to application.
 */
static void interrupts_disable(void)
{
    uint32_t interrupt_setting_mask;
    uint8_t  irq;

    // We start the loop from first interrupt, i.e. interrupt 0.
    irq                    = 0;
    // Fetch the current interrupt settings.
    interrupt_setting_mask = NVIC->ISER[0];
    
    for (; irq < MAX_NUMBER_INTERRUPTS; irq++)
    {
        if (interrupt_setting_mask & (IRQ_ENABLED << irq))
        {
            // The interrupt was enabled, and hence disable it.
            NVIC_DisableIRQ((IRQn_Type) irq);
        }
    }        
}


void bootloader_app_start(uint32_t app_addr)
{
    // If the applications CRC has been checked and passed, the magic number will be written and we
    // can start the application safely.
    uint32_t err_code = sd_softdevice_disable();
    APP_ERROR_CHECK(err_code);

    interrupts_disable();

    err_code = sd_softdevice_forward_to_application();
    APP_ERROR_CHECK(err_code);

    bootloader_util_app_start(CODE_REGION_1_START);
}


void bootloader_settings_get(bootloader_settings_t * const p_settings)
{
    bootloader_settings_t bootloader_settings;

    (void) pstorage_load((uint8_t *) &bootloader_settings, &m_bootsettings_handle, sizeof(bootloader_settings_t), 0);

    p_settings->bank_0      = bootloader_settings.bank_0;
    p_settings->bank_0_crc  = bootloader_settings.bank_0_crc;
    p_settings->bank_0_size = bootloader_settings.bank_0_size;
    p_settings->bank_1      = bootloader_settings.bank_1;
}
