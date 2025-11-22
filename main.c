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
#include "nrfx_spim.h"
#include "nrf_delay.h"

#include "Components/LCD/scales_lcd.h"

#include "Components/WeightSensor/WeightSensor.h"
#include "Components/FuelGauge/MAX17260/max17260.h"
#include "Components/LED/nrf_buddy_led.h"
#include "Components/Bluetooth/Bluetooth.h"
#include "Components/Bluetooth/Services/WeightSensorService.h"
#include "Components/Bluetooth/Services/ElapsedTimeService.h"
#include "Components/Bluetooth/Services/BatteryService.h"
#include "Components/Bluetooth/Services/ButtonThresholdService.h"
#include "Components/Bluetooth/Services/DiagnosticsService.h"
#include "Components/SavedParameters/SavedParameters.h"
#include "Components/IQS227D/iqs227d.h"

APP_TIMER_DEF(m_elapsed_time_timer_id);
APP_TIMER_DEF(m_battery_level_timer_id);
APP_TIMER_DEF(m_touch_sensor1_timer_id);
APP_TIMER_DEF(m_touch_sensor4_timer_id);


typedef enum  
{
    OFF,
    ON,
    WAITING_FOR_TOUCH_4_RELEASE,
} Scales_Operational_State_t;

typedef enum  
{
    IDLE,
    WAITING_FOR_TOUCH_RELEASE,
} Button_Operation_State_t;

Scales_Operational_State_t scalesOperationalState = OFF;

Button_Operation_State_t button1OperationState = IDLE;

uint32_t currentElapsedTime = 0;
bool elapsed_time_timer_running = false;

//Initializing TWI0 instance
#define TWI0_INSTANCE_ID                 0
#define TWI_SECONDARY_INSTANCE_ID       1

//I2C Pins Settings, you change them to any other pins
#define TWI0_SCL_M           6         //I2C SCL Pin
#define TWI0_SDA_M           8        //I2C SDA Pin

#define SPI3_SCK_PIN 12
#define SPI3_MISO_PIN 14
#define SPI3_MOSI_PIN 14
#define SPI3_SS_PIN 11

// Create a Handle for the twi communication
const nrfx_twi_t m_twi0 = NRFX_TWI_INSTANCE(TWI0_INSTANCE_ID);
const nrfx_twi_t m_twi_secondary = NRFX_TWI_INSTANCE(TWI_SECONDARY_INSTANCE_ID);

#define ST7789_SPI_INSTANCE 3
static const nrfx_spim_t spim3 = NRFX_SPIM_INSTANCE(ST7789_SPI_INSTANCE);  /**< SPI instance. */

#define ELAPSED_TIMER_TIMER_INTERVAL            APP_TIMER_TICKS(1000) 
#define BATTERY_LEVEL_TIMER_INTERVAL            APP_TIMER_TICKS(500) 
#define TOUCH_SENSOR1_TIMER_INTERVAL            APP_TIMER_TICKS(3000) 
#define TOUCH_SENSOR4_TIMER_INTERVAL            APP_TIMER_TICKS(3000) 

MAX17260 max17260Sensor;
bool writeToWeightCharacteristic = false;

void begin_timer_on_weight_change();
void start_weight_sensor_timers();
void set_coffee_weight_callback();
void start_elapsed_timer_timer_callback();
void stop_elapsed_time_timer();

void prepare_to_sleep();
void wakeup_from_sleep();

void touchSensor1TOutChanged(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (nrf_gpio_pin_read(pin) == 0U)
    {
        if (!elapsed_time_timer_running)
        {
            display_update_timer_label(0);
            ret_code_t err_code = app_timer_start(m_touch_sensor1_timer_id, TOUCH_SENSOR1_TIMER_INTERVAL, NULL);
            APP_ERROR_CHECK(err_code);
        }
        
        NRF_LOG_INFO("Pin %d Tout Touched.", pin);
    }
    else
    {
        ret_code_t err_code = app_timer_stop(m_touch_sensor1_timer_id);
        APP_ERROR_CHECK(err_code);
        NRF_LOG_INFO("Pin %d Tout released.", pin);
        
        if (button1OperationState == WAITING_FOR_TOUCH_RELEASE)
        {
            weight_sensor_get_stable_weight(begin_timer_on_weight_change);
            
            button1OperationState = IDLE;
        }
        else
        {
            if (elapsed_time_timer_running) {
                stop_elapsed_time_timer();  // Call this if the timer is running
                
            } else {
                start_elapsed_timer_timer_callback();
            }
        }    
    }

    NRF_LOG_FLUSH();
}

