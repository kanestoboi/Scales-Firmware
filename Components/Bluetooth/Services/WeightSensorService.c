#include "sdk_common.h"
#include "ble_srv_common.h"
#include "WeightSensorService.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"
#include "math.h"
#include "nrf_log_ctrl.h"

void (*mTareCallback)(void) = NULL;
void (*mCalibrationCallback)(void) = NULL;
void (*mCoffeeToWaterRatioCallback)(uint16_t requestValue) = NULL;
void (*mWeighModeCallback)(uint8_t requestValue) = NULL;

static uint32_t weight_sensor_weight_value_char_add(ble_weight_sensor_service_t * p_weight_sensor_service, const ble_weight_sensor_service_init_t * p_ble_weight_sensor_service_init);
static uint32_t weight_sensor_tare_char_add(ble_weight_sensor_service_t * p_weight_sensor_service, const ble_weight_sensor_service_init_t * p_ble_weight_sensor_service_init);
static uint32_t weight_sensor_calibration_char_add(ble_weight_sensor_service_t * p_weight_sensor_service, const ble_weight_sensor_service_init_t * p_ble_weight_sensor_service_init);
static uint32_t weight_sensor_coffee_to_water_ratio_char_add(ble_weight_sensor_service_t * p_weight_sensor_service, const ble_weight_sensor_service_init_t * p_ble_weight_sensor_service_init);
static uint32_t weight_sensor_weigh_mode_char_add(ble_weight_sensor_service_t * p_weight_sensor_service, const ble_weight_sensor_service_init_t * p_ble_weight_sensor_service_init);

BLE_WEIGHT_SENSOR_DEF(m_weight_sensor);

