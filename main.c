#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"
#include "nrfx_twi.h"

#include "Components/ADS123X/ADS123X.h"
#include "Components/FuelGauge/MAX17260/max17260.h"
#include "Components/LED/nrf_buddy_led.h"
#include "Components/Bluetooth/Bluetooth.h"
#include "Components/Bluetooth/Services/WeightSensorService.h"
#include "Components/SavedParameters/SavedParameters.h"

APP_TIMER_DEF(m_notification_timer_id);

#define NRF_LOG_FLOAT_SCALES(val) (uint32_t)(((val) < 0 && (val) > -1.0) ? "-" : ""),   \
                           (int32_t)(val),                                              \
                           (int32_t)((((val) > 0) ? (val) - (int32_t)(val)              \
                                                : (int32_t)(val) - (val))*10)
float scaleValue;
float roundedValue;



//Initializing TWI0 instance
#define TWI_INSTANCE_ID     0

//I2C Pins Settings, you change them to any other pins
#define TWI_SCL_M           6         //I2C SCL Pin
#define TWI_SDA_M           8        //I2C SDA Pin

#define NOTIFICATION_INTERVAL           APP_TIMER_TICKS(80)

// Create a Handle for the twi communication
const nrfx_twi_t m_twi = NRFX_TWI_INSTANCE(TWI_INSTANCE_ID);

MAX17260 max17260Sensor;
ADS123X scale;

void tare_scale()
{
    ADS123X_tare(&scale, 80);

    NRF_LOG_INFO("Scales tared.");
}