void touchSensor2TOutChanged(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (nrf_gpio_pin_read(pin) == 0U)
    {
        NRF_LOG_INFO("Pin %d Tout Touched.", pin);
    }
    else
    {
        NRF_LOG_INFO("Pin %d Tout released.", pin);
        weight_sensor_get_stable_weight(set_coffee_weight_callback);
    }
    NRF_LOG_FLUSH();
}

void touchSensor3TOutChanged(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (nrf_gpio_pin_read(pin) == 0U)
    {
        display_cycle_screen();
        NRF_LOG_INFO("Pin %d Tout Touched.", pin);
    }
    else
    {
        NRF_LOG_INFO("Pin %d Tout released.", pin);
    }
    NRF_LOG_FLUSH();
}

void touchSensor4TOutChanged(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (nrf_gpio_pin_read(pin) == 0U)
    {
        ret_code_t err_code = app_timer_start(m_touch_sensor4_timer_id, TOUCH_SENSOR4_TIMER_INTERVAL, NULL);
        APP_ERROR_CHECK(err_code);
        NRF_LOG_INFO("Pin %d Tout Touched.", pin);
    }
    else
    {
        ret_code_t err_code = app_timer_stop(m_touch_sensor4_timer_id);
        APP_ERROR_CHECK(err_code);
        NRF_LOG_INFO("Pin %d Tout released.", pin);
        if (scalesOperationalState == OFF)
        {
            wakeup_from_sleep();
        }
        else if (scalesOperationalState == WAITING_FOR_TOUCH_4_RELEASE)
        {
            scalesOperationalState = OFF;
        }
        else
        {
            display_indicate_tare();
            weight_sensor_tare();
        }
    }
    NRF_LOG_FLUSH();
}

IQS227D  touchSensor1 =    {
                            .pin_POUT = 20,
                            .pin_TOUT = 21,
                            .pin_VCC = 22,
                            .toutChangedFcn = touchSensor1TOutChanged,
                            .poutChangedFcn = NULL,
                           };

IQS227D  touchSensor2 =    {
                            .pin_POUT = 23,
                            .pin_TOUT = 24,
                            .pin_VCC = 25,
                            .toutChangedFcn = touchSensor2TOutChanged,
                            .poutChangedFcn = NULL,
                           };

IQS227D  touchSensor3 =    {
                            .pin_POUT = 17,
                            .pin_TOUT = 15,
                            .pin_VCC = 19,
                            .toutChangedFcn = touchSensor3TOutChanged,
                            .poutChangedFcn = NULL,
                           };

IQS227D  touchSensor4 =    {
                            .pin_POUT = 46,
                            .pin_TOUT = 45,
                            .pin_VCC = 47,
                            .toutChangedFcn = touchSensor4TOutChanged,
                            .poutChangedFcn = NULL,
                           };


Scales_Display_t display1 = {
    .dc_pin = 40,
    .rst_pin = 16,
    .en_pin = 41,
    .backlight_pin = 7,
    .height = 172,
    .width = 320,
    .x_start_offset = 34,
    .y_start_offset = 34,
    .spim_instance = &spim3,
};


