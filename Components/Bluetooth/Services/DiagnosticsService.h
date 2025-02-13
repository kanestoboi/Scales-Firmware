/** @file
 *
 * @defgroup ble_bas Battery Service
 * @{
 * @ingroup ble_sdk_srv
 * @brief Battery Service module.
 *
 * @details This module implements the Battery Service with the Battery Level characteristic.
 *          During initialization it adds the Battery Service and Battery Level characteristic
 *          to the BLE stack database. Optionally it can also add a Report Reference descriptor
 *          to the Battery Level characteristic (used when including the Battery Service in
 *          the HID service).
 *
 *          If specified, the module will support notification of the Battery Level characteristic
 *          through the ble_bas_battery_level_update() function.
 *          If an event handler is supplied by the application, the Battery Service will
 *          generate Battery Service events to the application.
 *
 * @note    The application must register this module as BLE event observer using the
 *          NRF_SDH_BLE_OBSERVER macro. Example:
 *          @code
 *              ble_bas_t instance;
 *              NRF_SDH_BLE_OBSERVER(anything, BLE_BAS_BLE_OBSERVER_PRIO,
 *                                   ble_bas_on_ble_evt, &instance);
 *          @endcode
 *
 * @note Attention!
 *  To maintain compliance with Nordic Semiconductor ASA Bluetooth profile
 *  qualification listings, this section of source code must not be modified.
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



/**@brief Battery Service event type. */
typedef enum
{
    DIAGNOSTICS_SERVICE_EVT_NOTIFICATION_ENABLED, /**< Battery value notification enabled event. */
    DIAGNOSTICS_SERVICE_EVT_NOTIFICATION_DISABLED /**< Battery value notification disabled event. */
} diagnostics_service_evt_type_t;

/**@brief Battery Service event. */
typedef struct
{
    diagnostics_service_evt_type_t evt_type;    /**< Type of event. */
    uint16_t           conn_handle; /**< Connection handle. */
} diagnostics_service_evt_t;

// Forward declaration of the ble_bas_t type.
typedef struct diagnostics_service_s diagnostics_service_t;

/**@brief Battery Service event handler type. */
typedef void (* diagnostics_service_evt_handler_t) (diagnostics_service_t * p_diagnosticsservice, diagnostics_service_evt_t * p_evt);

/**@brief Battery Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
    diagnostics_service_evt_handler_t  evt_handler;                    /**< Event handler to be called for handling events in the Battery Service. */
    bool                   support_notification;           /**< TRUE if notification of Battery Level measurement is supported. */
    ble_srv_report_ref_t * p_report_ref;                   /**< If not NULL, a Report Reference descriptor with the specified value will be added to the Battery Level characteristic */
    float               initial_filter_output_coefficient_value;             /**< Initial battery level */
    security_req_t         bl_rd_sec;                      /**< Security requirement for reading the BL characteristic value. */
    security_req_t         bl_cccd_wr_sec;                 /**< Security requirement for writing the BL characteristic CCCD. */
    security_req_t         bl_report_rd_sec;               /**< Security requirement for reading the BL characteristic descriptor. */
} diagnostics_service_init_t;

/**@brief Battery Service structure. This contains various status information for the service. */
struct diagnostics_service_s
{
    diagnostics_service_evt_handler_t   evt_handler;                            /**< Event handler to be called for handling events in the Battery Service. */
    uint16_t                            service_handle;                         /**< Handle of Battery Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t            weight_filter_output_coefficient_handles;/**< Handles related to the Battery Level characteristic. */
    uint16_t                            report_ref_handle;                      /**< Handle of the Report Reference descriptor. */
    float                               weight_filter_output_coefficient_last;         /**< Last Battery Level measurement passed to the Battery Service. */
    bool                                is_notification_supported;              /**< TRUE if notification of Battery Level is supported. */
    uint8_t                             uuid_type;
};


/**@brief Function for initializing the Battery Service.
 *
 * @param[out]  p_bas       Battery Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_bas_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
ret_code_t diagnostics_service_init();


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Battery Service.
 *
 * @note For the requirements in the BAS specification to be fulfilled,
 *       ble_bas_battery_level_update() must be called upon reconnection if the
 *       battery level has changed while the service has been disconnected from a bonded
 *       client.
 *
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 * @param[in]   p_context   Battery Service structure.
 */
void diagnostics_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);



ret_code_t diagnostics_service_weight_filter_output_coefficient_update(float coefficient, uint16_t conn_handle);


/**@brief Function for sending the last battery level when bonded client reconnects.
 *
 * @details The application calls this function, in the case of a reconnection of
 *          a bonded client if the value of the battery has changed since
 *          its disconnection.
 *
 * @note For the requirements in the BAS specification to be fulfilled,
 *       this function must be called upon reconnection if the battery level has changed
 *       while the service has been disconnected from a bonded client.
 *
 * @param[in]   p_bas          Battery Service structure.
 * @param[in]   conn_handle    Connection handle.
 *
 * @return      NRF_SUCCESS on success,
 *              NRF_ERROR_INVALID_STATE when notification is not supported,
 *              otherwise an error code returned by @ref sd_ble_gatts_hvx.
 */
ret_code_t diagnostics_service_battery_lvl_on_reconnection_update(uint16_t    conn_handle);


void diagnostics_service_weight_filter_output_coefficient_received_callback(void (*func)(float coefficient ));


extern diagnostics_service_t m_diagnostics_service;

#ifdef __cplusplus
}
#endif

#endif // DIAGNOSTICS_SERVICE_H__

/** @} */