void calibrate_scale()
{
    float val; 
    ADS123X_readAverage(&scale, &val, 80);

    nrf_gpio_pin_set(42U);
    float scaleFactor = ((val - ADS123X_getOffset(&scale))/50.0f);
    NRF_LOG_RAW_INFO("Scale Factor:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(scaleFactor) );
    ADS123X_setScaleFactor(&scale, scaleFactor);

    saved_parameters_SetSavedScaleFactor(scaleFactor);

    scaleFactor = saved_parameters_getSavedScaleFactor();

    NRF_LOG_RAW_INFO("Read Back Scale Factor:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(scaleFactor) );

    NRF_LOG_INFO("Scales calibrated.");
}

void initialise_accelerometer()
{    
    ret_code_t err_code = app_timer_start(m_notification_timer_id, NOTIFICATION_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

void shutdown_accelerometer()
{
    NRF_LOG_INFO("Shutting down scales");
    NRF_LOG_FLUSH();
   
    ret_code_t err_code = app_timer_stop(m_notification_timer_id);
    APP_ERROR_CHECK(err_code);

}

/**@brief Function for handling the Accelerometer measurement timer timeout.
 *
 * @details This function will be called each time the accelerometer level measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void notification_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    //ret_code_t err_code;
    // create arrays which will hold x,y & z co-ordinates values of acc

    ADS123X_getUnits(&scale, &scaleValue, 2U);

    roundedValue = abs(roundf(scaleValue * 10));

    ble_weight_sensor_service_sensor_data_update((uint8_t*)&scaleValue, sizeof(float));
    
    //NRF_LOG_RAW_INFO("ScaledValue:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(roundedValue/10) );
    //NRF_LOG_FLUSH();


    if (&max17260Sensor.initialised)
    {
        float soc;
        max17260_getStateOfCharge(&max17260Sensor, &soc);
        bluetooth_update_battery_level((uint8_t)roundf(soc));
    }
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    
    err_code = app_timer_create(&m_notification_timer_id, APP_TIMER_MODE_REPEATED, notification_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
void bluetooth_advertising_timeout_callback(void)
{
    ret_code_t err_code;

    err_code = nrf_buddy_led_indication(NRF_BUDDY_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    
#ifdef DEBUG_NRF
    // When in debug, this will put the device in a simulated sleep mode, still allowing the debugger to work
    sd_power_system_off();
    NRF_LOG_INFO("Powered off");
    while(1);
#else
    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    NRF_LOG_INFO("Powered off");
    APP_ERROR_CHECK(err_code);
#endif

}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

//Event Handler for TWI events
void twi_handler(nrfx_twi_evt_t const * p_event, void * p_context)
{
    //Check the event to see what type of event occurred
    switch (p_event->type)
    {
              //If data transmission or receiving is finished
      	case NRFX_TWI_EVT_DONE:
            switch (p_event->xfer_desc.address)
            {
                case MAX17260_ADDRESS:
                    max17260Sensor.mTransferDone = true;
                    break;

                default:
                    // do nothing
                    break;
            }
            break;

        case NRFX_TWI_EVT_ADDRESS_NACK:
           switch (p_event->xfer_desc.address)
            {
                case MAX17260_ADDRESS:
                    max17260Sensor.mTransferDone = true;
                    break;

                default:
                    // do nothing
                    break;
            }
            break;

        case NRFX_TWI_EVT_DATA_NACK:
            switch (p_event->xfer_desc.address)
            {
                 case MAX17260_ADDRESS:
                    max17260Sensor.mTransferDone = true;
                    break;

                default:
                    // do nothing
                    break;
            }
            break;
    }

    NRF_LOG_FLUSH();
}

//Initialize the TWI as Master device
void twi_master_init(void)
{
    ret_code_t err_code;

    // Configure the settings for twi communication
    const nrfx_twi_config_t twi_config = {
       .scl                = TWI_SCL_M,  //SCL Pin
       .sda                = TWI_SDA_M,  //SDA Pin
       .frequency          = NRF_TWI_FREQ_400K, //Communication Speed
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH, //Interrupt Priority(Note: if using Bluetooth then select priority carefully)
       .hold_bus_uninit     = true //automatically clear bus
    };

    //A function to initialize the twi communication
    err_code = nrfx_twi_init(&m_twi, &twi_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);
    
    //Enable the TWI Communication
    nrfx_twi_enable(&m_twi);
}

/**@brief Function for application main en
try.
 */
int main(void)
{
    bool erase_bonds;

    ret_code_t err_code;

    // Initialize the nRF logger. Log messages are sent out the RTT interface
    log_init();
    
    // Start execution.
    NRF_LOG_INFO("Scales Started.");
    NRF_LOG_INFO("Initilising Firmware...");
    NRF_LOG_FLUSH();

    twi_master_init();                  // initialize nRF5 the twi library 
    timers_init();                      // Initialise nRF5 timers library
    nrf_buddy_leds_init();              // initialise nRF52 buddy leds library
    power_management_init();            // initialise the nRF5 power management library

    bluetooth_init();
    if (ble_weight_sensor_service_init() != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Error Initialing weight sensor service");
    }

    ble_weight_sensor_set_tare_callback(&tare_scale);
    ble_weight_sensor_set_calibration_callback(&calibrate_scale);

    saved_parameters_init();
    bluetooth_advertising_start(erase_bonds);
    NRF_LOG_INFO("Bluetooth setup complete");
    NRF_LOG_FLUSH();

    /*if (max17260_init(&max17260Sensor, &m_twi))
    {
        NRF_LOG_INFO("MAX17260 Initialised");
        
        uint16_t val;
        max17260_register_read(&max17260Sensor, 0x18, (uint8_t*)&val, 2);
        NRF_LOG_INFO("Value: %X", val);
    }
    else
    {
        NRF_LOG_INFO("MAX17260 not found");
    }*/
    
    bluetooth_register_connected_callback(initialise_accelerometer);
    bluetooth_register_disconnected_callback(shutdown_accelerometer);


    NRF_LOG_FLUSH();

    // Keeping this LED on as a visual indicator that the accelerometer has been found        
    //nrf_buddy_led_on(3);

    // Enter main loop.

     nrf_gpio_cfg_output(42U);
    nrf_gpio_pin_set(42U);
    //nrf_delay_ms(500); 
    //nrf_gpio_pin_set(8U);
    //nrf_delay_ms(500); 

    uint8_t pin_DOUT = 33;
    uint8_t pin_SCLK = 35;
    uint8_t pin_PWDN = 37;
    uint8_t pin_GAIN0 = 36;
    uint8_t pin_GAIN1 = 38;
    uint8_t pin_SPEED = 39;

    NRF_LOG_INFO("Starting");


    
    ADS123X_Init(&scale, pin_DOUT, pin_SCLK, pin_PWDN, pin_GAIN0, pin_GAIN1, pin_SPEED);
    
    ADS123X_PowerOff(&scale);
    ADS123X_setGain(&scale, GAIN_128);
    ADS123X_setSpeed(&scale, SPEED_80SPS);
    nrf_delay_ms(500); 

    ADS123X_PowerOn(&scale);

    //ADS123X_calibrateOnNextConversion(&scale);
    ADS123X_tare(&scale, 80);
    float taredValue = ADS123X_getOffset(&scale);
    NRF_LOG_RAW_INFO("Offset:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(taredValue) );

    NRF_LOG_FLUSH();

    float scaleFactor = saved_parameters_getSavedScaleFactor();
    ADS123X_setScaleFactor(&scale, scaleFactor);

    NRF_LOG_RAW_INFO("Scale Factor:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(scaleFactor) );

    ADS123X_getUnits(&scale, &scaleValue, 2U);


    for (;;)
    {
        bluetooth_idle_state_handle();
    }
}

float map(int32_t value, int32_t low1, int32_t high1, int32_t low2, int32_t high2) {
    return low2 + ((float)(high2 - low2) * (value - low1) / (high1 - low1));
}

