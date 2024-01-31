#include "sdk_common.h"
#include "ble_srv_common.h"
#include "ElapsedTimeService.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"
#include "math.h"
#include "nrf_log_ctrl.h"

BLE_ELAPSED_TIME_DEF(m_elapsed_time_service);

uint32_t elapsed_time_service_elapsed_time_char_add(ble_elapsed_time_service_t * p_elapsed_time_service, const ble_elapsed_time_service_init_t * p_ble_elapsed_time_service_init);

uint32_t ble_elapsed_time_service_init()
{
    ble_elapsed_time_service_init_t elapsed_time_service_init;
    
     // Initialize custom Service init structure to zero.
    memset(&elapsed_time_service_init, 0, sizeof(elapsed_time_service_init));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&elapsed_time_service_init.elapsed_time_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&elapsed_time_service_init.elapsed_time_attr_md.write_perm);

    // Set the weight sensor event handler
    elapsed_time_service_init.evt_handler = ble_elapsed_time_on_elapsed_time_evt;

    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    m_elapsed_time_service.conn_handle = BLE_CONN_HANDLE_INVALID;

        // Add service
    BLE_UUID_BLE_ASSIGN(ble_uuid, ELAPSED_TIME_SERVICE_UUID_BASE);

    // Add the Custom Service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_elapsed_time_service.service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Initialize service structure
    m_elapsed_time_service.evt_handler           = elapsed_time_service_init.evt_handler;
    m_elapsed_time_service.conn_handle           = BLE_CONN_HANDLE_INVALID;

    /*  From here all the characteristics are added to the service in the order
     *  they will be discovered by the central.
     */
    err_code = elapsed_time_service_elapsed_time_char_add(&m_elapsed_time_service, &elapsed_time_service_init);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    return NRF_SUCCESS;
}

uint32_t elapsed_time_service_elapsed_time_char_add(ble_elapsed_time_service_t * p_elapsed_time_service, const ble_elapsed_time_service_init_t * p_ble_elapsed_time_service_init)
{
uint32_t            err_code;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    // Read  operation on Cccd should be possible without authentication.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.read   = 1;
    char_md.char_props.write  = 0;
    char_md.char_props.notify = 1; 
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md; 
    char_md.p_sccd_md         = NULL;

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm  = p_ble_elapsed_time_service_init->elapsed_time_attr_md.read_perm;
    attr_md.write_perm = p_ble_elapsed_time_service_init->elapsed_time_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    BLE_UUID_BLE_ASSIGN(ble_uuid, CURRENT_ELAPSED_TIME_UUID);

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(ble_elapsed_time_t);
    attr_char_value.init_offs = 0;

    ble_elapsed_time_t reset_elapsed_time;
    memset(&reset_elapsed_time, 0, sizeof(reset_elapsed_time));
    
            
    attr_char_value.p_value   = (uint8_t*)&reset_elapsed_time; // Pointer to the initial value

    attr_char_value.max_len   = sizeof(ble_elapsed_time_t);

    err_code = sd_ble_gatts_characteristic_add(m_elapsed_time_service.service_handle, &char_md,
                                               &attr_char_value,
                                               &m_elapsed_time_service.elapsed_time_handles);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}    


void ble_elapsed_time_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_elapsed_time_service_t * p_elapsed_time_service = (ble_elapsed_time_service_t *) p_context;
    
    if (p_elapsed_time_service == NULL || p_ble_evt == NULL)
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            ble_elapsed_time_on_connect(p_elapsed_time_service, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            ble_elapsed_time_on_disconnect(p_elapsed_time_service, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            ble_elapsed_time_on_write(p_elapsed_time_service, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void ble_elapsed_time_on_connect(ble_elapsed_time_service_t * p_elapsed_time_service, ble_evt_t const * p_ble_evt)
{
    m_elapsed_time_service.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

    ble_elapsed_time_evt_t evt;

    evt.evt_type = BLE_ELAPSED_TIME_EVT_CONNECTED;
    
    // check if the weight_sensor service ble event handler has been initialised before calling it
    if (m_elapsed_time_service.evt_handler == NULL)
    {
        return;
    }

    m_elapsed_time_service.evt_handler(p_elapsed_time_service, &evt);
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void ble_elapsed_time_on_disconnect(ble_elapsed_time_service_t * p_elapsed_time_service, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    m_elapsed_time_service.conn_handle = BLE_CONN_HANDLE_INVALID;
}

/* This code belongs in ble_cus.c*/

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void ble_elapsed_time_on_write(ble_elapsed_time_service_t * p_elapsed_time_service, ble_evt_t const * p_ble_evt)
{
   const ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    
};


uint32_t ble_elapsed_time_service_elapsed_time_update(ble_elapsed_time_t elapsed_time)
{
    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = sizeof(ble_elapsed_time_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = (uint8_t*)&elapsed_time;

    // Update database.
    err_code= sd_ble_gatts_value_set(m_elapsed_time_service.conn_handle,
                                        m_elapsed_time_service.elapsed_time_handles.value_handle,
                                        &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Send value if connected and notifying.
    if ((m_elapsed_time_service.conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = m_elapsed_time_service.elapsed_time_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;

        err_code = sd_ble_gatts_hvx(m_elapsed_time_service.conn_handle, &hvx_params);
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}


/**@brief Function for handling the Accelerometer Service Service events.
 *
 * @details This function will be called for all Accelerometer Service events which are passed to
 *          the application.
 *
 * @param[in]   p_elapsed_time_service  Accelerometer Service structure.
 * @param[in]   p_evt          Event received from the Accelerometer Service.
 *
 */
void ble_elapsed_time_on_elapsed_time_evt(ble_elapsed_time_service_t * p_elapsed_time_service, ble_elapsed_time_evt_t * p_evt)
{
    ret_code_t err_code;
    switch(p_evt->evt_type)
    {
        case BLE_ELAPSED_TIME_EVT_NOTIFICATION_ENABLED:
            break;

        case BLE_ELAPSED_TIME_EVT_NOTIFICATION_DISABLED:      
            break;

        case BLE_ELAPSED_TIME_EVT_CONNECTED:
            break;

        case BLE_ELAPSED_TIME_EVT_DISCONNECTED:
              break;

        default:
              // No implementation needed.
              break;
    }
}
