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
 
#include <stdint.h>
#include <stddef.h> 
#include "app_util.h"
#include "app_error.h"
#include "bootloader.h"
#include "bootloader_types.h"
#include "ble_flash.h"
#include "crc16.h"
#include "dfu.h"
#include "dfu_types.h"
#include "nrf.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"
#include "pstorage.h"

/**@brief States of the DFU state machine. */
typedef enum
{
    DFU_STATE_INIT_ERROR,                                    /**< State for: dfu_init(...) error. */
    DFU_STATE_IDLE,                                          /**< State for: idle. */
    DFU_STATE_RDY,                                           /**< State for: ready. */
    DFU_STATE_RX_INIT_PKT,                                   /**< State for: receiving initialization packet. */
    DFU_STATE_RX_DATA_PKT,                                   /**< State for: receiving data packet. */
    DFU_STATE_VALIDATE,                                      /**< State for: validate. */
    DFU_STATE_WAIT_4_ACTIVATE                                /**< State for: waiting for dfu_image_activate(). */
} dfu_state_t;

static dfu_state_t m_dfu_state;                              /**< Current DFU state. */
static uint32_t                m_image_size;                 /**< Size of the image that will be transmitted. */
static uint32_t                m_init_packet[16];            /**< Init packet, can hold CRC, Hash, Signed Hash and similar, for image validation, integrety check and authorization checking. */
static uint8_t                 m_init_packet_length;         /**< Length of init packet received. */
static uint16_t                m_image_crc;                  /**< Calculated CRC of the image received. */
static uint32_t                m_received_data;              /**< Amount of received data. */
static pstorage_handle_t       m_storage_handle_app;
static pstorage_module_param_t m_storage_module_param;
static dfu_callback_t          m_data_pkt_cb;


#define IMAGE_WRITE_IN_PROGRESS() (m_received_data > 0)      /**< Macro for determining is image write in progress. */

static void pstorage_callback_handler(pstorage_handle_t * handle, 
                                      uint8_t             op_code, 
                                      uint32_t            result, 
                                      uint8_t           * p_data, 
                                      uint32_t            data_len)
{
    if ((m_dfu_state == DFU_STATE_RX_DATA_PKT) && 
        (op_code == PSTORAGE_STORE_OP_CODE)    && 
        (result == NRF_SUCCESS))
    {
        if (m_data_pkt_cb != NULL)
        {
            m_data_pkt_cb(result, p_data);
        }
    }
    APP_ERROR_CHECK(result);
}


uint32_t dfu_init(void)
{        
    uint32_t  err_code = NRF_SUCCESS;
    
    m_dfu_state          = DFU_STATE_IDLE;    
    m_init_packet_length = 0;
    m_image_crc          = 0;    

    m_storage_module_param.cb          = pstorage_callback_handler;

    err_code = pstorage_raw_register(&m_storage_module_param, &m_storage_handle_app);
    if (err_code != NRF_SUCCESS)
    {
        m_dfu_state = DFU_STATE_INIT_ERROR;
        return err_code;
    }

    m_storage_handle_app.block_id   = CODE_REGION_1_START;

    return NRF_SUCCESS;
}


void dfu_register_callback(dfu_callback_t callback_handler)
{
    m_data_pkt_cb = callback_handler;
}


uint32_t dfu_image_size_set(uint32_t image_size)
{
    uint32_t            err_code;
    dfu_update_status_t update_status;
    
    err_code = NRF_ERROR_INVALID_STATE;
    
    if (image_size > DFU_IMAGE_MAX_SIZE_FULL)
    {
        return NRF_ERROR_DATA_SIZE;
    }   
    
    if ((image_size & (sizeof(uint32_t) - 1)) != 0)
    {
        // Image_size is not a multiple of 4 (word size).
        return NRF_ERROR_NOT_SUPPORTED;
    }    
    
    switch (m_dfu_state)
    {
        case DFU_STATE_IDLE:
            err_code = pstorage_raw_clear(&m_storage_handle_app, image_size);
            if (err_code != NRF_SUCCESS)
            {
                return err_code;
            }

            m_received_data           = 0;
            m_image_size              = image_size;
            m_dfu_state               = DFU_STATE_RDY;
            update_status.status_code = DFU_BANK_0_ERASED;
            
            bootloader_dfu_update_process(update_status);                    
            break;
            
        default:
            err_code = NRF_ERROR_INVALID_STATE;
            break;    
    }
    
    return err_code;
}


