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
#include "nrfx_twi.h"
#include "nrf_delay.h"
#include "nrf_csense.h"

#include "Components/LCD/scales_lcd.h"

#include "Components/WeightSensor/WeightSensor.h"
#include "Components/FuelGauge/MAX17260/max17260.h"
#include "Components/LED/nrf_buddy_led.h"
#include "Components/Bluetooth/Bluetooth.h"
#include "Components/Bluetooth/Services/WeightSensorService.h"
#include "Components/Bluetooth/Services/ElapsedTimeService.h"
#include "Components/Bluetooth/Services/BatteryService.h"
#include "Components/Bluetooth/Services/ButtonThresholdService.h"
#include "Components/SavedParameters/SavedParameters.h"

APP_TIMER_DEF(m_read_weight_timer_id);
APP_TIMER_DEF(m_display_timer_id);
APP_TIMER_DEF(m_wakeup_timer_id);
APP_TIMER_DEF(m_keep_alive_timer_id);
APP_TIMER_DEF(m_elapsed_time_timer_id);
APP_TIMER_DEF(m_battery_level_timer_id);

/* Time between RTC interrupts. */
#define APP_TIMER_TICKS_TIMEOUT APP_TIMER_TICKS(50)

#define BUTTON1                  6
#define BUTTON2                  1
#define BUTTON3                  0
#define BUTTON4                  4

#define TOUCHPAD1_THRESHOLD_1   900
#define TOUCHPAD1_THRESHOLD_2   600
#define TOUCHPAD1_THRESHOLD_3   900
#define TOUCHPAD1_THRESHOLD_4   900

NRF_CSENSE_BUTTON_DEF(m_button1, (BUTTON1, TOUCHPAD1_THRESHOLD_1));
NRF_CSENSE_BUTTON_DEF(m_button2, (BUTTON2, TOUCHPAD1_THRESHOLD_2));
NRF_CSENSE_BUTTON_DEF(m_button3, (BUTTON3, TOUCHPAD1_THRESHOLD_3));
NRF_CSENSE_BUTTON_DEF(m_button4, (BUTTON4, TOUCHPAD1_THRESHOLD_4));

uint32_t currentElapsedTime = 0;

//Initializing TWI0 instance
#define TWI_INSTANCE_ID     0

//I2C Pins Settings, you change them to any other pins
#define TWI_SCL_M           6         //I2C SCL Pin
#define TWI_SDA_M           8        //I2C SDA Pin

#define READ_WEIGHT_SENSOR_TICKS_INTERVAL       APP_TIMER_TICKS(40)   
#define DISPLAY_UPDATE_WEIGHT_INTERVAL_TICKS    APP_TIMER_TICKS(40)  
#define KEEP_ALIVE_INTERVAL_TICKS               APP_TIMER_TICKS(180000)
#define WAKEUP_NOTIFICATION_INTERVAL            APP_TIMER_TICKS(500) 
#define ELAPSED_TIMER_TIMER_INTERVAL            APP_TIMER_TICKS(1000) 
#define BATTERY_LEVEL_TIMER_INTERVAL            APP_TIMER_TICKS(5000) 

// Create a Handle for the twi communication
const nrfx_twi_t m_twi = NRFX_TWI_INSTANCE(TWI_INSTANCE_ID);

MAX17260 max17260Sensor;

bool writeToWeightCharacteristic = false;

void start_weight_sensor_timers();
void set_coffee_weight_callback();
void start_timer_callback();

void prepare_to_sleep();
void wakeup_from_sleep();

/**
 * @brief Event handler for Capacitive Sensor High module.
 *
 * @param [in] p_evt_type                    Pointer to event data structure.
 */
