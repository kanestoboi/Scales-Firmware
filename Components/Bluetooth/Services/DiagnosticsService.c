#include "sdk_common.h"
#include "DiagnosticsService.h"
#include <string.h>
#include "ble_srv_common.h"
#include "ble_conn_state.h"
#include "nrf_log.h"

void (*mWeightFilterInputCoefficientReceivedCallback)(float coefficient) = NULL;
void (*mWeightFilterOutputCoefficientReceivedCallback)(float coefficient) = NULL;


DIAGNOSTICS_SERVICE_DEF(m_diagnostics_service);

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_evt_t const * p_ble_evt)
{
    if (!m_diagnostics_service.is_notification_supported)
    {
        return;
    }

    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (    (p_evt_write->handle == m_diagnostics_service.weight_filter_output_coefficient_handles.cccd_handle) &&
            (p_evt_write->len == 4))
    {
        if (m_diagnostics_service.evt_handler == NULL)
        {
            return;
        }

        diagnostics_service_evt_t evt;

        if (ble_srv_is_notification_enabled(p_evt_write->data))
        {
            evt.evt_type = DIAGNOSTICS_SERVICE_EVT_NOTIFICATION_ENABLED;
        }
        else
        {
            evt.evt_type = DIAGNOSTICS_SERVICE_EVT_NOTIFICATION_DISABLED;
        }

        evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;

        // CCCD written, call application event handler.
        m_diagnostics_service.evt_handler(&m_diagnostics_service, &evt);
    }

    if (    (p_evt_write->handle == m_diagnostics_service.weight_filter_output_coefficient_handles.value_handle) &&
            (p_evt_write->len == 4)
       )
    {
        NRF_LOG_INFO("Value Received from weight filter output coefficient.");

        if (*mWeightFilterOutputCoefficientReceivedCallback != NULL)
        {
            float coefficient;
            memcpy(&coefficient, p_evt_write->data, sizeof(coefficient));
            (mWeightFilterOutputCoefficientReceivedCallback)(coefficient);
        }
        else
        {
            NRF_LOG_INFO("No weight filter output coefficient received callback set.");
        }
    }
}

