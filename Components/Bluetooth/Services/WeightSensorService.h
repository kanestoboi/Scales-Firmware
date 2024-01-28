/*
 * The MIT License (MIT)
 * Copyright (c) 2018 Novel Bits
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef WEIGHT_SENSOR_SERVICE_H
#define WEIGHT_SENSOR_SERVICE_H

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#define WEIGHT_SENSOR_SERVICE_UUID_BASE {0x86, 0xfe, 0x92, 0x36, 0x4B, 0x4e, 0x67, 0xAF, 0xBE, 0x41, 0x8B, 0x81, 0xE4, 0x46, 0x89, 0xCE}

#define WEIGHT_SENSOR_SERVICE_UUID                      0x1400
#define WEIGHT_SENSOR_WEIGHT_VALUE_CHAR_UUID            0x1401
#define WEIGHT_SENSOR_TARE_CHAR_UUID                    0x1402
#define WEIGHT_SENSOR_CALIBRATION_CHAR_UUID             0x1403
#define WEIGHT_SENSOR_COFFEE_TO_WATER_RATIO_CHAR_UUID   0x1404
#define WEIGHT_SENSOR_WEIGH_MODE_CHAR_UUID              0x1405


/**@brief   Macro for defining a ble_weight_sensor instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_WEIGHT_SENSOR_DEF(_name)                          \
ble_weight_sensor_service_t _name;                     \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
                     BLE_HRS_BLE_OBSERVER_PRIO,               \
                     ble_weight_sensor_on_ble_evt, &_name)

// Forward declaration of the ble_accerometer_service_t type.
typedef struct ble_accerometer_service_s ble_weight_sensor_service_t;

typedef enum
{
    BLE_WEIGHT_SENSOR_EVT_NOTIFICATION_ENABLED,                             /**< Custom value notification enabled event. */
    BLE_WEIGHT_SENSOR_EVT_NOTIFICATION_DISABLED,                            /**< Custom value notification disabled event. */
    BLE_WEIGHT_SENSOR_EVT_DISCONNECTED,
    BLE_WEIGHT_SENSOR_EVT_CONNECTED
} ble_weight_sensor_evt_type_t;

typedef enum
{
    BLE_WEIGHT_SENSOR_READ_VALUE_REQUEST,
    BLE_WEIGHT_SENSOR_WRITE_VALUE_REQUEST,
} ble_weight_sensor_request_type_t;

/**@brief Custom Service event. */
typedef struct
{
    ble_weight_sensor_evt_type_t evt_type;                                  /**< Type of event. */
} ble_weight_sensor_evt_t;

/**@brief Custom Service event handler type. */
typedef void (*ble_weight_sensor_evt_handler_t) (ble_weight_sensor_service_t * p_weight_sensor_service, ble_weight_sensor_evt_t * p_evt);

/**@brief Custom Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
    ble_weight_sensor_evt_handler_t         evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */
    uint8_t                       initial_custom_value;           /**< Initial custom value */
    ble_srv_cccd_security_mode_t  weight_sensor_sensor_attr_md;     /**< Initial security level for Custom characteristics attribute */
} ble_weight_sensor_service_init_t;

/**@brief weight_sensor Service structure. This contains various status information for the service. */
struct ble_accerometer_service_s
{
    ble_weight_sensor_evt_handler_t         evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */
    uint16_t                        service_handle;                 /**< Handle of weight_sensor Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t        weight_sensor_sensor_data_handles;      /**< Handles related to the weight_sensor Value characteristic. */
    ble_gatts_char_handles_t        weight_sensor_tare_handles;
    ble_gatts_char_handles_t        weight_sensor_calibration_handles;
    ble_gatts_char_handles_t        weight_sensor_coffee_to_water_ratio_handles;
    ble_gatts_char_handles_t        weight_sensor_weigh_mode_handles;
    uint16_t                      conn_handle;                    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    uint8_t                       uuid_type; 
};


/**@brief Function for initializing the Custom Service.
 *
 * @param[out]  p_cus       Custom Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_cus_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_weight_sensor_service_init();

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Battery Service.
 *
 * @note 
 *
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 * @param[in]   p_context  Custom Service structure.
 */
void ble_weight_sensor_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context);

static void on_connect(ble_weight_sensor_service_t * p_weight_sensor_service, ble_evt_t const * p_ble_evt);

static void on_disconnect(ble_weight_sensor_service_t * p_weight_sensor_service, ble_evt_t const * p_ble_evt);

static void on_write(ble_weight_sensor_service_t * p_weight_sensor_service, ble_evt_t const * p_ble_evt);

/**@brief Function for updating the custom value.
 *
 * @details The application calls this function when the cutom value should be updated. If
 *          notification has been enabled, the custom value characteristic is sent to the client.
 *
 * @note 
 *       
 * @param[in]   p_cus          Custom Service structure.
 * @param[in]   Custom value 
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_weight_sensor_service_sensor_data_update(uint8_t *custom_value, uint8_t custom_value_length);

uint32_t ble_weight_sensor_service_tare_update(uint8_t *custom_value, uint8_t custom_value_length);

uint32_t ble_weight_sensor_service_coffee_to_water_ratio_update(uint8_t *custom_value, uint8_t custom_value_length);

uint32_t ble_weight_sensor_service_weigh_mode_update(uint8_t *custom_value, uint8_t custom_value_length);

void ble_weight_sensor_on_weight_sensor_evt(ble_weight_sensor_service_t * p_weight_sensor_service, ble_weight_sensor_evt_t * p_evt);

uint32_t ble_weight_sensor_service_sensor_data_set(uint8_t *custom_value, uint8_t custom_value_length);

void ble_weight_sensor_set_tare_callback(void (*func)(void));

void ble_weight_sensor_set_calibration_callback(void (*func)(void));

void ble_weight_sensor_set_coffee_to_water_ratio_callback(void (*func)(uint16_t requestValue ));

void ble_weight_sensor_set_weigh_mode_callback(void (*func)(uint8_t requestValue));

extern ble_weight_sensor_service_t m_weight_sensor;

#endif /* WEIGHT_SENSOR_SERVICE_H */