static void weight_filter_output_coefficient_callback(float coeffifient)
{
    NRF_LOG_INFO("weight_filter_output_coefficient_callback entered.");
    NRF_LOG_FLUSH();
    weight_sensor_set_weight_filter_output_coefficient(coeffifient);
    NRF_LOG_INFO("weight_filter_output_coefficient_callback complete");
    NRF_LOG_FLUSH();
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

void begin_timer_on_weight_change()
{
    ret_code_t err_code = app_timer_stop(m_elapsed_time_timer_id);
    APP_ERROR_CHECK(err_code);

    ble_elapsed_time_t elapsed_time = 
    {
        .flags = 0b00000001, // Time value is counter
        .elapsed_time_lsb = 0 & 0xFF,
        .elapsed_time_5sb = 0 >> 8 & 0xFF
    };

    ble_elapsed_time_service_elapsed_time_update(elapsed_time);

    display_update_timer_label(0);

    weight_sensor_enable_weight_change_sense(start_elapsed_timer_timer_callback);
}

void start_elapsed_timer_timer_callback()
{
    ret_code_t err_code = app_timer_stop(m_elapsed_time_timer_id);
    APP_ERROR_CHECK(err_code);

    currentElapsedTime = 0;

    display_stop_flash_elapsed_time_label();

    err_code = app_timer_start(m_elapsed_time_timer_id, ELAPSED_TIMER_TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);   

    elapsed_time_timer_running = true;
}

void stop_elapsed_time_timer()
{
    ret_code_t err_code = app_timer_stop(m_elapsed_time_timer_id);
    APP_ERROR_CHECK(err_code);

    elapsed_time_timer_running = false;
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

/**@brief Function for handling the weight measurement timer timeout.
 *
 * @details This function will be called each time the weight measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void new_weight_value_received_handler(float weight)
{
    float scaleValue = roundf(weight*10)/10.0;

    if (writeToWeightCharacteristic)
    {
        ble_weight_sensor_service_sensor_data_update((uint8_t*)&scaleValue, sizeof(float));
    }
    display_update_weight_label(scaleValue);
}

static void weight_conversion_complete_handler()
{
    float gramsPerSecond = weight_sensor_get_grams_per_second();
    uint16_t samplingRate = weight_sensor_get_sampling_rate();


    display_update_tare_attempts_label(weight_sensor_get_taring_attempts());
    display_update_grams_per_second_bar_label(gramsPerSecond);
    display_update_sampling_rate_label(samplingRate);
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
        float averageCurrent;
        float ttf;
        float tte;
        float voltage;
        float cycles;
        float external_source_power = 0;
        float remaining_capacity;
        float fullCapacity;
        float remainingCapacity;

        max17260_getStateOfCharge(&max17260Sensor, &soc);
        max17260_getAvgCurrent(&max17260Sensor, &averageCurrent);
        max17260_getTimeToFull(&max17260Sensor, &ttf);
        max17260_getTimeToEmpty(&max17260Sensor, &tte);
        max17260_getCellVoltage(&max17260Sensor, &voltage);
        max17260_getRemainingCapacity(&max17260Sensor, &remaining_capacity);
        max17260_getCycles(&max17260Sensor, &cycles);
        max17260_getFullCapacity(&max17260Sensor, &fullCapacity);
        max17260_getRemainingCapacity(&max17260Sensor, &remainingCapacity);

        battery_service_battery_level_update((uint8_t)roundf(soc), BLE_CONN_HANDLE_ALL);

        battery_time_status_t battery_time_status;

        uint32_t tte_minutes = (uint32_t)(tte / 60.0);
        uint32_t ttf_minutes = (uint32_t)(ttf / 60.0);

        battery_time_status.flags = 0;

        battery_time_status.time_until_discharged_bytes[0] = tte_minutes & 0xFF;
        battery_time_status.time_until_discharged_bytes[1] = tte_minutes >> 8 & 0xFF;
        battery_time_status.time_until_discharged_bytes[2] = tte_minutes >> 16 & 0xFF;

        if (averageCurrent > 0)
        {
            battery_time_status.flags = 0x01;
            battery_time_status.time_uintil_recharged_bytes[0] = ttf_minutes & 0xFF;
            battery_time_status.time_uintil_recharged_bytes[1] = ttf_minutes >> 8 & 0xFF;
            battery_time_status.time_uintil_recharged_bytes[2] = ttf_minutes >> 16 & 0xFF;

            external_source_power = voltage * averageCurrent;
        }
        else
        {
            battery_time_status.flags = 0x00;
        }

        battery_service_battery_time_status_update(battery_time_status, BLE_CONN_HANDLE_ALL);

        battery_energy_status_t battery_energy_status = {
            .external_power_source = float32_to_float16(external_source_power),
            .present_voltage = float32_to_float16(voltage),
            .available_energy = float32_to_float16(remaining_capacity),

        };

        battery_service_battery_energy_status_update(battery_energy_status, BLE_CONN_HANDLE_ALL);

        //NRF_LOG_RAW_INFO("Cell Voltage:%s%d.%01d V\n" , NRF_LOG_FLOAT_SCALES(voltage) );


        display_update_battery_label((uint8_t)roundf(soc));
        display_update_battery_time_to_charge_value(ttf);
        display_update_battery_time_to_empty_value(tte);
        display_update_battery_cycles_value(cycles);
        display_update_battery_average_current_value(averageCurrent);
        display_update_battery_cell_voltage_value(voltage);
        display_update_battery_full_capacity_value(fullCapacity);
        display_update_battery_remaining_capacity_value(remainingCapacity);
    }

    ret_code_t err_code = app_timer_start(m_battery_level_timer_id, BATTERY_LEVEL_TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

void touch_sensor1_timeout_handler(void * p_context)
{
    if (button1OperationState == IDLE)
    {
        ret_code_t err_code = app_timer_stop(m_elapsed_time_timer_id);
        APP_ERROR_CHECK(err_code);
        display_flash_elapsed_time_label();
        display_update_timer_label(0);
        button1OperationState = WAITING_FOR_TOUCH_RELEASE;
    }
}

void touch_sensor4_timeout_handler(void * p_context)
{
    if (scalesOperationalState == ON)
    {
        prepare_to_sleep();
        scalesOperationalState = WAITING_FOR_TOUCH_4_RELEASE;
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

    err_code = app_timer_create(&m_elapsed_time_timer_id, APP_TIMER_MODE_REPEATED, elapsed_time_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_battery_level_timer_id, APP_TIMER_MODE_SINGLE_SHOT, battery_level_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_touch_sensor1_timer_id, APP_TIMER_MODE_SINGLE_SHOT, touch_sensor1_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_touch_sensor4_timer_id, APP_TIMER_MODE_SINGLE_SHOT, touch_sensor4_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

void timers_start()
{
    ret_code_t err_code;

    err_code = app_timer_start(m_battery_level_timer_id, BATTERY_LEVEL_TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

void timers_stop()
{
    ret_code_t err_code;

    err_code = app_timer_stop(m_elapsed_time_timer_id);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_stop(m_battery_level_timer_id);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_stop(m_touch_sensor1_timer_id);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_stop(m_touch_sensor4_timer_id);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
void system_off(void)
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


//Initialize the TWI as Master device
void twi0_master_init(void)
{
    ret_code_t err_code;

    // Configure the settings for twi communication
    const nrfx_twi_config_t twi_config = {
       .scl                = TWI0_SCL_M,  //SCL Pin
       .sda                = TWI0_SDA_M,  //SDA Pin
       .frequency          = NRF_TWI_FREQ_400K, //Communication Speed
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH, //Interrupt Priority(Note: if using Bluetooth then select priority carefully)
       .hold_bus_uninit     = true //automatically clear bus
    };

    //A function to initialize the twi communication
    err_code = nrfx_twi_init(&m_twi0, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);
    
    //Enable the TWI Communication
    nrfx_twi_enable(&m_twi0);
}

//Initialize the TWI as Master device
void twi_master_secondary_init(void)
{
    ret_code_t err_code;

    // Configure the settings for twi communication
    const nrfx_twi_config_t twi_config = {
       .scl                = 17,  //SCL Pin
       .sda                = 19,  //SDA Pin
       .frequency          = NRF_TWI_FREQ_400K, //Communication Speed
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH, //Interrupt Priority(Note: if using Bluetooth then select priority carefully)
       .hold_bus_uninit     = true, //automatically clear bus
    };

    //A function to initialize the twi communication
    err_code = nrfx_twi_init(&m_twi_secondary, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);
    
    //Enable the TWI Communication
    nrfx_twi_enable(&m_twi_secondary);
}

void twi0_master_uninit(void)
{
    //disable the TWI Communication
    nrfx_twi_disable(&m_twi0);

    //A function to initialize the twi communication
    nrfx_twi_uninit(&m_twi0);

    // set pins to defult config
    nrf_gpio_cfg_default(TWI0_SCL_M);
    nrf_gpio_cfg_default(TWI0_SDA_M);
}


static ret_code_t spi3_master_init()
{
    ret_code_t err_code;

    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;

    spi_config.sck_pin  = SPI3_SCK_PIN;
    spi_config.miso_pin = SPI3_MISO_PIN;
    spi_config.mosi_pin = SPI3_MOSI_PIN;
    spi_config.ss_pin   = SPI3_SS_PIN;
    spi_config.frequency = SPIM_FREQUENCY_FREQUENCY_M32;

    err_code = nrfx_spim_init(&spim3, &spi_config, display_spi_xfer_complete_callback, NULL);
    APP_ERROR_CHECK(err_code);
    return err_code;
}

static void spi3_master_uninit()
{
    nrfx_spim_abort(&spim3);
    nrfx_spim_uninit(&spim3);

    // set pins to defult config
    nrf_gpio_cfg_default(SPI3_SCK_PIN);
    nrf_gpio_cfg_default(SPI3_MISO_PIN);
    nrf_gpio_cfg_default(SPI3_SS_PIN);
    nrf_gpio_cfg_default(SPI3_MOSI_PIN);
}


void prepare_to_sleep()
{
    timers_stop();

    // send components to sleep
    weight_sensor_sleep();
    display_sleep();

    // uninitialise peripherals for low power sleep
    twi0_master_uninit();
    spi3_master_uninit();

    // disconnect from any central device
    bluetooth_disconnect_ble_connection();

    // stop advertising
    bluetooth_advertising_stop();

    iqs227d_power_off(&touchSensor1);
    iqs227d_power_off(&touchSensor2);
    iqs227d_power_off(&touchSensor3);

    iqs227d_uninit(&touchSensor1);
    iqs227d_uninit(&touchSensor2);
    iqs227d_uninit(&touchSensor3);

    scalesOperationalState = OFF;
}

void wakeup_from_sleep()
{
    // wake up hardware peripherals
    twi0_master_init();
    spi3_master_init();

    // wake up components
    display_wakeup();
    weight_sensor_wakeup();
        
   // weight_sensor_tare();

    timers_start();

    bluetooth_advertising_start(false);

    iqs227d_init(&touchSensor1, &m_twi_secondary);
    iqs227d_init(&touchSensor2, &m_twi_secondary);
    iqs227d_init(&touchSensor3, &m_twi_secondary);

    iqs227d_power_on(&touchSensor1);
    iqs227d_power_on(&touchSensor2);
    iqs227d_power_on(&touchSensor3);

    scalesOperationalState = ON;
}

/**@brief Function for application main entry.
 */
int main(void)
{
    bool erase_bonds = false;

    ret_code_t err_code;

    // Initialize the nRF logger. Log messages are sent out the RTT interface
    log_init();

    // Initialise the saved parameters module
    saved_parameters_init();
    
    // Start execution.
    NRF_LOG_INFO("Scales Started.");
    NRF_LOG_INFO("Initilising Firmware...");
    NRF_LOG_FLUSH();

    twi0_master_init();                  // initialize nRF5 the twi library 
    spi3_master_init();
    //twi_master_secondary_init();
    timers_init();                      // Initialise nRF5 timers library
    nrf_buddy_leds_init();              // initialise nRF52 buddy leds library
    power_management_init();            // initialise the nRF5 power management library
    bluetooth_init();
    display_init(&display1);

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

    if (diagnostics_service_init() != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Error initialing diagnostics service");
    }

    ble_weight_sensor_set_tare_callback(weight_sensor_tare);
    ble_weight_sensor_set_calibration_callback(weight_sensor_service_calibration_callback);
    ble_weight_sensor_set_coffee_to_water_ratio_callback(set_coffee_to_water_ratio);
    ble_weight_sensor_set_weigh_mode_callback(set_weigh_mode);
    ble_weight_sensor_set_coffee_weight_callback(set_coffee_weight_callback);
    ble_weight_sensor_set_start_timer_callback(begin_timer_on_weight_change);

    saved_parameters_setCoffeeToWaterRatioNumerator(1);
    saved_parameters_setCoffeeToWaterRatioDenominator(16);

    weight_sensor_set_weight_filter_output_coefficient(saved_parameters_getWeightFilterOutputCoefficient());

    diagnostics_service_weight_filter_output_coefficient_received_callback(weight_filter_output_coefficient_callback);

    uint16_t savedCoffeeToWaterRatio = saved_parameters_getCoffeeToWaterRatioNumerator() << 8 | saved_parameters_getCoffeeToWaterRatioDenominator();
    ble_weight_sensor_service_coffee_to_water_ratio_update((uint8_t*)&savedCoffeeToWaterRatio, sizeof(savedCoffeeToWaterRatio));

    uint8_t savedWeighMode = saved_parameters_getWeighMode();
    ble_weight_sensor_service_weigh_mode_update(&savedWeighMode, sizeof(savedWeighMode));

    bluetooth_register_connected_callback(enable_write_to_weight_characteristic);
    bluetooth_register_disconnected_callback(disable_write_to_weight_characteristic);

    NRF_LOG_INFO("Bluetooth setup complete");
    NRF_LOG_FLUSH();

    weight_sensor_init_t ws_init = {
        .scaleFactor = saved_parameters_getSavedScaleFactor(),
        .newWeightValueReceivedCallback = NULL,//new_weight_value_received_handler,
        .newWeightFilteredValueReceivedCallback = new_weight_value_received_handler,
        .conversionCompleteCallback = weight_conversion_complete_handler
    };

    weight_sensor_init(ws_init);

    if (max17260_init(&max17260Sensor, &m_twi0))
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

    iqs227d_init(&touchSensor1, &m_twi_secondary);
    iqs227d_init(&touchSensor2, &m_twi_secondary);
    iqs227d_init(&touchSensor3, &m_twi_secondary);

    iqs227d_power_on(&touchSensor1);
    iqs227d_power_on(&touchSensor2);
    iqs227d_power_on(&touchSensor3);

    iqs227d_init(&touchSensor4, &m_twi_secondary);
    iqs227d_power_on(&touchSensor4);

    NRF_LOG_FLUSH();

    prepare_to_sleep();

    for (;;)
    {
        if (scalesOperationalState == ON)
        {
            display_loop();
        }
        else
        {
            bluetooth_idle_state_handle();
        }
    }
}
