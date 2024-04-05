#include "sdk_common.h"
#include "ButtonThresholdService.h"
#include <string.h>
#include "ble_srv_common.h"
#include "ble_conn_state.h"
#include "nrf_log.h"

#define INVALID_BATTERY_LEVEL 255

void (*mButton1ThresholdReceivedCallback)(uint16_t threshold) = NULL;
void (*mButton2ThresholdReceivedCallback)(uint16_t threshold) = NULL;
void (*mButton3ThresholdReceivedCallback)(uint16_t threshold) = NULL;
void (*mButton4ThresholdReceivedCallback)(uint16_t threshold) = NULL;

BUTTON_THRESHOLD_SERVICE_DEF(m_button_threshold_service);

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_bas       Battery Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(button_threshold_service_t * p_button_threshold_service, ble_evt_t const * p_ble_evt)
{
    if (!p_button_threshold_service->is_notification_supported)
    {
        return;
    }

    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (    (p_evt_write->handle == p_button_threshold_service->button1_threshold_handles.cccd_handle)
        &&  (p_evt_write->len == 2))
    {
        if (p_button_threshold_service->evt_handler == NULL)
        {
            return;
        }

        button_threshold_service_evt_t evt;

        if (ble_srv_is_notification_enabled(p_evt_write->data))
        {
            evt.evt_type = BUTTON_THRESHOLD_SERVICE_EVT_NOTIFICATION_ENABLED;
        }
        else
        {
            evt.evt_type = BUTTON_THRESHOLD_SERVICE_EVT_NOTIFICATION_DISABLED;
        }
        evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;

        // CCCD written, call application event handler.
        p_button_threshold_service->evt_handler(p_button_threshold_service, &evt);
    }

    if ((p_evt_write->handle == m_button_threshold_service.button1_threshold_handles.value_handle)
        && (p_evt_write->len == 2)
       )
    {
        NRF_LOG_INFO("Value Received from button1 threshold.");

        if (*mButton1ThresholdReceivedCallback != NULL)
        {
            (mButton1ThresholdReceivedCallback)((uint16_t)(uint16_t)p_evt_write->data[1] << 8 | (uint16_t)p_evt_write->data[0]);
        }
        else
        {
            NRF_LOG_INFO("No Button1 threshold callback set.");
        }
    }

    if ((p_evt_write->handle == m_button_threshold_service.button2_threshold_handles.value_handle)
        && (p_evt_write->len == 2)
       )
    {
        NRF_LOG_INFO("Value Received from button2 threshold.");

        if (*mButton2ThresholdReceivedCallback != NULL)
        {
            (mButton2ThresholdReceivedCallback)((uint16_t)(uint16_t)p_evt_write->data[1] << 8 | (uint16_t)p_evt_write->data[0]);
        }
        else
        {
            NRF_LOG_INFO("No Button2 threshold callback set.");
        }
    }

    if ((p_evt_write->handle == m_button_threshold_service.button3_threshold_handles.value_handle)
        && (p_evt_write->len == 2)
       )
    {
        NRF_LOG_INFO("Value Received from button3 threshold.");

        if (*mButton3ThresholdReceivedCallback != NULL)
        {
            (mButton3ThresholdReceivedCallback)((uint16_t)(uint16_t)p_evt_write->data[1] << 8 | (uint16_t)p_evt_write->data[0]);
        }
        else
        {
            NRF_LOG_INFO("No Button3 threshold callback set.");
        }
    }

    if ((p_evt_write->handle == m_button_threshold_service.button4_threshold_handles.value_handle)
        && (p_evt_write->len == 2)
       )
    {
        NRF_LOG_INFO("Value Received from button4 threshold.");

        if (*mButton4ThresholdReceivedCallback != NULL)
        {
            (mButton4ThresholdReceivedCallback)((uint16_t)(uint16_t)p_evt_write->data[1] << 8 | (uint16_t)p_evt_write->data[0]);
        }
        else
        {
            NRF_LOG_INFO("No Button4 threshold callback set.");
        }
    }
}


