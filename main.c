#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

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

#include "Components/LCD/scales_lcd.h"

#include "Components/ADS123X/ADS123X.h"
#include "Components/FuelGauge/MAX17260/max17260.h"
#include "Components/LED/nrf_buddy_led.h"
#include "Components/Bluetooth/Bluetooth.h"
#include "Components/Bluetooth/Services/WeightSensorService.h"
#include "Components/SavedParameters/SavedParameters.h"

APP_TIMER_DEF(m_notification_timer_id);
APP_TIMER_DEF(m_display_timer_id);
APP_TIMER_DEF(m_wakeup_timer_id);
APP_TIMER_DEF(m_keep_alive_timer_id);

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

#define NOTIFICATION_INTERVAL           APP_TIMER_TICKS(20)   
#define DISPLAY_NOTIFICATION_INTERVAL           APP_TIMER_TICKS(120)  
#define KEEP_ALIVE_NOTIFICATION_INTERVAL           APP_TIMER_TICKS(60000)
#define WAKEUP_NOTIFICATION_INTERVAL           APP_TIMER_TICKS(500) 

// Create a Handle for the twi communication
const nrfx_twi_t m_twi = NRFX_TWI_INSTANCE(TWI_INSTANCE_ID);

MAX17260 max17260Sensor;
ADS123X scale;

bool writeToWeightCharacteristic = false;

void tare_scale()
{
    ADS123X_tare(&scale, 80);

    NRF_LOG_INFO("Scales tared.");
}

void initialise_weight_sensor();