uint32_t dfu_data_pkt_handle(dfu_update_packet_t * p_packet)
{
    uint32_t  err_code;
    uint32_t  data_length;
    uint8_t * p_data;
    
    if (p_packet == NULL)
    {
        return NRF_ERROR_NULL;
    }

    // Check pointer alignment.
    if (!is_word_aligned(p_packet->p_data_packet))
    {
        // The p_data_packet is not word aligned address.
        return NRF_ERROR_INVALID_ADDR;
    }

    switch (m_dfu_state)
    {
        case DFU_STATE_RDY:
            // Fall-through.
        
        case DFU_STATE_RX_INIT_PKT:
            m_dfu_state = DFU_STATE_RX_DATA_PKT;
        
            // Fall-through.
        
        case DFU_STATE_RX_DATA_PKT:
            data_length = p_packet->packet_length  * sizeof(uint32_t);
            if ((uint32_t)(m_received_data + data_length) > m_image_size)
            {
                // The caller is trying to write more bytes into the flash than the size provided to 
                // the dfu_image_size_set function.
                return NRF_ERROR_DATA_SIZE;
            }

            p_data   = (uint8_t *)p_packet->p_data_packet;            
            err_code = pstorage_raw_store(&m_storage_handle_app, p_data, data_length, m_received_data);
            if (err_code != NRF_SUCCESS)
            {
                return err_code;
            }
            m_received_data += data_length;        
            
            break;
            
        default:
            err_code = NRF_ERROR_INVALID_STATE;
            break;
    }
    
    return err_code;
}


uint32_t dfu_init_pkt_handle(dfu_update_packet_t * p_packet)
{
    uint32_t err_code;
    uint32_t i;
    
    switch (m_dfu_state)
    {
        case DFU_STATE_RDY:
            m_dfu_state = DFU_STATE_RX_INIT_PKT;
            // Fall-through.     
            
        case DFU_STATE_RX_INIT_PKT:
            // DFU initialization has been done and a start packet has been received.
            if (IMAGE_WRITE_IN_PROGRESS())
            {
                // Image write is already in progress. Cannot handle an init packet now.
                return NRF_ERROR_INVALID_STATE;
            }

            for (i = 0; i < p_packet->packet_length; i++)
            {
                m_init_packet[m_init_packet_length++] = p_packet->p_data_packet[i];
            }
            
            err_code = NRF_SUCCESS;
            break;

        default:
            // Either the start packet was not received or dfu_init function was not called before.
            err_code = NRF_ERROR_INVALID_STATE;        
            break;
    }
    
    return err_code;    
}


uint32_t dfu_image_validate()
{
    uint32_t err_code;
    uint16_t received_crc;    

    switch (m_dfu_state)
    {
        case DFU_STATE_RX_DATA_PKT:
            m_dfu_state = DFU_STATE_VALIDATE;
            
            // Check if the application image write has finished.
            if (m_received_data != m_image_size)
            {
                // Image not yet fully transferred by the peer.
                err_code = NRF_ERROR_INVALID_STATE;
            }
            else
            {
                // Calculate CRC from DFU_BANK_0_REGION_START to mp_app_write_address.
                m_image_crc  = crc16_compute((uint8_t*)DFU_BANK_0_REGION_START, 
                                             m_image_size, 
                                             NULL);
                received_crc = uint16_decode((uint8_t*)&m_init_packet[0]);
                    
                if ((m_init_packet_length != 0) && (m_image_crc != received_crc))
                {
                    return NRF_ERROR_INVALID_DATA;
                }
                m_dfu_state = DFU_STATE_WAIT_4_ACTIVATE;
                err_code    = NRF_SUCCESS;
            }

            break;            
            
        default:
            err_code = NRF_ERROR_INVALID_STATE;
            break;  
    }
    
    return err_code;    
}


uint32_t dfu_image_activate()
{
    uint32_t            err_code;
    dfu_update_status_t update_status;
    
    switch (m_dfu_state)
    {    
        case DFU_STATE_WAIT_4_ACTIVATE:            
            update_status.status_code = DFU_UPDATE_COMPLETE;
            update_status.app_crc     = m_image_crc;
            update_status.app_size    = m_image_size;

            bootloader_dfu_update_process(update_status);        
            err_code = NRF_SUCCESS;
            break;
            
        default:
            err_code = NRF_ERROR_INVALID_STATE;
            break;
    }
    
    return err_code;
}


void dfu_reset(void)
{
    dfu_update_status_t update_status;
    
    update_status.status_code = DFU_RESET;

    bootloader_dfu_update_process(update_status);
}