void diagnostics_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            on_write(p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for adding the Weight Filter Output Coefficient characteristic.
 *
.
 * @param[in]   p_diagnostics_service_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t diagnostics_service_weight_filter_output_coefficient_char_add(const diagnostics_service_init_t * p_diagnostics_service_init)
{
    ret_code_t             err_code;
    ble_add_char_params_t  add_char_params;
    ble_add_descr_params_t add_descr_params;
    float                  initial_weight_filter_output_coefficient_threshold;
    uint8_t                init_len;
    uint8_t                encoded_report_ref[BLE_SRV_ENCODED_REPORT_REF_LEN];

    // Add weight filter output coefficient characteristic
    initial_weight_filter_output_coefficient_threshold = 0.5;

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = DIAGNOSTICS_SERVICE_WEIGHT_FILTER_OUTPUT_COEFFICIENT_CHAR_UUID;
    add_char_params.uuid_type         = m_diagnostics_service.uuid_type;
    add_char_params.max_len           = sizeof(float);
    add_char_params.init_len          = sizeof(float);
    add_char_params.p_init_value      = (uint8_t*)&initial_weight_filter_output_coefficient_threshold;
    add_char_params.char_props.notify = m_diagnostics_service.is_notification_supported;
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.write   = 1;
    add_char_params.cccd_write_access = SEC_OPEN;
    add_char_params.read_access       = p_diagnostics_service_init->bl_rd_sec;
    add_char_params.write_access = SEC_OPEN;

    err_code = characteristic_add(m_diagnostics_service.service_handle,
                                  &add_char_params,
                                  &(m_diagnostics_service.weight_filter_output_coefficient_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    if (p_diagnostics_service_init->p_report_ref != NULL)
    {
        // Add Report Reference descriptor
        init_len = ble_srv_report_ref_encode(encoded_report_ref, p_diagnostics_service_init->p_report_ref);

        memset(&add_descr_params, 0, sizeof(add_descr_params));
        add_descr_params.uuid        = BLE_UUID_REPORT_REF_DESCR;
        add_descr_params.read_access = p_diagnostics_service_init->bl_report_rd_sec;
        add_descr_params.write_access = SEC_OPEN;
        add_descr_params.init_len    = init_len;
        add_descr_params.max_len     = add_descr_params.init_len;
        add_descr_params.p_value     = encoded_report_ref;

        err_code = descriptor_add(m_diagnostics_service.weight_filter_output_coefficient_handles.value_handle,
                                  &add_descr_params,
                                  &m_diagnostics_service.report_ref_handle);
        return err_code;
    }
    else
    {
        m_diagnostics_service.report_ref_handle = BLE_GATT_HANDLE_INVALID;
    }

    return NRF_SUCCESS;
}

ret_code_t diagnostics_service_init()
{
    // Initialize Diagnostics Service.
    diagnostics_service_init_t     diagnostics_service_init;
    memset(&diagnostics_service_init, 0, sizeof(diagnostics_service_init));

    // Here the sec level for the Diagnostics Service can be changed/increased.
    diagnostics_service_init.bl_rd_sec        = SEC_OPEN;
    diagnostics_service_init.bl_cccd_wr_sec   = SEC_OPEN;
    diagnostics_service_init.bl_report_rd_sec = SEC_OPEN;

    diagnostics_service_init.evt_handler          = NULL;
    diagnostics_service_init.support_notification = true;
    diagnostics_service_init.p_report_ref         = NULL;
    diagnostics_service_init.initial_filter_output_coefficient_value   = 0.5;


    ret_code_t err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    m_diagnostics_service.evt_handler               = diagnostics_service_init.evt_handler;
    m_diagnostics_service.is_notification_supported = diagnostics_service_init.support_notification;
    m_diagnostics_service.weight_filter_output_coefficient_last        = 0.5;

    // Add service
    //BLE_UUID_BLE_ASSIGN(ble_uuid, DIAGNOSTICS_SERVICE_SERVICE_UUID);

    ble_uuid128_t base_uuid = {DIAGNOSTICS_SERVICE_UUID_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &m_diagnostics_service.uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = m_diagnostics_service.uuid_type;
    ble_uuid.uuid = DIAGNOSTICS_SERVICE_SERVICE_UUID;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_diagnostics_service.service_handle);
    VERIFY_SUCCESS(err_code);

    // Add weight filter output coefficient characteristic
    err_code = diagnostics_service_weight_filter_output_coefficient_char_add(&diagnostics_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return err_code;
}


/**@brief Function for sending notifications with the Diagnostics Service.
 *
 * @param[in]   p_hvx_params Pointer to structure with notification data.
 * @param[in]   conn_handle  Connection handle.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t diagnostics_service_notification_send(ble_gatts_hvx_params_t * const p_hvx_params, uint16_t conn_handle)
{
    ret_code_t err_code = sd_ble_gatts_hvx(conn_handle, p_hvx_params);
    if (err_code == NRF_SUCCESS)
    {
        NRF_LOG_INFO("Diagnostics notification has been sent using conn_handle: 0x%04X", conn_handle);
    }
    else
    {
        NRF_LOG_DEBUG("Error: 0x%08X while sending notification with conn_handle: 0x%04X",
                      err_code,
                      conn_handle);
    }
    return err_code;
}

ret_code_t diagnostics_service_weight_filter_output_coefficient_update(float coefficient, uint16_t conn_handle)
{
    ret_code_t         err_code = NRF_SUCCESS;
    ble_gatts_value_t  gatts_value;

    if (coefficient != m_diagnostics_service.weight_filter_output_coefficient_last)
    {
        // Initialize value struct.
        memset(&gatts_value, 0, sizeof(gatts_value));

        gatts_value.len     = sizeof(uint16_t);
        gatts_value.offset  = 0;
        gatts_value.p_value = (uint8_t*)&coefficient;

        // Update database.
        err_code = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID,
                                          m_diagnostics_service.weight_filter_output_coefficient_handles.value_handle,
                                          &gatts_value);
        if (err_code == NRF_SUCCESS)
        {
            // Save new output coefficient value.
            m_diagnostics_service.weight_filter_output_coefficient_last = coefficient;
        }
        else
        {
            NRF_LOG_DEBUG("Error during weight filter output coefficient update: 0x%08X", err_code)

            return err_code;
        }

        // Send value if connected and notifying.
        if (m_diagnostics_service.is_notification_supported)
        {
            ble_gatts_hvx_params_t hvx_params;

            memset(&hvx_params, 0, sizeof(hvx_params));

            hvx_params.handle = m_diagnostics_service.weight_filter_output_coefficient_handles.value_handle;
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
                            err_code = diagnostics_service_notification_send(&hvx_params, conn_handles.conn_handles[i]);
                        }
                        else
                        {
                            // Preserve the first non-zero error code
                            UNUSED_RETURN_VALUE(diagnostics_service_notification_send(&hvx_params, conn_handles.conn_handles[i]));
                        }
                    }
                }
            }
            else
            {
                err_code = diagnostics_service_notification_send(&hvx_params, conn_handle);
            }
        }
        else
        {
            err_code = NRF_ERROR_INVALID_STATE;
        }
    }

    return err_code;
}

ret_code_t diagnostics_service_weight_filter_output_coefficient_on_reconnection_update(uint16_t    conn_handle)
{
    ret_code_t err_code;

    if (m_diagnostics_service.is_notification_supported)
    {
        ble_gatts_hvx_params_t hvx_params;
        uint16_t               len = sizeof(float);

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = m_diagnostics_service.weight_filter_output_coefficient_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &len;
        hvx_params.p_data = (uint8_t*)&m_diagnostics_service.weight_filter_output_coefficient_last;

        err_code = diagnostics_service_notification_send(&hvx_params, conn_handle);

        return err_code;
    }

    return NRF_ERROR_INVALID_STATE;
}

void diagnostics_service_weight_filter_output_coefficient_received_callback(void (*func)(float coefficient ))
{
    mWeightFilterOutputCoefficientReceivedCallback = func;
}