void nrf_csense_handler(nrf_csense_evt_t * p_evt)
{
    switch (p_evt->nrf_csense_evt_type)
    {
        case NRF_CSENSE_BTN_EVT_PRESSED:
            if (p_evt->p_instance == (&m_button1))
            {
                uint16_t * btn_cnt = ((uint16_t *)p_evt->p_instance->p_context);
                (*btn_cnt)++;
                NRF_LOG_INFO("Button1 touched %03d times.", (*btn_cnt));
                display_button1_count_label(*btn_cnt);
                
            }
            if (p_evt->p_instance == (&m_button2))
            {
                start_timer_callback();
                uint16_t * btn_cnt = ((uint16_t *)p_evt->p_instance->p_context);
                (*btn_cnt)++;
                NRF_LOG_INFO("Button2 touched %03d times.", (*btn_cnt));
                display_button2_count_label(*btn_cnt);
            }
            if (p_evt->p_instance == (&m_button3))
            {
                weight_sensor_tare();
                uint16_t * btn_cnt = ((uint16_t *)p_evt->p_instance->p_context);
                (*btn_cnt)++;
                NRF_LOG_INFO("Button3 touched %03d times.", (*btn_cnt));
                display_button3_count_label(*btn_cnt);
            }
            if (p_evt->p_instance == (&m_button4))
            {
                //weight_sensor_tare();
                uint16_t * btn_cnt = ((uint16_t *)p_evt->p_instance->p_context);
                (*btn_cnt)++;
                NRF_LOG_INFO("Button4 touched %03d times.", (*btn_cnt));
                display_button4_count_label(*btn_cnt);
            }
            break;
        case NRF_CSENSE_BTN_EVT_RELEASED:
            break;
        case NRF_CSENSE_SLIDER_EVT_PRESSED:
        break;
        case NRF_CSENSE_SLIDER_EVT_RELEASED:
            break;
        case NRF_CSENSE_SLIDER_EVT_DRAGGED:
            break;
        default:
            NRF_LOG_WARNING("Unknown event.");
            break;
    }
}


/**
 * @brief Function for starting Capacitive Sensor High module.
 *
 * Function enables one slider and one button.
 */