void calibrate_scale()
{
    ret_code_t err_code;
    err_code = app_timer_stop(m_notification_timer_id);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_stop(m_keep_alive_timer_id);
    APP_ERROR_CHECK(err_code);

    float val; 
    ADS123X_readAverage(&scale, &val, 80);

    float scaleFactor = ((val - ADS123X_getOffset(&scale))/50.0f);
    NRF_LOG_RAW_INFO("Scale Factor:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(scaleFactor) );
    ADS123X_setScaleFactor(&scale, scaleFactor);

    saved_parameters_SetSavedScaleFactor(scaleFactor);

    scaleFactor = saved_parameters_getSavedScaleFactor();

    NRF_LOG_RAW_INFO("Read Back Scale Factor:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(scaleFactor) );

    NRF_LOG_INFO("Scales calibrated.");

    initialise_weight_sensor();
}

void set_coffee_to_water_ratio(uint16_t requestValue)
{
    saved_parameters_setCoffeeToWaterRatioNumerator(requestValue >> 8);
    saved_parameters_setCoffeeToWaterRatioDenominator(requestValue);
}

void set_weigh_mode(uint8_t requestValue)
{
    saved_parameters_setWeighMode(requestValue);
}      

void enable_write_to_weight_characteristic()
{
    writeToWeightCharacteristic = true;
}

void disable_write_to_weight_characteristic()
{
    writeToWeightCharacteristic = false;
}

void initialise_weight_sensor()
{    
    ret_code_t err_code = app_timer_start(m_notification_timer_id, NOTIFICATION_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_display_timer_id, DISPLAY_NOTIFICATION_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);


    err_code = app_timer_start(m_keep_alive_timer_id, KEEP_ALIVE_NOTIFICATION_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}



/**@brief Function for handling the weight measurement timer timeout.
 *
 * @details This function will be called each time the weight measurement timer expires.
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

    roundedValue = abs(roundf(scaleValue * 10.0));

    if (writeToWeightCharacteristic)
    {
        ble_weight_sensor_service_sensor_data_update((uint8_t*)&scaleValue, sizeof(float));
    }
    //weight_print(roundedValue/10.0);

    //printSquare();
    
    //NRF_LOG_RAW_INFO("ScaledValue:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(roundedValue/10) );
    //NRF_LOG_FLUSH();


    /*if (&max17260Sensor.initialised)
    {
        float soc;
        max17260_getStateOfCharge(&max17260Sensor, &soc);
        bluetooth_update_battery_level((uint8_t)roundf(soc));
    }*/

    ret_code_t err_code = app_timer_start(m_notification_timer_id, NOTIFICATION_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

static void display_timeout_handler(void * p_context)
{
   // weight_print(roundedValue/10.0);

    ret_code_t err_code = app_timer_start(m_display_timer_id, DISPLAY_NOTIFICATION_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

}

float m_last_keep_alive_value = 0.0;
float m_last_wakeup_value = 0.0;
static void keep_alive_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    ret_code_t err_code;
    // create arrays which will hold x,y & z co-ordinates values of acc

    if (abs(m_last_keep_alive_value - roundedValue) < 0.3 && !writeToWeightCharacteristic)
    {
        err_code = app_timer_stop(m_notification_timer_id);
        APP_ERROR_CHECK(err_code);

        err_code = app_timer_start(m_wakeup_timer_id, WAKEUP_NOTIFICATION_INTERVAL, NULL);
        APP_ERROR_CHECK(err_code);

        // set LCD backlight to off
        nrf_gpio_cfg_output(45);
        nrf_gpio_pin_clear(45);

        m_last_wakeup_value  = roundedValue;

    }

    m_last_keep_alive_value = roundedValue;


}


static void wakeup_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    ret_code_t err_code;

    //ADS123X_PowerOn(&scale);

    ADS123X_getUnits(&scale, &scaleValue, 2U);
    //ADS123X_PowerOff(&scale);

    roundedValue = abs(roundf(scaleValue * 10.0));

    if (abs(m_last_wakeup_value - roundedValue) > 0.3)
    {
        // set LCD backlight to on
        nrf_gpio_cfg_output(45);
        nrf_gpio_pin_set(45);
        print_taring();
        ADS123X_tare(&scale, 80);
        err_code = app_timer_stop(m_wakeup_timer_id);
        APP_ERROR_CHECK(err_code);
        initialise_weight_sensor();
    }
    else{
        m_last_wakeup_value = roundedValue;
        
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
    
    err_code = app_timer_create(&m_notification_timer_id, APP_TIMER_MODE_SINGLE_SHOT, notification_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_display_timer_id, APP_TIMER_MODE_SINGLE_SHOT, display_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_wakeup_timer_id, APP_TIMER_MODE_REPEATED, wakeup_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_keep_alive_timer_id, APP_TIMER_MODE_REPEATED, keep_alive_timeout_handler);
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

    // Initialise the saved parameters module
   saved_parameters_init();
    
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
    ble_weight_sensor_set_coffee_to_water_ratio_callback(&set_coffee_to_water_ratio);
    ble_weight_sensor_set_weigh_mode_callback(&set_weigh_mode);

    uint16_t savedCoffeeToWaterRatio = saved_parameters_getCoffeeToWaterRatioNumerator() << 8 | saved_parameters_getCoffeeToWaterRatioDenominator();
    ble_weight_sensor_service_coffee_to_water_ratio_update((uint8_t*)&savedCoffeeToWaterRatio, sizeof(savedCoffeeToWaterRatio));

    uint8_t savedWeighMode = saved_parameters_getWeighMode();
    ble_weight_sensor_service_weigh_mode_update(&savedWeighMode, sizeof(savedWeighMode));



    bluetooth_register_connected_callback(enable_write_to_weight_characteristic);
    bluetooth_register_disconnected_callback(disable_write_to_weight_characteristic);

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
    



    NRF_LOG_FLUSH();

    scales_lcd_init();
    text_print();

    uint8_t pin_DOUT = 33;
    uint8_t pin_SCLK = 35;
    uint8_t pin_PWDN = 37;
    uint8_t pin_GAIN0 = 36;
    uint8_t pin_GAIN1 = 38;
    uint8_t pin_SPEED = 39;

    NRF_LOG_INFO("Starting");
    NRF_LOG_FLUSH();

    
    ADS123X_Init(&scale, pin_DOUT, pin_SCLK, pin_PWDN, pin_GAIN0, pin_GAIN1, pin_SPEED);
    
    ADS123X_PowerOff(&scale);
    ADS123X_setGain(&scale, GAIN_128);
    ADS123X_setSpeed(&scale, SPEED_80SPS);

    ADS123X_PowerOn(&scale);

    //ADS123X_calibrateOnNextConversion(&scale);
    print_taring();
    ADS123X_tare(&scale, 80);
    float taredValue = ADS123X_getOffset(&scale);

    //screen_clear();
    NRF_LOG_RAW_INFO("Offset:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(taredValue) );

    NRF_LOG_FLUSH();

    float scaleFactor = saved_parameters_getSavedScaleFactor();
    ADS123X_setScaleFactor(&scale, scaleFactor);

    NRF_LOG_RAW_INFO("Scale Factor:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(scaleFactor) );

    ADS123X_getUnits(&scale, &scaleValue, 2U);

    initialise_weight_sensor();    

    for (;;)
    {
        bluetooth_idle_state_handle();
    }
}

float map(int32_t value, int32_t low1, int32_t high1, int32_t low2, int32_t high2) {
    return low2 + ((float)(high2 - low2) * (value - low1) / (high1 - low1));
}

