/** @file
 *
 * @defgroup Diagnostics Service
 * @{
 * @ingroup ble_sdk_srv
 * @brief Diagnostics Service module.
 *
 * @details This module implements the Diagnostics Service.
 *          During initialization it adds the Diagnostics Service characteristics
 *          to the BLE stack database. 
 *
 *
 */
#ifndef DIAGNOSTICS_SERVICE_H__
#define DIAGNOSTICS_SERVICE_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DIAGNOSTICS_SERVICE_UUID_BASE {0x1a, 0xc5, 0x1b, 0x8e, 0xa3, 0x06, 0x75, 0xb0, 0x45, 0x41, 0xf4, 0x67, 0xdc, 0x2b, 0xfc, 0x2b}

#define DIAGNOSTICS_SERVICE_SERVICE_UUID                    0x1400
#define DIAGNOSTICS_SERVICE_WEIGHT_FILTER_OUTPUT_COEFFICIENT_CHAR_UUID          0x1401


/**@brief Macro for defining a ble_bas instance.
 *
 * @param   _name  Name of the instance.
 * @hideinitializer
 */
#define DIAGNOSTICS_SERVICE_DEF(_name)                          \
    diagnostics_service_t _name;                         \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,             \
                         BLE_HRS_BLE_OBSERVER_PRIO, \
                         diagnostics_service_on_ble_evt,        \
                         &_name)



/**@brief Diagnostics Service event type. */
typedef enum
{
    DIAGNOSTICS_SERVICE_EVT_NOTIFICATION_ENABLED, /**< Diagnostics value notification enabled event. */
    DIAGNOSTICS_SERVICE_EVT_NOTIFICATION_DISABLED /**< Diagnostics value notification disabled event. */
} diagnostics_service_evt_type_t;

/**@brief Diagnostics Service event. */
typedef struct
{
    diagnostics_service_evt_type_t evt_type;    /**< Type of event. */
    uint16_t           conn_handle; /**< Connection handle. */
} diagnostics_service_evt_t;

// Forward declaration of the ble_bas_t type.
typedef struct diagnostics_service_s diagnostics_service_t;

/**@brief Diagnostics Service event handler type. */
typedef void (* diagnostics_service_evt_handler_t) (diagnostics_service_t * p_diagnosticsservice, diagnostics_service_evt_t * p_evt);

/**@brief Diagnostics Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
    diagnostics_service_evt_handler_t  evt_handler;                 /**< Event handler to be called for handling events in the Diagnostics Service. */
    bool                   support_notification;                    /**< TRUE if notification of Diagnostics Level measurement is supported. */
    ble_srv_report_ref_t * p_report_ref;                            /**< If not NULL, a Report Reference descriptor with the specified value will be added to the weight filter output coefficient characteristic */
    float               initial_filter_output_coefficient_value;    /**< Initial filter output coefficient */
    security_req_t         bl_rd_sec;                               /**< Security requirement for reading the BL characteristic value. */
    security_req_t         bl_cccd_wr_sec;                          /**< Security requirement for writing the BL characteristic CCCD. */
    security_req_t         bl_report_rd_sec;                        /**< Security requirement for reading the BL characteristic descriptor. */
} diagnostics_service_init_t;

/**@brief Diagnostics Service structure. This contains various status information for the service. */
struct diagnostics_service_s
{
    diagnostics_service_evt_handler_t   evt_handler;                            /**< Event handler to be called for handling events in the Diagnostics Service. */
    uint16_t                            service_handle;                         /**< Handle of Diagnostics Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t            weight_filter_output_coefficient_handles;/**< Handles related to the Diagnostics Level characteristic. */
    uint16_t                            report_ref_handle;                      /**< Handle of the Report Reference descriptor. */
    float                               weight_filter_output_coefficient_last;         /**< Last Diagnostics Level measurement passed to the Diagnostics Service. */
    bool                                is_notification_supported;              /**< TRUE if notification of Diagnostics Level is supported. */
    uint8_t                             uuid_type;
};


/**@brief Function for initializing the Diagnostics Service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
ret_code_t diagnostics_service_init();


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Diagnostics Service.
 *
 *
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 * @param[in]   p_context   Diagnostics Service structure.
 */
void diagnostics_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);


ret_code_t diagnostics_service_weight_filter_output_coefficient_update(float coefficient, uint16_t conn_handle);


/**@brief Function for sending the weight filter output coefficient when bonded client reconnects.
 *
 * @details The application calls this function, in the case of a reconnection of
 *          a bonded client if the value of the weight filter output coefficient has changed since
 *          its disconnection.
 *
 *
 * @param[in]   conn_handle    Connection handle.
 *
 * @return      NRF_SUCCESS on success,
 *              NRF_ERROR_INVALID_STATE when notification is not supported,
 *              otherwise an error code returned by @ref sd_ble_gatts_hvx.
 */
ret_code_t diagnostics_service_weight_filter_output_coefficient_on_reconnection_update(uint16_t    conn_handle);


void diagnostics_service_weight_filter_output_coefficient_received_callback(void (*func)(float coefficient ));


extern diagnostics_service_t m_diagnostics_service;

#ifdef __cplusplus
}
#endif

#endif // DIAGNOSTICS_SERVICE_H__

/** @} */