static void csense_start(void)
{


    ret_code_t err_code;

    static uint16_t touched_counter1 = 0;
    static uint16_t touched_counter2 = 0;
    static uint16_t touched_counter3 = 0;
    static uint16_t touched_counter4 = 0;

    err_code = nrf_csense_init(nrf_csense_handler, APP_TIMER_TICKS_TIMEOUT);
    APP_ERROR_CHECK(err_code);

    nrf_csense_instance_context_set(&m_button1, (void*)&touched_counter1);
    nrf_csense_instance_context_set(&m_button2, (void*)&touched_counter2);
    nrf_csense_instance_context_set(&m_button3, (void*)&touched_counter3);
    nrf_csense_instance_context_set(&m_button4, (void*)&touched_counter4);

    m_button1.p_nrf_csense_pad->threshold = saved_parameters_getButton1CSenseThreshold();
    m_button2.p_nrf_csense_pad->threshold = saved_parameters_getButton2CSenseThreshold();
    m_button3.p_nrf_csense_pad->threshold = saved_parameters_getButton3CSenseThreshold();
    m_button4.p_nrf_csense_pad->threshold = saved_parameters_getButton4CSenseThreshold();

    err_code = nrf_csense_add(&m_button1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_csense_add(&m_button2);
    //APP_ERROR_CHECK(err_code);

    err_code = nrf_csense_add(&m_button3);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_csense_add(&m_button4);
    APP_ERROR_CHECK(err_code);
}

static void calibration_complete_callback(float scaleFactor)
{
    NRF_LOG_INFO("calibration_complete_callback entered.");
    NRF_LOG_FLUSH();
    saved_parameters_SetSavedScaleFactor(scaleFactor);
    NRF_LOG_INFO("Calibration complete callback.");
    NRF_LOG_FLUSH();
}

static void weight_sensor_service_calibration_callback()
{
    weight_sensor_calibrate(calibration_complete_callback);

    NRF_LOG_INFO("weight_sensor_service_calibration_callback exit.");
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

void set_coffee_weight_callback()
{
    float coffeeWeight = roundf(weight_sensor_get_weight_filtered() * 10)/10.0;

    display_update_coffee_weight_label(coffeeWeight);

    float waterWeight = coffeeWeight / (saved_parameters_getCoffeeToWaterRatioNumerator()) * saved_parameters_getCoffeeToWaterRatioDenominator();
    NRF_LOG_RAW_INFO("waterWeight:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(waterWeight) );
    NRF_LOG_INFO("set_coffee_weight_callback - calculated water weight");
    ble_weight_sensor_service_water_weight_update(waterWeight);
    display_update_water_weight_label(waterWeight);
    NRF_LOG_INFO("set_coffee_weight_callback - exit");
}      

void start_timer_callback()
{
    ret_code_t err_code = app_timer_stop(m_elapsed_time_timer_id);
    APP_ERROR_CHECK(err_code);

    currentElapsedTime = 0;

    err_code = app_timer_start(m_elapsed_time_timer_id, ELAPSED_TIMER_TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);   
}

void button1_threshold_received_callback(uint16_t threshold)
{
    m_button1.p_nrf_csense_pad->threshold = threshold;
    NRF_LOG_INFO("Button 1 threshold updated to %d", threshold);
    saved_parameters_setButton1CSenseThreshold(threshold);
    display_button1_threshold_label(threshold);
}

void button2_threshold_received_callback(uint16_t threshold)
{
    m_button2.p_nrf_csense_pad->threshold = threshold;
    NRF_LOG_INFO("Button 2 threshold updated to %d", threshold);
    saved_parameters_setButton2CSenseThreshold(threshold);
    display_button2_threshold_label(threshold);
}

void button3_threshold_received_callback(uint16_t threshold)
{
    m_button3.p_nrf_csense_pad->threshold = threshold;
    NRF_LOG_INFO("Button 3 threshold updated to %d", threshold);
    saved_parameters_setButton3CSenseThreshold(threshold);
    display_button3_threshold_label(threshold);
}

void button4_threshold_received_callback(uint16_t threshold)
{
    m_button4.p_nrf_csense_pad->threshold = threshold;
    NRF_LOG_INFO("Button 4 threshold updated to %d", threshold);
    saved_parameters_setButton4CSenseThreshold(threshold);
    display_button4_threshold_label(threshold);
}


void enable_write_to_weight_characteristic()
{
    display_bluetooth_logo_show();
    writeToWeightCharacteristic = true;
}

void disable_write_to_weight_characteristic()
{
    display_bluetooth_logo_hide();
    writeToWeightCharacteristic = false;
}

void start_weight_sensor_timers()
{    
    ret_code_t err_code = app_timer_start(m_read_weight_timer_id, READ_WEIGHT_SENSOR_TICKS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

    //err_code = app_timer_start(m_display_timer_id, DISPLAY_UPDATE_WEIGHT_INTERVAL_TICKS, NULL);
    //APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_keep_alive_timer_id, KEEP_ALIVE_INTERVAL_TICKS, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the weight measurement timer timeout.
 *
 * @details This function will be called each time the weight measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void read_weight_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    float scaleValue = roundf(weight_sensor_get_weight_filtered()*10)/10.0;

    if (writeToWeightCharacteristic)
    {
        ble_weight_sensor_service_sensor_data_update((uint8_t*)&scaleValue, sizeof(float));
    }

    display_update_weight_label(scaleValue);

    ret_code_t err_code = app_timer_start(m_read_weight_timer_id, READ_WEIGHT_SENSOR_TICKS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

static void display_timeout_handler(void * p_context)
{
    float roundedValue = roundf(weight_sensor_get_weight_filtered() * 10.0);

    display_update_weight_label(roundedValue/10.0);

    ret_code_t err_code = app_timer_start(m_display_timer_id, DISPLAY_UPDATE_WEIGHT_INTERVAL_TICKS, NULL);
    APP_ERROR_CHECK(err_code);
}

float m_last_keep_alive_value = 0.0;
float m_last_wakeup_value = 0.0;
static void keep_alive_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    ret_code_t err_code;
    
    float roundedValue = roundf(weight_sensor_get_weight_filtered() * 10.0);

    if (fabs(m_last_keep_alive_value - roundedValue) < 5 && !writeToWeightCharacteristic)
    {
        NRF_LOG_INFO("SLEEPING");

        prepare_to_sleep();

        m_last_wakeup_value  = roundedValue;
    }
    else
    {
        NRF_LOG_INFO("NOT SLEEPING THIS ROUND");
        err_code = app_timer_start(m_keep_alive_timer_id, KEEP_ALIVE_INTERVAL_TICKS, NULL);
    }

    m_last_keep_alive_value = roundedValue;
}


static void wakeup_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    ret_code_t err_code;

    float roundedValue = roundf(weight_sensor_read_weight() * 10.0);

    if (fabs(m_last_wakeup_value - roundedValue) > 50)
    {
        err_code = app_timer_stop(m_wakeup_timer_id);
        APP_ERROR_CHECK(err_code);

        NRF_LOG_INFO("WAKING.");

        wakeup_from_sleep();
    }
    else{
        NRF_LOG_INFO("DIDN'T MEET WAKE THRESHOLD.");
        err_code = app_timer_start(m_wakeup_timer_id, WAKEUP_NOTIFICATION_INTERVAL, NULL);
        APP_ERROR_CHECK(err_code);
        m_last_wakeup_value = roundedValue;
    } 
}

void elapsed_time_timeout_handler(void * p_context)
{
    currentElapsedTime++;

    ble_elapsed_time_t elapsed_time = 
    {
        .flags = 0b00000001, // Time value is counter
        .elapsed_time_lsb = currentElapsedTime & 0xFF,
        .elapsed_time_5sb = currentElapsedTime >> 8 & 0xFF
    };

    ble_elapsed_time_service_elapsed_time_update(elapsed_time);

    display_update_timer_label(currentElapsedTime);
}

void battery_level_timeout_handler(void * p_context)
{
    if (max17260Sensor.initialised)
    {
        float soc;
        float current;
        float ttf;
        float tte;

        max17260_getStateOfCharge(&max17260Sensor, &soc);
        max17260_getAvgCurrent(&max17260Sensor, &current);
        max17260_getTimeToFull(&max17260Sensor, &ttf);
        max17260_getTimeToEmpty(&max17260Sensor, &tte);
        battery_service_battery_level_update((uint8_t)roundf(soc), BLE_CONN_HANDLE_ALL);

        battery_time_status_t battery_time_status;

        uint32_t tte_minutes = (uint32_t)(tte / 60.0);
        uint32_t ttf_minutes = (uint32_t)(ttf / 60.0);

        battery_time_status.flags = 0;

        battery_time_status.time_until_discharged_bytes[0] = tte_minutes & 0xFF;
        battery_time_status.time_until_discharged_bytes[1] = tte_minutes >> 8 & 0xFF;
        battery_time_status.time_until_discharged_bytes[2] = tte_minutes >> 16 & 0xFF;

        if (current > 0)
        {
            battery_time_status.flags = 0x02;
            battery_time_status.time_uintil_recharged_bytes[0] = ttf_minutes & 0xFF;
            battery_time_status.time_uintil_recharged_bytes[1] = ttf_minutes >> 8 & 0xFF;
            battery_time_status.time_uintil_recharged_bytes[2] = ttf_minutes >> 16 & 0xFF;
        }
        else
        {
            battery_time_status.flags = 0x02;
        }

        battery_service_battery_time_status_update(battery_time_status, BLE_CONN_HANDLE_ALL);

        display_update_battery_label((uint8_t)roundf(soc));

        //NRF_LOG_RAW_INFO("Current:%s%d.%01d mA\n" , NRF_LOG_FLOAT_SCALES(current*1000) );
        //NRF_LOG_RAW_INFO("ttf:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(ttf) );
        //NRF_LOG_RAW_INFO("tte:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(tte) );
    }

    ret_code_t err_code = app_timer_start(m_battery_level_timer_id, BATTERY_LEVEL_TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
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
    
    err_code = app_timer_create(&m_read_weight_timer_id, APP_TIMER_MODE_SINGLE_SHOT, read_weight_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_display_timer_id, APP_TIMER_MODE_SINGLE_SHOT, display_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_wakeup_timer_id, APP_TIMER_MODE_SINGLE_SHOT, wakeup_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_keep_alive_timer_id, APP_TIMER_MODE_SINGLE_SHOT, keep_alive_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_elapsed_time_timer_id, APP_TIMER_MODE_REPEATED, elapsed_time_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_battery_level_timer_id, APP_TIMER_MODE_SINGLE_SHOT, battery_level_timeout_handler);
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
    NRF_LOG_INFO("Powered off - simulated");
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

//Initialize the TWI as Master device
void twi_master_uninit(void)
{
    ret_code_t err_code;

        //Enable the TWI Communication
    nrfx_twi_disable(&m_twi);

    //A function to initialize the twi communication
    nrfx_twi_uninit(&m_twi);

    nrf_gpio_cfg_default(TWI_SCL_M);
    nrf_gpio_cfg_default(TWI_SDA_M);
}

void prepare_to_sleep()
{
    ret_code_t err_code;

    err_code = app_timer_stop(m_read_weight_timer_id);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_stop(m_battery_level_timer_id);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_wakeup_timer_id, WAKEUP_NOTIFICATION_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

    weight_sensor_sleep();

    twi_master_uninit();

    display_sleep();
}

void wakeup_from_sleep()
{
    twi_master_init();

    display_wakeup();
    weight_sensor_wakeup();

    display_indicate_tare();
    if (bluetooth_is_connected())
    {
        display_bluetooth_logo_show();
    }
    else
    {
        display_bluetooth_logo_hide();
    }
        
    weight_sensor_tare();

    start_weight_sensor_timers();

    ret_code_t err_code = app_timer_start(m_battery_level_timer_id, BATTERY_LEVEL_TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
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
    display_init();

    bluetooth_init();
    if (ble_weight_sensor_service_init() != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Error Initialing weight sensor service");
    }
 
    if (ble_elapsed_time_service_init() != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Error Initialing Elapsed Time service");
    }

    if (battery_service_init() != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Error Initialing battery service");
    }

    if (button_threshold_service_init() != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Error Initialing button threshold service");
    }

    

    ble_weight_sensor_set_tare_callback(weight_sensor_tare);
    ble_weight_sensor_set_calibration_callback(weight_sensor_service_calibration_callback);
    ble_weight_sensor_set_coffee_to_water_ratio_callback(set_coffee_to_water_ratio);
    ble_weight_sensor_set_weigh_mode_callback(set_weigh_mode);
    ble_weight_sensor_set_coffee_weight_callback(set_coffee_weight_callback);
    ble_weight_sensor_set_start_timer_callback(start_timer_callback);

    button_threshold_service_button1_threshold_received_callback(button1_threshold_received_callback);
    button_threshold_service_button2_threshold_received_callback(button2_threshold_received_callback);
    button_threshold_service_button3_threshold_received_callback(button3_threshold_received_callback);
    button_threshold_service_button4_threshold_received_callback(button4_threshold_received_callback);

    display_button1_threshold_label(saved_parameters_getButton1CSenseThreshold());
    display_button2_threshold_label(saved_parameters_getButton2CSenseThreshold());
    display_button3_threshold_label(saved_parameters_getButton3CSenseThreshold());
    display_button4_threshold_label(saved_parameters_getButton4CSenseThreshold());

    saved_parameters_setCoffeeToWaterRatioNumerator(1);
    saved_parameters_setCoffeeToWaterRatioDenominator(16);

    uint16_t savedCoffeeToWaterRatio = saved_parameters_getCoffeeToWaterRatioNumerator() << 8 | saved_parameters_getCoffeeToWaterRatioDenominator();
    ble_weight_sensor_service_coffee_to_water_ratio_update((uint8_t*)&savedCoffeeToWaterRatio, sizeof(savedCoffeeToWaterRatio));

    uint8_t savedWeighMode = saved_parameters_getWeighMode();
    ble_weight_sensor_service_weigh_mode_update(&savedWeighMode, sizeof(savedWeighMode));

    bluetooth_register_connected_callback(enable_write_to_weight_characteristic);
    bluetooth_register_disconnected_callback(disable_write_to_weight_characteristic);

    bluetooth_advertising_start(erase_bonds);
    NRF_LOG_INFO("Bluetooth setup complete");
    NRF_LOG_FLUSH();

    
    float scaleFactor = saved_parameters_getSavedScaleFactor();
    weight_sensor_init(scaleFactor);

    
    if (max17260_init(&max17260Sensor, &m_twi))
    {
        NRF_LOG_INFO("MAX17260 Initialised");
        
        uint16_t val;
        max17260_register_read(&max17260Sensor, 0x18, (uint8_t*)&val, 2);
        NRF_LOG_INFO("Value: %X", val);
    }
    else
    {
        NRF_LOG_INFO("MAX17260 not found");
    }

    NRF_LOG_FLUSH();

    start_weight_sensor_timers(); 
    
    err_code = app_timer_start(m_battery_level_timer_id, BATTERY_LEVEL_TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);  

    csense_start();

    for (;;)
    {
        bluetooth_idle_state_handle();
    }
}

float map(int32_t value, int32_t low1, int32_t high1, int32_t low2, int32_t high2) {
    return low2 + ((float)(high2 - low2) * (value - low1) / (high1 - low1));
}