uint32_t ble_weight_sensor_service_init()
{
    ble_weight_sensor_service_init_t weight_sensor_service_init;
    
     // Initialize custom Service init structure to zero.
    memset(&weight_sensor_service_init, 0, sizeof(weight_sensor_service_init));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&weight_sensor_service_init.weight_sensor_sensor_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&weight_sensor_service_init.weight_sensor_sensor_attr_md.write_perm);

    // Set the weight sensor event handler
    weight_sensor_service_init.evt_handler = ble_weight_sensor_on_weight_sensor_evt;

    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    m_weight_sensor.conn_handle = BLE_CONN_HANDLE_INVALID;

    ble_uuid128_t base_uuid = {WEIGHT_SENSOR_SERVICE_UUID_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &m_weight_sensor.uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = m_weight_sensor.uuid_type;
    ble_uuid.uuid = WEIGHT_SENSOR_SERVICE_UUID;

    // Add the Custom Service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_weight_sensor.service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Initialize service structure
    m_weight_sensor.evt_handler           = weight_sensor_service_init.evt_handler;
    m_weight_sensor.conn_handle           = BLE_CONN_HANDLE_INVALID;

    /*  From here all the characteristics are added to the service in the order
     *  they will be discovered by the central.
     */

    err_code = weight_sensor_weight_value_char_add(&m_weight_sensor, &weight_sensor_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = weight_sensor_tare_char_add(&m_weight_sensor, &weight_sensor_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = weight_sensor_calibration_char_add(&m_weight_sensor, &weight_sensor_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = weight_sensor_coffee_to_water_ratio_char_add(&m_weight_sensor, &weight_sensor_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = weight_sensor_weigh_mode_char_add(&m_weight_sensor, &weight_sensor_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}    

/**@brief Function for adding the acceleration sensor data characteristic.
 *
 * @param[in]   p_cus        Custom Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t weight_sensor_weight_value_char_add(ble_weight_sensor_service_t * p_weight_sensor_service, const ble_weight_sensor_service_init_t * p_ble_weight_sensor_service_init)
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

    attr_md.read_perm  = p_ble_weight_sensor_service_init->weight_sensor_sensor_attr_md.read_perm;
    attr_md.write_perm = p_ble_weight_sensor_service_init->weight_sensor_sensor_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    ble_uuid.type = m_weight_sensor.uuid_type;

    ble_uuid.uuid = WEIGHT_SENSOR_WEIGHT_VALUE_CHAR_UUID;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(float);
    attr_char_value.init_offs = 0;

   
    attr_char_value.max_len   = sizeof(float);
    
    err_code = sd_ble_gatts_characteristic_add(m_weight_sensor.service_handle, &char_md,
                                               &attr_char_value,
                                               &m_weight_sensor.weight_sensor_sensor_data_handles);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}


/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_cus        Custom Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t weight_sensor_tare_char_add(ble_weight_sensor_service_t * p_weight_sensor_service, const ble_weight_sensor_service_init_t * p_ble_weight_sensor_service_init)
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
    char_md.char_props.write  = 1;
    char_md.char_props.notify = 0; 
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md; 
    char_md.p_sccd_md         = NULL;

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm  = p_ble_weight_sensor_service_init->weight_sensor_sensor_attr_md.read_perm;
    attr_md.write_perm = p_ble_weight_sensor_service_init->weight_sensor_sensor_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    ble_uuid.type = m_weight_sensor.uuid_type;

    ble_uuid.uuid = WEIGHT_SENSOR_TARE_CHAR_UUID;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 1*sizeof(uint8_t);
    attr_char_value.init_offs = 0;

    uint8_t resetValue = 0;        
    attr_char_value.p_value   = (uint8_t*)&resetValue; // Pointer to the initial value

    attr_char_value.max_len   = 1*sizeof(uint8_t);

    err_code = sd_ble_gatts_characteristic_add(m_weight_sensor.service_handle, &char_md,
                                               &attr_char_value,
                                               &m_weight_sensor.weight_sensor_tare_handles);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;

}


/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_cus        Custom Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t weight_sensor_calibration_char_add(ble_weight_sensor_service_t * p_weight_sensor_service, const ble_weight_sensor_service_init_t * p_ble_weight_sensor_service_init)
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
    char_md.char_props.write  = 1;
    char_md.char_props.notify = 0; 
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md; 
    char_md.p_sccd_md         = NULL;

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm  = p_ble_weight_sensor_service_init->weight_sensor_sensor_attr_md.read_perm;
    attr_md.write_perm = p_ble_weight_sensor_service_init->weight_sensor_sensor_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    ble_uuid.type = m_weight_sensor.uuid_type;

    ble_uuid.uuid = WEIGHT_SENSOR_CALIBRATION_CHAR_UUID;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 1*sizeof(uint8_t);
    attr_char_value.init_offs = 0;

    uint8_t resetValue = 0;        
    attr_char_value.p_value   = (uint8_t*)&resetValue; // Pointer to the initial value

    attr_char_value.max_len   = 1*sizeof(uint8_t);

    err_code = sd_ble_gatts_characteristic_add(m_weight_sensor.service_handle, &char_md,
                                               &attr_char_value,
                                               &m_weight_sensor.weight_sensor_calibration_handles);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;

}


/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_cus        Custom Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t weight_sensor_coffee_to_water_ratio_char_add(ble_weight_sensor_service_t * p_weight_sensor_service, const ble_weight_sensor_service_init_t * p_ble_weight_sensor_service_init)
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
    char_md.char_props.write  = 1;
    char_md.char_props.notify = 0; 
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md; 
    char_md.p_sccd_md         = NULL;

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm  = p_ble_weight_sensor_service_init->weight_sensor_sensor_attr_md.read_perm;
    attr_md.write_perm = p_ble_weight_sensor_service_init->weight_sensor_sensor_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    ble_uuid.type = m_weight_sensor.uuid_type;

    ble_uuid.uuid = WEIGHT_SENSOR_COFFEE_TO_WATER_RATIO_CHAR_UUID;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 1*sizeof(uint16_t);
    attr_char_value.init_offs = 0;

    uint16_t resetValue = 0x1001;   // 1/16 with first byte numerator, second byte the denominator. Value here is little endian        
    attr_char_value.p_value   = (uint8_t*)&resetValue; // Pointer to the initial value

    attr_char_value.max_len   = 1*sizeof(uint16_t);

    err_code = sd_ble_gatts_characteristic_add(m_weight_sensor.service_handle, &char_md,
                                               &attr_char_value,
                                               &m_weight_sensor.weight_sensor_coffee_to_water_ratio_handles);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;

}


/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_cus        Custom Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t weight_sensor_weigh_mode_char_add(ble_weight_sensor_service_t * p_weight_sensor_service, const ble_weight_sensor_service_init_t * p_ble_weight_sensor_service_init)
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
    char_md.char_props.write  = 1;
    char_md.char_props.notify = 0; 
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md; 
    char_md.p_sccd_md         = NULL;

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm  = p_ble_weight_sensor_service_init->weight_sensor_sensor_attr_md.read_perm;
    attr_md.write_perm = p_ble_weight_sensor_service_init->weight_sensor_sensor_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    ble_uuid.type = m_weight_sensor.uuid_type;

    ble_uuid.uuid = WEIGHT_SENSOR_WEIGH_MODE_CHAR_UUID;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 1*sizeof(uint8_t);
    attr_char_value.init_offs = 0;

    uint8_t resetValue = 0x00;        
    attr_char_value.p_value   = (uint8_t*)&resetValue; // Pointer to the initial value

    attr_char_value.max_len   = 1*sizeof(uint8_t);

    err_code = sd_ble_gatts_characteristic_add(m_weight_sensor.service_handle, &char_md,
                                               &attr_char_value,
                                               &m_weight_sensor.weight_sensor_weigh_mode_handles);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;

}

void ble_weight_sensor_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_weight_sensor_service_t * p_weight_sensor_service = (ble_weight_sensor_service_t *) p_context;
    
    if (p_weight_sensor_service == NULL || p_ble_evt == NULL)
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            ble_weight_sensor_on_connect(p_weight_sensor_service, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            ble_weight_sensor_on_disconnect(p_weight_sensor_service, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            ble_weight_sensor_on_write(p_weight_sensor_service, p_ble_evt);
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
static void ble_weight_sensor_on_connect(ble_weight_sensor_service_t * p_weight_sensor_service, ble_evt_t const * p_ble_evt)
{
    m_weight_sensor.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

    ble_weight_sensor_evt_t evt;

    evt.evt_type = BLE_WEIGHT_SENSOR_EVT_CONNECTED;
    
    // check if the weight_sensor service ble event handler has been initialised before calling it
    if (m_weight_sensor.evt_handler == NULL)
    {
        return;
    }

    m_weight_sensor.evt_handler(p_weight_sensor_service, &evt);
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void ble_weight_sensor_on_disconnect(ble_weight_sensor_service_t * p_weight_sensor_service, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    m_weight_sensor.conn_handle = BLE_CONN_HANDLE_INVALID;
}

/* This code belongs in ble_cus.c*/

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void ble_weight_sensor_on_write(ble_weight_sensor_service_t * p_weight_sensor_service, ble_evt_t const * p_ble_evt)
{
   const ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    
    // Check if the handle passed with the event matches the Custom Value Characteristic handle.
    if (p_evt_write->handle == m_weight_sensor.weight_sensor_sensor_data_handles.value_handle)
    {
        // Put specific task here. 
        NRF_LOG_INFO("Message Received.");
        NRF_LOG_FLUSH();
    }

    // Check if the Custom value CCCD is written to and that the value is the appropriate length, i.e 2 bytes.
    if ((p_evt_write->handle == m_weight_sensor.weight_sensor_sensor_data_handles.cccd_handle)
        && (p_evt_write->len == 2)
       )
    {
        // CCCD written, call application event handler
        if (m_weight_sensor.evt_handler != NULL)
        {
            ble_weight_sensor_evt_t evt;

            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_WEIGHT_SENSOR_EVT_NOTIFICATION_ENABLED;
            }
            else
            {
                evt.evt_type = BLE_WEIGHT_SENSOR_EVT_NOTIFICATION_DISABLED;
            }
            // Call the application event handler.
            m_weight_sensor.evt_handler(p_weight_sensor_service, &evt);
        }
    }

    // Check if the handle passed with the event matches the Orientation Characteristic handle.
    if ((p_evt_write->handle == m_weight_sensor.weight_sensor_tare_handles.value_handle)
        && (p_evt_write->len == 1)
       )
    {
        switch (p_evt_write->data[0])
        {
        case 1:
        {
            NRF_LOG_INFO("Message Received from Tare.");

            if (mTareCallback != NULL)
            {
                mCalibrationCallback();
            }
            else
            {
                NRF_LOG_INFO("No tare callback registered");
            }
            
            NRF_LOG_FLUSH();
            
            break;
        }
        
        default:
            break;
        }
        

        uint8_t resetValue = 0;
        ble_weight_sensor_service_tare_update(&resetValue, 1);        
    }

    if ((p_evt_write->handle == m_weight_sensor.weight_sensor_calibration_handles.value_handle)
        && (p_evt_write->len == 1)
       )
    {
        switch (p_evt_write->data[0])
        {
        case 1:
        {
            NRF_LOG_INFO("Message Received from Calibration 1.");

            if (mCalibrationCallback != NULL)
            {
                mCalibrationCallback();
            }
            else
            {
                NRF_LOG_INFO("No Calibration callback registered");
            }
            
            NRF_LOG_FLUSH();
            break;
        }
        case 2:
        {
            NRF_LOG_INFO("Message Received from Calibration 2.");
            NRF_LOG_FLUSH();
            
            break;
        }


        
        default:
            break;
        }

        uint8_t resetValue = 0;
        ble_weight_sensor_service_tare_update(&resetValue, 1);        
    }

    if ((p_evt_write->handle == m_weight_sensor.weight_sensor_coffee_to_water_ratio_handles.value_handle)
        && (p_evt_write->len == 2)
       )
    {
        NRF_LOG_INFO("Value Received from coffee to water ratio.");

        if (*mCoffeeToWaterRatioCallback != NULL)
        {
            (mCoffeeToWaterRatioCallback)((uint16_t)(uint16_t)p_evt_write->data[1] << 8 | (uint16_t)p_evt_write->data[0]);
        }
        else
        {
            NRF_LOG_INFO("No coffee to water ratio callback set.");
        }

        NRF_LOG_FLUSH();
    }

    if ((p_evt_write->handle == m_weight_sensor.weight_sensor_weigh_mode_handles.value_handle)
        && (p_evt_write->len == 1)
       )
    {
        NRF_LOG_INFO("Value Received from weigh mode.");

        if (*mWeighModeCallback != NULL)
        {
            (mWeighModeCallback)(p_evt_write->data[0]);
        }
        else
        {
            NRF_LOG_INFO("No weigh mode callback set.");
        }

        NRF_LOG_FLUSH();
    }
};

uint32_t ble_weight_sensor_service_sensor_data_update(uint8_t *custom_value, uint8_t custom_value_length)
{
    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = custom_value_length*sizeof(uint8_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = custom_value;

    // Update database.
    err_code= sd_ble_gatts_value_set(m_weight_sensor.conn_handle,
                                        m_weight_sensor.weight_sensor_sensor_data_handles.value_handle,
                                        &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Send value if connected and notifying.
    if ((m_weight_sensor.conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = m_weight_sensor.weight_sensor_sensor_data_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;

        err_code = sd_ble_gatts_hvx(m_weight_sensor.conn_handle, &hvx_params);
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}

uint32_t ble_weight_sensor_service_tare_update(uint8_t *custom_value, uint8_t custom_value_length)
{
    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = custom_value_length*sizeof(uint8_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = custom_value;

    // Update database.
    err_code= sd_ble_gatts_value_set(m_weight_sensor.conn_handle,
                                        m_weight_sensor.weight_sensor_tare_handles.value_handle,
                                        &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Send value if connected and notifying.
    if ((m_weight_sensor.conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = m_weight_sensor.weight_sensor_tare_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;

        err_code = sd_ble_gatts_hvx(m_weight_sensor.conn_handle, &hvx_params);
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}

uint32_t ble_weight_sensor_service_coffee_to_water_ratio_update(uint8_t *custom_value, uint8_t custom_value_length)
{
    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = custom_value_length*sizeof(uint8_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = custom_value;

    // Update database.
    err_code= sd_ble_gatts_value_set(m_weight_sensor.conn_handle,
                                        m_weight_sensor.weight_sensor_coffee_to_water_ratio_handles.value_handle,
                                        &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Send value if connected and notifying.
    if ((m_weight_sensor.conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = m_weight_sensor.weight_sensor_coffee_to_water_ratio_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;

        err_code = sd_ble_gatts_hvx(m_weight_sensor.conn_handle, &hvx_params);
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}

uint32_t ble_weight_sensor_service_weigh_mode_update(uint8_t *custom_value, uint8_t custom_value_length)
{
    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = custom_value_length*sizeof(uint8_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = custom_value;

    // Update database.
    err_code= sd_ble_gatts_value_set(m_weight_sensor.conn_handle,
                                        m_weight_sensor.weight_sensor_weigh_mode_handles.value_handle,
                                        &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Send value if connected and notifying.
    if ((m_weight_sensor.conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = m_weight_sensor.weight_sensor_weigh_mode_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;

        err_code = sd_ble_gatts_hvx(m_weight_sensor.conn_handle, &hvx_params);
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
 * @param[in]   p_weight_sensor_service  Accelerometer Service structure.
 * @param[in]   p_evt          Event received from the Accelerometer Service.
 *
 */
void ble_weight_sensor_on_weight_sensor_evt(ble_weight_sensor_service_t * p_weight_sensor_service, ble_weight_sensor_evt_t * p_evt)
{
    ret_code_t err_code;
    switch(p_evt->evt_type)
    {
        case BLE_WEIGHT_SENSOR_EVT_NOTIFICATION_ENABLED:
            break;

        case BLE_WEIGHT_SENSOR_EVT_NOTIFICATION_DISABLED:      
            break;

        case BLE_WEIGHT_SENSOR_EVT_CONNECTED:
            break;

        case BLE_WEIGHT_SENSOR_EVT_DISCONNECTED:
              break;

        default:
              // No implementation needed.
              break;
    }
}

void ble_weight_sensor_set_tare_callback(void (*func)(void))
{
    mTareCallback = func;
}

void ble_weight_sensor_set_calibration_callback(void (*func)(void))
{
    mCalibrationCallback = func;
}

void ble_weight_sensor_set_coffee_to_water_ratio_callback(void (*func)(uint16_t requestValue ))
{
    mCoffeeToWaterRatioCallback = func;
}

void ble_weight_sensor_set_weigh_mode_callback(void (*func)(uint8_t requestValue))
{
    mWeighModeCallback = func;
}