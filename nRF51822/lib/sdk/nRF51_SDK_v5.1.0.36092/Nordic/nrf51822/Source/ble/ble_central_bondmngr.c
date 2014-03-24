#include "ble_central_bondmngr.h"

#include "ble_flash.h"
#include "simple_uart.h"
#include "app_error.h"

#include <string.h>
#include <stdio.h>

#define BLE_CENTRAL_BONDMNGR_PAGE_NUM (NRF_FICR->CODESIZE-3)

typedef struct
{
    ble_gap_addr_t addr;
    ble_gap_sec_keyset_t keyset;
    ble_gap_enc_key_t enc_key;
} peripheral_bond_info_t;

static peripheral_bond_info_t bond_info;
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

uint8_t uart_buffer[20];

static void print_address(ble_gap_addr_t * p_addr)
{
    printf("A: %x:%x:%x:%x:%x:%x\r\n", p_addr->addr[0], p_addr->addr[1], p_addr->addr[2], p_addr->addr[3], p_addr->addr[4], p_addr->addr[5]);
}

uint32_t ble_central_bondmngr_init(ble_central_bondmngr_t * p_bm, ble_central_bondmngr_init_t * p_bm_init)
{
    uint32_t err_code;
    
    p_bm->p_sec_params = p_bm_init->p_sec_params;
    
    bond_info.keyset.keys_periph.p_enc_key = &bond_info.enc_key;
    
    if (p_bm_init->delete_bonds)
    {
        err_code = ble_flash_page_erase(BLE_CENTRAL_BONDMNGR_PAGE_NUM);
        if (err_code != NRF_SUCCESS)
            return err_code;
    }
    else
    {
        uint8_t size_to_read = sizeof(bond_info)/sizeof(uint32_t);
        err_code = ble_flash_page_read(BLE_CENTRAL_BONDMNGR_PAGE_NUM, (uint32_t *) &bond_info, &size_to_read);
        if (err_code != NRF_ERROR_NOT_FOUND && err_code != NRF_SUCCESS)
            return err_code;

        printf("Restoring data from %d, %d...\r\n", BLE_CENTRAL_BONDMNGR_PAGE_NUM, BLE_CENTRAL_BONDMNGR_PAGE_NUM*NRF_FICR->CODEPAGESIZE);
    }
    
    print_address(&bond_info.addr);
    
    return NRF_SUCCESS;
}

void ble_central_bondmngr_on_ble_evt(ble_central_bondmngr_t * p_bm, ble_evt_t * p_ble_evt)
{
    uint32_t err_code = NRF_SUCCESS;
    
    switch(p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            printf("Starting authentication.\r\n");
            
            printf("enc_info: %x\r\n", bond_info.enc_key.enc_info.ltk[0]);
                    
            print_address(&p_ble_evt->evt.gap_evt.params.connected.peer_addr);
            if (memcmp((void *) &bond_info.addr, (void *) &p_ble_evt->evt.gap_evt.params.connected.peer_addr, sizeof(ble_gap_addr_t)) == 0)
            {
                printf("Is reconnecting.\r\n");
                
                err_code = sd_ble_gap_encrypt(m_conn_handle, &bond_info.enc_key.master_id, &bond_info.enc_key.enc_info);
            }
            else
            {
                printf("New bond.\r\n");
                bond_info.addr = p_ble_evt->evt.gap_evt.params.connected.peer_addr;
                
                err_code = sd_ble_gap_authenticate(m_conn_handle, p_bm->p_sec_params);
            }            
            break;
        
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_SUCCESS, NULL, &bond_info.keyset);
            printf("Security parameters requested.\r\n");
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;
        
        default:
            break;
    }
    APP_ERROR_CHECK(err_code);
}

uint32_t ble_central_bondmngr_store(ble_central_bondmngr_t * p_bm)
{
    uint32_t err_code;
    
    print_address(&bond_info.addr);
    
    err_code = ble_flash_page_erase(BLE_CENTRAL_BONDMNGR_PAGE_NUM);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    printf("Writing %d b to %d\r\n", sizeof(bond_info)/sizeof(uint32_t), BLE_CENTRAL_BONDMNGR_PAGE_NUM);
    
    err_code = ble_flash_page_write(BLE_CENTRAL_BONDMNGR_PAGE_NUM, (uint32_t *) &bond_info, sizeof(bond_info)/sizeof(uint32_t));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    return NRF_SUCCESS;
}
