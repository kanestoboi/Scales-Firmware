#ifndef WEIGHT_SENSOR_h
#define WEIGHT_SENSOR_h
#include "nrf_drv_gpiote.h"

#define NRF_LOG_FLOAT_SCALES(val) (uint32_t)(((val) < 0 && (val) > -1.0) ? "-" : ""),   \
                           (int32_t)(val),                                              \
                           (int32_t)((((val) > 0) ? (val) - (int32_t)(val)              \
                                                : (int32_t)(val) - (val))*10)

typedef enum  
{
    NORMAL,
    START_TARING,
    TARING,
    VERIFY_TARE,
    START_CALIBRATION,
    CALIBRATING,
    VERIFY_CALIBRATION,
} weight_sensor_state_t;

typedef enum  
{
    NOT_SENSING,
    SENSING_WEIGHT_CHANGE
} weight_sensor_sense_state_t;

typedef struct
{
    float scaleFactor;
    void (*newWeightValueReceivedCallback)(float weight);
    void (*newWeightFilteredValueReceivedCallback)(float weight);
    void (*conversionCompleteCallback)(void);
} weight_sensor_init_t;

void weight_sensor_init(weight_sensor_init_t ws_init);
float weight_sensor_get_weight();
float weight_sensor_get_weight_filtered();
void weight_sensor_tare();
void weight_sensor_calibrate(void (*calibrationCompleteCallback)(float scaleFactor ));

void weight_sensor_sleep();
void weight_sensor_wakeup();

float weight_sensor_read_weight();

void weight_sensor_enable_weight_change_sense(void (*weightSenseTriggeredCallback)(void));

void weight_sensor_get_stable_weight(void (*stableWeightAcheivedCallback)(void));

void weight_sensor_set_weight_filter_input_coefficient(float coefficient);

void weight_sensor_set_weight_filter_output_coefficient(float coefficient);

uint16_t weight_sensor_get_taring_attempts();
uint16_t weight_sensor_get_sampling_rate();

void weight_sensor_data_ready_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

void weight_sensor_tick_inc(uint32_t tickPeriod);

float weight_sensor_get_grams_per_second();

#endif