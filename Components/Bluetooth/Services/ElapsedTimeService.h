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

#ifndef ELAPSED_TIME_SERVICE_H
#define ELAPSED_TIME_SERVICE_H

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#define ELAPSED_TIME_SERVICE_UUID_BASE 0x183F
#define CURRENT_ELAPSED_TIME_UUID      0x2Bf2


/**@brief   Macro for defining a ble_elapsed_time instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_ELAPSED_TIME_DEF(_name)                          \
ble_elapsed_time_service_t _name;                     \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
                     BLE_HRS_BLE_OBSERVER_PRIO,               \
                     ble_elapsed_time_on_ble_evt, &_name)

// Forward declaration of the ble_accerometer_service_t type.
typedef struct ble_elapsed_time_service_s ble_elapsed_time_service_t;

typedef enum
{
    BLE_ELAPSED_TIME_EVT_NOTIFICATION_ENABLED,                             /**< Custom value notification enabled event. */
    BLE_ELAPSED_TIME_EVT_NOTIFICATION_DISABLED,                            /**< Custom value notification disabled event. */
    BLE_ELAPSED_TIME_EVT_DISCONNECTED,
    BLE_ELAPSED_TIME_EVT_CONNECTED
} ble_elapsed_time_evt_type_t;

/**@brief Custom Service event. */
typedef struct
{
    ble_elapsed_time_evt_type_t evt_type;                                  /**< Type of event. */
} ble_elapsed_time_evt_t;

/**@brief Custom Service event handler type. */
typedef void (*ble_elapsed_time_evt_handler_t) (ble_elapsed_time_service_t * p_elapsed_time_service, ble_elapsed_time_evt_t * p_evt);

/**@brief Custom Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
    ble_elapsed_time_evt_handler_t         evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */
    uint8_t                       initial_custom_value;           /**< Initial custom value */
    ble_srv_cccd_security_mode_t  elapsed_time_attr_md;     /**< Initial security level for Custom characteristics attribute */
} ble_elapsed_time_service_init_t;

/**@brief elapsed_time Service structure. This contains various status information for the service. */
struct ble_elapsed_time_service_s
{
    ble_elapsed_time_evt_handler_t         evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */
    uint16_t                        service_handle;                 /**< Handle of elapsed_time Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t        elapsed_time_handles;      /**< Handles related to the elapsed_time Value characteristic. */
    uint16_t                      conn_handle;                    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    uint8_t                       uuid_type; 
};

typedef struct
{
    uint8_t flags;
    uint8_t elapsed_time_lsb;
    uint8_t elapsed_time_5sb;
    uint8_t elapsed_time_4sb;
    uint8_t elapsed_time_3sb;
    uint8_t elapsed_time_2sb;
    uint8_t elapsed_time_msb;
    uint8_t timeime_sync_source_type;
    int8_t tz_dst_offset;
} ble_elapsed_time_t;


/**@brief Function for initializing the Custom Service.
 *
 * @param[out]  p_cus       Custom Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_cus_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_elapsed_time_service_init();

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Battery Service.
 *
 * @note 
 *
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 * @param[in]   p_context  Custom Service structure.
 */
void ble_elapsed_time_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context);

static void ble_elapsed_time_on_connect(ble_elapsed_time_service_t * p_elapsed_time_service, ble_evt_t const * p_ble_evt);

static void ble_elapsed_time_on_disconnect(ble_elapsed_time_service_t * p_elapsed_time_service, ble_evt_t const * p_ble_evt);

static void ble_elapsed_time_on_write(ble_elapsed_time_service_t * p_elapsed_time_service, ble_evt_t const * p_ble_evt);

uint32_t ble_elapsed_time_service_elapsed_time_update(ble_elapsed_time_t elapsed_time);

void ble_elapsed_time_on_elapsed_time_evt(ble_elapsed_time_service_t * p_elapsed_time_service, ble_elapsed_time_evt_t * p_evt);

extern ble_elapsed_time_service_t m_elapsed_time_service;

#endif /* ELAPSED_TIME_SERVICE_H */