void button_threshold_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    button_threshold_service_t * p_button_threshold_service = (button_threshold_service_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            on_write(p_button_threshold_service, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for adding the Battery Level characteristic.
 *
 * @param[in]   p_bas        Battery Service structure.
 * @param[in]   p_bas_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t button_threshold_service_button1_threshold_char_add(const button_threshold_service_init_t * p_button_threshold_service_init)
{
    ret_code_t             err_code;
    ble_add_char_params_t  add_char_params;
    ble_add_descr_params_t add_descr_params;
    uint16_t               initial_button1_threshold;
    uint8_t                init_len;
    uint8_t                encoded_report_ref[BLE_SRV_ENCODED_REPORT_REF_LEN];

    // Add battery level characteristic
    initial_button1_threshold = p_button_threshold_service_init->initial_button1_threshold;

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = BUTTON_THRESHOLD_SERVICE_BUTTON1_THRESHOLD_CHAR_UUID;
    add_char_params.uuid_type         = m_button_threshold_service.uuid_type;
    add_char_params.max_len           = sizeof(uint16_t);
    add_char_params.init_len          = sizeof(uint16_t);
    add_char_params.p_init_value      = (uint8_t*)&initial_button1_threshold;
    add_char_params.char_props.notify = m_button_threshold_service.is_notification_supported;
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.write   = 1;
    add_char_params.cccd_write_access = SEC_OPEN;
    add_char_params.read_access       = p_button_threshold_service_init->bl_rd_sec;
    add_char_params.write_access = SEC_OPEN;

    err_code = characteristic_add(m_button_threshold_service.service_handle,
                                  &add_char_params,
                                  &(m_button_threshold_service.button1_threshold_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    if (p_button_threshold_service_init->p_report_ref != NULL)
    {
        // Add Report Reference descriptor
        init_len = ble_srv_report_ref_encode(encoded_report_ref, p_button_threshold_service_init->p_report_ref);

        memset(&add_descr_params, 0, sizeof(add_descr_params));
        add_descr_params.uuid        = BLE_UUID_REPORT_REF_DESCR;
        add_descr_params.read_access = p_button_threshold_service_init->bl_report_rd_sec;
        add_descr_params.write_access = SEC_OPEN;
        add_descr_params.init_len    = init_len;
        add_descr_params.max_len     = add_descr_params.init_len;
        add_descr_params.p_value     = encoded_report_ref;

        err_code = descriptor_add(m_button_threshold_service.button1_threshold_handles.value_handle,
                                  &add_descr_params,
                                  &m_button_threshold_service.report_ref_handle);
        return err_code;
    }
    else
    {
        m_button_threshold_service.report_ref_handle = BLE_GATT_HANDLE_INVALID;
    }

    return NRF_SUCCESS;
}

static ret_code_t button_threshold_service_button2_threshold_char_add(const button_threshold_service_init_t * p_button_threshold_service_init)
{
    ret_code_t             err_code;
    ble_add_char_params_t  add_char_params;
    ble_add_descr_params_t add_descr_params;
    uint16_t               initial_button2_threshold;
    uint8_t                init_len;
    uint8_t                encoded_report_ref[BLE_SRV_ENCODED_REPORT_REF_LEN];

    // Add battery level characteristic
    initial_button2_threshold = p_button_threshold_service_init->initial_button2_threshold;

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = BUTTON_THRESHOLD_SERVICE_BUTTON2_THRESHOLD_CHAR_UUID;
    add_char_params.uuid_type         = m_button_threshold_service.uuid_type;
    add_char_params.max_len           = sizeof(uint16_t);
    add_char_params.init_len          = sizeof(uint16_t);
    add_char_params.p_init_value      = (uint8_t*)&initial_button2_threshold;
    add_char_params.char_props.notify = m_button_threshold_service.is_notification_supported;
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.write   = 1;
    add_char_params.cccd_write_access = SEC_OPEN;
    add_char_params.read_access       = p_button_threshold_service_init->bl_rd_sec;
    add_char_params.write_access = SEC_OPEN;

    err_code = characteristic_add(m_button_threshold_service.service_handle,
                                  &add_char_params,
                                  &(m_button_threshold_service.button2_threshold_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    if (p_button_threshold_service_init->p_report_ref != NULL)
    {
        // Add Report Reference descriptor
        init_len = ble_srv_report_ref_encode(encoded_report_ref, p_button_threshold_service_init->p_report_ref);

        memset(&add_descr_params, 0, sizeof(add_descr_params));
        add_descr_params.uuid        = BLE_UUID_REPORT_REF_DESCR;
        add_descr_params.read_access = p_button_threshold_service_init->bl_report_rd_sec;
        add_descr_params.write_access = SEC_OPEN;
        add_descr_params.init_len    = init_len;
        add_descr_params.max_len     = add_descr_params.init_len;
        add_descr_params.p_value     = encoded_report_ref;

        err_code = descriptor_add(m_button_threshold_service.button2_threshold_handles.value_handle,
                                  &add_descr_params,
                                  &m_button_threshold_service.report_ref_handle);
        return err_code;
    }
    else
    {
        m_button_threshold_service.report_ref_handle = BLE_GATT_HANDLE_INVALID;
    }

    return NRF_SUCCESS;
}

static ret_code_t button_threshold_service_button3_threshold_char_add(const button_threshold_service_init_t * p_button_threshold_service_init)
{
    ret_code_t             err_code;
    ble_add_char_params_t  add_char_params;
    ble_add_descr_params_t add_descr_params;
    uint16_t               initial_button3_threshold;
    uint8_t                init_len;
    uint8_t                encoded_report_ref[BLE_SRV_ENCODED_REPORT_REF_LEN];

    // Add battery level characteristic
    initial_button3_threshold = p_button_threshold_service_init->initial_button3_threshold;

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = BUTTON_THRESHOLD_SERVICE_BUTTON3_THRESHOLD_CHAR_UUID;
    add_char_params.uuid_type         = m_button_threshold_service.uuid_type;
    add_char_params.max_len           = sizeof(uint16_t);
    add_char_params.init_len          = sizeof(uint16_t);
    add_char_params.p_init_value      = (uint8_t*)&initial_button3_threshold;
    add_char_params.char_props.notify = m_button_threshold_service.is_notification_supported;
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.write   = 1;
    add_char_params.cccd_write_access = SEC_OPEN;
    add_char_params.read_access       = p_button_threshold_service_init->bl_rd_sec;
    add_char_params.write_access = SEC_OPEN;

    err_code = characteristic_add(m_button_threshold_service.service_handle,
                                  &add_char_params,
                                  &(m_button_threshold_service.button3_threshold_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    if (p_button_threshold_service_init->p_report_ref != NULL)
    {
        // Add Report Reference descriptor
        init_len = ble_srv_report_ref_encode(encoded_report_ref, p_button_threshold_service_init->p_report_ref);

        memset(&add_descr_params, 0, sizeof(add_descr_params));
        add_descr_params.uuid        = BLE_UUID_REPORT_REF_DESCR;
        add_descr_params.read_access = p_button_threshold_service_init->bl_report_rd_sec;
        add_descr_params.write_access = SEC_OPEN;
        add_descr_params.init_len    = init_len;
        add_descr_params.max_len     = add_descr_params.init_len;
        add_descr_params.p_value     = encoded_report_ref;

        err_code = descriptor_add(m_button_threshold_service.button3_threshold_handles.value_handle,
                                  &add_descr_params,
                                  &m_button_threshold_service.report_ref_handle);
        return err_code;
    }
    else
    {
        m_button_threshold_service.report_ref_handle = BLE_GATT_HANDLE_INVALID;
    }

    return NRF_SUCCESS;
}

static ret_code_t button_threshold_service_button4_threshold_char_add(const button_threshold_service_init_t * p_button_threshold_service_init)
{
    ret_code_t             err_code;
    ble_add_char_params_t  add_char_params;
    ble_add_descr_params_t add_descr_params;
    uint16_t               initial_button4_threshold;
    uint8_t                init_len;
    uint8_t                encoded_report_ref[BLE_SRV_ENCODED_REPORT_REF_LEN];

    // Add battery level characteristic
    initial_button4_threshold = p_button_threshold_service_init->initial_button4_threshold;

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = BUTTON_THRESHOLD_SERVICE_BUTTON4_THRESHOLD_CHAR_UUID;
    add_char_params.uuid_type         = m_button_threshold_service.uuid_type;
    add_char_params.max_len           = sizeof(uint16_t);
    add_char_params.init_len          = sizeof(uint16_t);
    add_char_params.p_init_value      = (uint8_t*)&initial_button4_threshold;
    add_char_params.char_props.notify = m_button_threshold_service.is_notification_supported;
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.write   = 1;
    add_char_params.cccd_write_access = SEC_OPEN;
    add_char_params.read_access       = p_button_threshold_service_init->bl_rd_sec;
    add_char_params.write_access = SEC_OPEN;

    err_code = characteristic_add(m_button_threshold_service.service_handle,
                                  &add_char_params,
                                  &(m_button_threshold_service.button4_threshold_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    if (p_button_threshold_service_init->p_report_ref != NULL)
    {
        // Add Report Reference descriptor
        init_len = ble_srv_report_ref_encode(encoded_report_ref, p_button_threshold_service_init->p_report_ref);

        memset(&add_descr_params, 0, sizeof(add_descr_params));
        add_descr_params.uuid        = BLE_UUID_REPORT_REF_DESCR;
        add_descr_params.read_access = p_button_threshold_service_init->bl_report_rd_sec;
        add_descr_params.write_access = SEC_OPEN;
        add_descr_params.init_len    = init_len;
        add_descr_params.max_len     = add_descr_params.init_len;
        add_descr_params.p_value     = encoded_report_ref;

        err_code = descriptor_add(m_button_threshold_service.button4_threshold_handles.value_handle,
                                  &add_descr_params,
                                  &m_button_threshold_service.report_ref_handle);
        return err_code;
    }
    else
    {
        m_button_threshold_service.report_ref_handle = BLE_GATT_HANDLE_INVALID;
    }

    return NRF_SUCCESS;
}




ret_code_t button_threshold_service_init()
{
    // Initialize Battery Service.
    button_threshold_service_init_t     button_threshold_service_init;
    memset(&button_threshold_service_init, 0, sizeof(button_threshold_service_init));

    // Here the sec level for the Battery Service can be changed/increased.
    button_threshold_service_init.bl_rd_sec        = SEC_OPEN;
    button_threshold_service_init.bl_cccd_wr_sec   = SEC_OPEN;
    button_threshold_service_init.bl_report_rd_sec = SEC_OPEN;

    button_threshold_service_init.evt_handler          = NULL;
    button_threshold_service_init.support_notification = true;
    button_threshold_service_init.p_report_ref         = NULL;
    button_threshold_service_init.initial_button1_threshold   = 0xffff;

    ret_code_t err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    m_button_threshold_service.evt_handler               = button_threshold_service_init.evt_handler;
    m_button_threshold_service.is_notification_supported = button_threshold_service_init.support_notification;
    m_button_threshold_service.threshold_last        = 0xFFFF;

    // Add service
    //BLE_UUID_BLE_ASSIGN(ble_uuid, BUTTON_THRESHOLD_SERVICE_SERVICE_UUID);

    ble_uuid128_t base_uuid = {BUTTON_THRESHOLD_SERVICE_UUID_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &m_button_threshold_service.uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = m_button_threshold_service.uuid_type;
    ble_uuid.uuid = BUTTON_THRESHOLD_SERVICE_SERVICE_UUID;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_button_threshold_service.service_handle);
    VERIFY_SUCCESS(err_code);

    // Add battery level characteristic
    err_code = button_threshold_service_button1_threshold_char_add(&button_threshold_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = button_threshold_service_button2_threshold_char_add(&button_threshold_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = button_threshold_service_button3_threshold_char_add(&button_threshold_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = button_threshold_service_button4_threshold_char_add(&button_threshold_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }


    return err_code;
}


/**@brief Function for sending notifications with the Battery Level characteristic.
 *
 * @param[in]   p_hvx_params Pointer to structure with notification data.
 * @param[in]   conn_handle  Connection handle.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t button_threshold_service_battery_notification_send(ble_gatts_hvx_params_t * const p_hvx_params,
                                            uint16_t                       conn_handle)
{
    ret_code_t err_code = sd_ble_gatts_hvx(conn_handle, p_hvx_params);
    if (err_code == NRF_SUCCESS)
    {
        NRF_LOG_INFO("Battery notification has been sent using conn_handle: 0x%04X", conn_handle);
    }
    else
    {
        NRF_LOG_DEBUG("Error: 0x%08X while sending notification with conn_handle: 0x%04X",
                      err_code,
                      conn_handle);
    }
    return err_code;
}



ret_code_t button_threshold_service_button1_threshold_update(uint16_t threshold, uint16_t conn_handle)
{
    ret_code_t         err_code = NRF_SUCCESS;
    ble_gatts_value_t  gatts_value;

    if (threshold != m_button_threshold_service.threshold_last)
    {
        // Initialize value struct.
        memset(&gatts_value, 0, sizeof(gatts_value));

        gatts_value.len     = sizeof(uint16_t);
        gatts_value.offset  = 0;
        gatts_value.p_value = (uint8_t*)&threshold;

        // Update database.
        err_code = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID,
                                          m_button_threshold_service.button1_threshold_handles.value_handle,
                                          &gatts_value);
        if (err_code == NRF_SUCCESS)
        {
            //NRF_LOG_INFO("Battery level has been updated: %d%%", battery_level)

            // Save new battery value.
            m_button_threshold_service.threshold_last = threshold;
        }
        else
        {
            NRF_LOG_DEBUG("Error during battery level update: 0x%08X", err_code)

            return err_code;
        }

        // Send value if connected and notifying.
        if (m_button_threshold_service.is_notification_supported)
        {
            ble_gatts_hvx_params_t hvx_params;

            memset(&hvx_params, 0, sizeof(hvx_params));

            hvx_params.handle = m_button_threshold_service.button1_threshold_handles.value_handle;
            hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
            hvx_params.offset = gatts_value.offset;
            hvx_params.p_len  = &gatts_value.len;
            hvx_params.p_data = gatts_value.p_value;

            if (conn_handle == BLE_CONN_HANDLE_ALL)
            {
                ble_conn_state_conn_handle_list_t conn_handles = ble_conn_state_conn_handles();

                // Try sending notifications to all valid connection handles.
                for (uint32_t i = 0; i < conn_handles.len; i++)
                {
                    if (ble_conn_state_status(conn_handles.conn_handles[i]) == BLE_CONN_STATUS_CONNECTED)
                    {
                        if (err_code == NRF_SUCCESS)
                        {
                            err_code = button_threshold_service_battery_notification_send(&hvx_params,
                                                                 conn_handles.conn_handles[i]);
                        }
                        else
                        {
                            // Preserve the first non-zero error code
                            UNUSED_RETURN_VALUE(button_threshold_service_battery_notification_send(&hvx_params,
                                                                          conn_handles.conn_handles[i]));
                        }
                    }
                }
            }
            else
            {
                err_code = button_threshold_service_battery_notification_send(&hvx_params, conn_handle);
            }
        }
        else
        {
            err_code = NRF_ERROR_INVALID_STATE;
        }
    }

    return err_code;
}

ret_code_t button_threshold_service_battery_lvl_on_reconnection_update(button_threshold_service_t * p_button_threshold_service,
                                                      uint16_t    conn_handle)
{
    if (p_button_threshold_service == NULL)
    {
        return NRF_ERROR_NULL;
    }

    ret_code_t err_code;

    if (p_button_threshold_service->is_notification_supported)
    {
        ble_gatts_hvx_params_t hvx_params;
        uint16_t               len = sizeof(uint8_t);

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_button_threshold_service->button1_threshold_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &len;
        hvx_params.p_data = (uint8_t*)&p_button_threshold_service->threshold_last;

        err_code = button_threshold_service_battery_notification_send(&hvx_params, conn_handle);

        return err_code;
    }

    return NRF_ERROR_INVALID_STATE;
}

void button_threshold_service_button1_threshold_received_callback(void (*func)(uint16_t threshold ))
{
    mButton1ThresholdReceivedCallback = func;
}

void button_threshold_service_button2_threshold_received_callback(void (*func)(uint16_t threshold ))
{
    mButton2ThresholdReceivedCallback = func;
}

void button_threshold_service_button3_threshold_received_callback(void (*func)(uint16_t threshold ))
{
    mButton3ThresholdReceivedCallback = func;
}

void button_threshold_service_button4_threshold_received_callback(void (*func)(uint16_t threshold ))
{
    mButton4ThresholdReceivedCallback = func;
}
