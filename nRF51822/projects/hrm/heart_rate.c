/**************************************************************************/
/*!
    @file     heart_rate.c
*/
/**************************************************************************/
#include "common.h"
#include "boards/board.h"
#include "heart_rate.h"
#include "ble_hrs.h"

static app_timer_id_t    m_heart_rate_timer_id;
static volatile uint16_t m_cur_heart_rate;
ble_hrs_t                m_hrs;

static void heart_rate_meas_timeout_handler(void * p_context);

/**************************************************************************/
/*!
    @brief      Initialises the heart rate monitor service

    @returns
*/
/**************************************************************************/
error_t heart_rate_init(void)
{
  uint8_t body_sensor_location = BLE_HRS_BODY_SENSOR_LOCATION_FINGER;

  ASSERT_STATUS ( app_timer_create(&m_heart_rate_timer_id, APP_TIMER_MODE_REPEATED, heart_rate_meas_timeout_handler) );

  ble_hrs_init_t hrs_init =
  {
    .is_sensor_contact_supported = false,
    .p_body_sensor_location      = &body_sensor_location,
    .evt_handler                 = NULL // heart_rate_service_cb
  };

  /* The security level for the Heart Rate Service can be set here */
  BLE_GAP_CONN_SEC_MODE_SET_OPEN      (&hrs_init.hrs_hrm_attr_md.cccd_write_perm );
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS (&hrs_init.hrs_hrm_attr_md.read_perm       );
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS (&hrs_init.hrs_hrm_attr_md.write_perm      );

  BLE_GAP_CONN_SEC_MODE_SET_OPEN      (&hrs_init.hrs_bsl_attr_md.read_perm       );
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS (&hrs_init.hrs_bsl_attr_md.write_perm      );

  ASSERT_STATUS( ble_hrs_init(&m_hrs, &hrs_init) );

  return ERROR_NONE;
}

/**************************************************************************/
/*!
    @brief      BLE event handler for the heart rate monitor service

    @param[in]  p_ble_evt
    
    @returns
*/
/**************************************************************************/
void heart_rate_handler(ble_evt_t * p_ble_evt)
{
  /* Invoke the service lib handler */
  ble_hrs_on_ble_evt(&m_hrs, p_ble_evt);

  /* Now run the custom event handler code */
  switch (  p_ble_evt->header.evt_id )
  {
    case BLE_GAP_EVT_CONNECTED:
      /* Initialize the current heart rate to the average of max and min
       * values. So that every time a new connection is made, the heart
       * rate starts from the same value. */
      m_cur_heart_rate = 100;

      /* Start the timers used to generate HR measurements */
      #define HEART_RATE_MEAS_INTERVAL             APP_TIMER_TICKS(1000, CFG_TIMER_PRESCALER) /**< Heart rate measurement interval (ticks). */
      ASSERT_STATUS_RET_VOID ( app_timer_start(m_heart_rate_timer_id, HEART_RATE_MEAS_INTERVAL, NULL) );
    break;

    case BLE_GAP_EVT_DISCONNECTED:

    break;

    default: break;
  }
}

/**************************************************************************/
/*!
    @brief      Timeout handler for the heart rate monitor service

    @param[in]  p_context
    
    @returns
*/
/**************************************************************************/
static void heart_rate_meas_timeout_handler(void * p_context)
{
  (void) p_context;

  error_t err_code;

  uint32_t offset; // -1 0 +1
  app_timer_cnt_get(&offset);

  m_cur_heart_rate += (offset%3);
  m_cur_heart_rate--;

  err_code = (error_t) ble_hrs_heart_rate_measurement_send(&m_hrs, m_cur_heart_rate);

  if ((err_code != NRF_SUCCESS                      ) &&
      (err_code != NRF_ERROR_INVALID_STATE          ) &&
      (err_code != BLE_ERROR_NO_TX_BUFFERS          ) &&
      (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING ) )
  {
    ASSERT_STATUS_RET_VOID(err_code);
  }
}

/* Callback from service lib */
//static void heart_rate_service_cb(ble_hrs_t * p_hrs, ble_hrs_evt_t * p_evt)
//{
//
//}
