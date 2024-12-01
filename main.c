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
#include "Components/SavedParameters/SavedParameters.h"
#include "Components/IQS227D/iqs227d.h"

APP_TIMER_DEF(m_read_weight_timer_id);
APP_TIMER_DEF(m_display_timer_id);
APP_TIMER_DEF(m_wakeup_timer_id);
APP_TIMER_DEF(m_keep_alive_timer_id);
APP_TIMER_DEF(m_elapsed_time_timer_id);
APP_TIMER_DEF(m_battery_level_timer_id);

/* Time between RTC interrupts. */
#define APP_TIMER_TICKS_TIMEOUT APP_TIMER_TICKS(100)


uint32_t currentElapsedTime = 0;

//Initializing TWI0 instance
#define TWI_INSTANCE_ID                 0
#define TWI_SECONDARY_INSTANCE_ID       1

//I2C Pins Settings, you change them to any other pins
#define TWI_SCL_M           6         //I2C SCL Pin
#define TWI_SDA_M           8        //I2C SDA Pin

#define SPI3_DC_PIN 40
#define SPI3_SCK_PIN 12
#define SPI3_MISO_PIN 14
#define SPI3_MOSI_PIN 14
#define ST7789_SS_PIN 11
#define ST7789_RST_PIN 16
#define ST7789_EN_PIN 41
#define ST7789_BACKLIGHT_PIN 7;

// Create a Handle for the twi communication
const nrfx_twi_t m_twi = NRFX_TWI_INSTANCE(TWI_INSTANCE_ID);
const nrfx_twi_t m_twi_secondary = NRFX_TWI_INSTANCE(TWI_SECONDARY_INSTANCE_ID);

//#define ST7789_SPI_INSTANCE 3
//static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(ST7789_SPI_INSTANCE);  /**< SPI instance. */

#define READ_WEIGHT_SENSOR_TICKS_INTERVAL       APP_TIMER_TICKS(40)   
#define DISPLAY_UPDATE_WEIGHT_INTERVAL_TICKS    APP_TIMER_TICKS(40)  
#define KEEP_ALIVE_INTERVAL_TICKS               APP_TIMER_TICKS(180000)
#define WAKEUP_NOTIFICATION_INTERVAL            APP_TIMER_TICKS(500) 
#define ELAPSED_TIMER_TIMER_INTERVAL            APP_TIMER_TICKS(1000) 
#define BATTERY_LEVEL_TIMER_INTERVAL            APP_TIMER_TICKS(5000) 

MAX17260 max17260Sensor;

void touchSensor4TOutChanged(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    
    if (nrf_gpio_pin_read(pin) == 0U)
    {
        NRF_LOG_INFO("Tout Touched.");
    }
    else
    {
        NRF_LOG_INFO("Tout released.");
        weight_sensor_tare();
    }
}

void touchSensor4POutChanged(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    NRF_LOG_INFO("Pout Changed.");
}

IQS227D  touchSensor4 =    {
                            .pin_POUT = 46,
                            .pin_TOUT = 45,
                            .pin_VCC = 47,
                            .toutChangedFcn = touchSensor4TOutChanged,
                            .poutChangedFcn = touchSensor4POutChanged,
                           };


Scales_Display_t display1 = {
    .dc_pin = 40,
    .rst_pin = 16,
    .en_pin = 41,
    .backlight_pin = 7,
};

bool writeToWeightCharacteristic = false;

void start_weight_sensor_timers();
void set_coffee_weight_callback();
void start_timer_callback();

void prepare_to_sleep();
void wakeup_from_sleep();


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

    weight_sensor_enable_weight_change_sense(start_timer_callback);
}

void start_timer_callback()
{
    ret_code_t err_code = app_timer_stop(m_elapsed_time_timer_id);
    APP_ERROR_CHECK(err_code);

    currentElapsedTime = 0;

    err_code = app_timer_start(m_elapsed_time_timer_id, ELAPSED_TIMER_TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);   
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
    //NRF_LOG_RAW_INFO("weight:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(scaleValue) );

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
        NRF_LOG_FLUSH();

        wakeup_from_sleep();
    }
    else
    {
        NRF_LOG_INFO("DIDN'T MEET WAKE THRESHOLD.");
        NRF_LOG_FLUSH();
        weight_sensor_sleep();
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
        float voltage;
        float external_source_power = 0;
        float remaining_capacity = 0;

        max17260_getStateOfCharge(&max17260Sensor, &soc);
        max17260_getAvgCurrent(&max17260Sensor, &current);
        max17260_getTimeToFull(&max17260Sensor, &ttf);
        max17260_getTimeToEmpty(&max17260Sensor, &tte);
        max17260_getCellVoltage(&max17260Sensor, &voltage);
        max17260_getRemainingCapacity(&max17260Sensor, &remaining_capacity);

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
            battery_time_status.flags = 0x01;
            battery_time_status.time_uintil_recharged_bytes[0] = ttf_minutes & 0xFF;
            battery_time_status.time_uintil_recharged_bytes[1] = ttf_minutes >> 8 & 0xFF;
            battery_time_status.time_uintil_recharged_bytes[2] = ttf_minutes >> 16 & 0xFF;

            external_source_power = voltage * current;
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

        NRF_LOG_RAW_INFO("Cell Voltage:%s%d.%01d mA\n" , NRF_LOG_FLOAT_SCALES(voltage) );


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
    err_code = nrfx_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);
    
    //Enable the TWI Communication
    nrfx_twi_enable(&m_twi);
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

/*
static ret_code_t spi3_master_init()
{
    ret_code_t err_code;

    nrf_gpio_cfg_output(SPI3_DC_PIN);

    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;

    spi_config.sck_pin  = SPI3_MISO_PIN;
    spi_config.miso_pin = SPI3_MISO_PIN;
    spi_config.mosi_pin = SPI3_MOSI_PIN;
    spi_config.ss_pin   = SPI3_DC_PIN;
    spi_config.frequency = SPIM_FREQUENCY_FREQUENCY_M32;

    err_code = nrfx_spim_init(&spi, &spi_config, NULL, NULL);
    return err_code;
}
*/

void prepare_to_sleep()
{
    ret_code_t err_code;

    err_code = app_timer_stop(m_read_weight_timer_id);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_stop(m_battery_level_timer_id);
    APP_ERROR_CHECK(err_code);

    weight_sensor_sleep();

    twi_master_uninit();

    display_sleep();

    err_code = app_timer_stop(m_elapsed_time_timer_id);
    APP_ERROR_CHECK(err_code);

    bluetooth_advertising_stop();

    err_code = app_timer_start(m_wakeup_timer_id, WAKEUP_NOTIFICATION_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
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

    bluetooth_advertising_start(false);

    ret_code_t err_code = app_timer_start(m_battery_level_timer_id, BATTERY_LEVEL_TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for application main en
try.
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

    twi_master_init();                  // initialize nRF5 the twi library 
    //twi_master_secondary_init();
    timers_init();                      // Initialise nRF5 timers library
    nrf_buddy_leds_init();              // initialise nRF52 buddy leds library
    power_management_init();            // initialise the nRF5 power management library
    bluetooth_init();
    display_init();

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
    ble_weight_sensor_set_start_timer_callback(begin_timer_on_weight_change);

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

    
    //iqs227d_init(&touchSensor4, &m_twi_secondary);

    NRF_LOG_FLUSH();

    weight_sensor_wakeup();
    display_wakeup();


    start_weight_sensor_timers(); 

    for (;;)
    {
        bluetooth_idle_state_handle();
    }
}
