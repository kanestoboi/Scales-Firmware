#include "ADS123X/ADS123X.h"
#include "WeightSensor.h"
#include "nrf_log.h"
#include "app_timer.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "libraries/biquad/biquad.h"

#include <math.h>

ADS123X scale;

uint32_t mTimeMS = 0;
uint32_t mThisTimePeriodStart = 0;
uint32_t mLastTimePeriodStart = 0;
uint32_t mSamplesLastTimePeriod = 0;
uint32_t mSamplesThisTimePeriod = 0;
uint16_t mSamplesPerSecond = 0;
uint32_t MIN_SAMPLE_WINDOW_MS = 15*20;

uint32_t first_sample = 1;


static float mGramsPerSecond = 0.0;
static float mGramsPerSecondFiltered = 0.0;
float mGramsLastTimePeriod = 0.0;
float mGramsThisTimePeriod = 0.0;

float mScaleValue;
float prev_filtered_weight = 0;
float mFilteredScaleValue = 0.0;
float mLastFilteredScaleValue = 0.0;
float mRoundedValue;
float mSenseThresholdValue = 0.0;

float mWeightFilterInputCoefficient = 0.4;
float mWeightFilterOutputCoefficient = 0.6;

int32_t mTaringSum = 0;
int32_t mTaringReadCount = 0;
int32_t mCalibrationSum = 0;
int32_t mCalibrationReadCount = 0;

uint16_t mTaringAttempts = 0;

weight_sensor_state_t mWeightSensorCurrentState = NORMAL;
weight_sensor_sense_state_t mWeightSensorSenseState = NOT_SENSING;

bool mStableWeightRequested = false;

const uint8_t pin_DOUT = 33;
const uint8_t pin_SCLK = 35;
const uint8_t pin_PWDN = 37;
const uint8_t pin_GAIN0 = 36;
const uint8_t pin_GAIN1 = 38;
const uint8_t pin_SPEED = 39;
const uint8_t pin_APWR = 4; // analogue power pin

void (*mCalibrationCompleteCallback)(float scaleFactor) = NULL;
void (*mWeightMovedFromZeroCallback)() = NULL;
void (*mStableWeightAcheivedCallback)() = NULL;
void (*mConversionCompleteCallback)() = NULL;
void (*mNewWeightValueReceivedCallback)(float weight) = NULL;
void (*mNewWeightFilteredValueReceivedCallback)(float weight) = NULL;

// Define filter order and sections (4th order = 2 biquads)
#define NUM_SECTIONS 2

Biquad weight_filter[NUM_SECTIONS] = {
    // Section 1 coefficients (b0,b1,b2,a1,a2)
    {0.06745527, 0.13491055, 0.06745527, -1.1429805, 0.4128016, 0, 0},
    // Section 2 coefficients
    {0.06745527, 0.13491055, 0.06745527, -1.1429805, 0.4128016, 0, 0}
};

Biquad flow_filter[NUM_SECTIONS] = {
    // Example coeffs for 4th order Butterworth low-pass 10 Hz @ 80 Hz sampling
    {0.02008337, 0.04016673, 0.02008337, -1.5610181, 0.6413515, 0, 0},
    {0.02008337, 0.04016673, 0.02008337, -1.5610181, 0.6413515, 0, 0}
};


static void ads123x_timeout_handler(void * p_context)
{
    nrf_drv_gpiote_in_event_disable(pin_DOUT);

    uint32_t elapsedMs = mTimeMS - mThisTimePeriodStart;
    if (elapsedMs >= MIN_SAMPLE_WINDOW_MS) // e.g. 100 or 250 ms
    {
        mLastTimePeriodStart = mThisTimePeriodStart;
        mThisTimePeriodStart = mTimeMS;

        mSamplesLastTimePeriod = mSamplesThisTimePeriod;
        mSamplesThisTimePeriod = 0;

        // mGramsLastTimePeriod = mGramsThisTimePeriod;
        // mGramsThisTimePeriod = mFilteredScaleValue;

        // mGramsPerSecond = ((mFilteredScaleValue-mGramsLastTimePeriod) * 1000.0) / (float)elapsedMs;

        mSamplesPerSecond = (mSamplesLastTimePeriod * 1000) / elapsedMs;
    }

    mSamplesThisTimePeriod++;


    switch (mWeightSensorCurrentState)
    {
        case NORMAL:
        {
            ADS123X_ERROR_t err = ADS123X_getUnits(&scale, &mScaleValue, 1);

            if (err != NoERROR)
            {
                break;
            }


            if (mStableWeightRequested)
            {
                mLastFilteredScaleValue = mFilteredScaleValue;
            }

            mFilteredScaleValue = filter_process(weight_filter, NUM_SECTIONS, mScaleValue); //mWeightFilterOutputCoefficient * mFilteredScaleValue + mWeightFilterInputCoefficient * mScaleValue;

            if (!first_sample) {
                mGramsPerSecond = (mFilteredScaleValue - prev_filtered_weight) / (1.0/mSamplesPerSecond);
            } else {
                first_sample = 0;
            }
            prev_filtered_weight = mFilteredScaleValue;

            // Filter the flow rate
            mGramsPerSecondFiltered = filter_process(flow_filter, NUM_SECTIONS, mGramsPerSecond);


            if (mStableWeightRequested && fabs(mFilteredScaleValue - mLastFilteredScaleValue) <= 0.01)
            {
                mStableWeightAcheivedCallback();
                mStableWeightRequested = false;
            }

            if (mWeightSensorSenseState == SENSING_WEIGHT_CHANGE && mFilteredScaleValue >= mSenseThresholdValue)
            {
                mWeightMovedFromZeroCallback();
                mWeightSensorSenseState = NOT_SENSING;
            }

            if (mNewWeightValueReceivedCallback != NULL)
            {
                mNewWeightValueReceivedCallback(mScaleValue);
            }

            if (mNewWeightFilteredValueReceivedCallback != NULL)
            {
                mNewWeightFilteredValueReceivedCallback(mFilteredScaleValue);
            }

            break;
        }
        case START_TARING:
        {
            mTaringAttempts++;
            mTaringReadCount = 0;
            mTaringSum = 0;
            int32_t readValue;
            if (ADS123X_read(&scale, &readValue) == NoERROR)
            {
                mTaringSum += readValue;
                mTaringReadCount++;
            }

            mWeightSensorCurrentState = TARING;
            break;
        }
        case TARING:
        {
            int32_t readValue;
            if (ADS123X_read(&scale, &readValue) == NoERROR)
            {
                mTaringSum += readValue;
                mTaringReadCount++;
            }

            if (mTaringReadCount >= 20)
            {
                ADS123X_setOffset(&scale, mTaringSum/20);
                mWeightSensorCurrentState = VERIFY_TARE;
            }

            break;
        }
        case VERIFY_TARE:
        {    
            ADS123X_ERROR_t error = ADS123X_getUnits(&scale, &mScaleValue, 1);

            if(error == NoERROR && fabs(mScaleValue) < 0.02)
            {
                mWeightSensorCurrentState = NORMAL;
                mScaleValue = 0.0;
                mFilteredScaleValue = 0;
                mGramsPerSecond = 0;
                mGramsPerSecondFiltered = 0;
            }
            else if (error != NoERROR)
            {
                break;
            }
            else
            {
                mWeightSensorCurrentState = START_TARING;
            }

            break;
        }
        case START_CALIBRATION :
        {
            mCalibrationReadCount = 0;
            mCalibrationSum = 0;
            int32_t readValue;
            if (ADS123X_read(&scale, &readValue) == NoERROR)
            {
                mCalibrationSum += readValue;
                mCalibrationReadCount++;
            }

            mWeightSensorCurrentState = CALIBRATING;
            break;
        }
        case CALIBRATING:
        {
            int32_t readValue;
            if (ADS123X_read(&scale, &readValue) == NoERROR)
            {
                mCalibrationSum += readValue;
                mCalibrationReadCount++;
            }

            if (mCalibrationReadCount == 80)
            {
                float averageValue = mCalibrationSum/80.0;
                float scaleFactor = ((averageValue - ADS123X_getOffset(&scale))/50.0f);
                NRF_LOG_RAW_INFO("Scale Factor:%s%d.%01d\n" , NRF_LOG_FLOAT_SCALES(scaleFactor) );
                ADS123X_setScaleFactor(&scale, scaleFactor);
                
                mWeightSensorCurrentState = VERIFY_CALIBRATION;
            }

            break;
        }
        case VERIFY_CALIBRATION:
        {    
            ADS123X_ERROR_t error = ADS123X_getUnits(&scale, &mScaleValue, 1);

            if(error == NoERROR && mScaleValue < 50.02 && mScaleValue > 49.98)
            {
                float scaleFactor = ADS123X_getScaleFactor(&scale);

                mCalibrationCompleteCallback(scaleFactor);
                mWeightSensorCurrentState = NORMAL;
            }
            else if (error != NoERROR)
            {
                break;
            }
            else
            {
                mWeightSensorCurrentState = START_CALIBRATION;
            }

            break;
        }

        default:
            break;
    }
    
    mConversionCompleteCallback();

    nrf_drv_gpiote_in_event_enable(pin_DOUT, true);
}
                                            
void weight_sensor_init(weight_sensor_init_t ws_init)
{
    first_sample = 1;

    mNewWeightValueReceivedCallback = ws_init.newWeightValueReceivedCallback;
    mNewWeightFilteredValueReceivedCallback = ws_init.newWeightFilteredValueReceivedCallback;
    mConversionCompleteCallback = ws_init.conversionCompleteCallback;
    
    ret_code_t err_code;
    nrf_gpio_cfg_output(pin_APWR);
    nrf_gpio_pin_clear(pin_APWR);

        // GPIOTE init
    if (!nrfx_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    // Configure input pins for ADS1232, but don't enable interrupts yet
    in_config.pull = NRF_GPIO_PIN_NOPULL; 

    err_code = nrf_drv_gpiote_in_init(pin_DOUT, &in_config, weight_sensor_data_ready_handler);
    APP_ERROR_CHECK(err_code);
    // Don't enable event yet

    ADS123X_Init(&scale, pin_DOUT, pin_SCLK, pin_PWDN, pin_GAIN0, pin_GAIN1, pin_SPEED);

    ADS123X_setGain(&scale, GAIN_128);
    ADS123X_setSpeed(&scale, SPEED_80SPS);
    ADS123X_setScaleFactor(&scale, ws_init.scaleFactor);

    weight_sensor_sleep(&scale);

    mTaringAttempts = 0;
}

void weight_sensor_tare()
{
    mWeightSensorCurrentState = START_TARING;    
}

void weight_sensor_calibrate(void (*calibrationCompleteCallback)(float scaleFactor ))
{
    mCalibrationCompleteCallback = calibrationCompleteCallback;

    mWeightSensorCurrentState = START_CALIBRATION;
}

float weight_sensor_get_weight()
{
    return mScaleValue;
}

float weight_sensor_get_weight_filtered()
{
    if (mWeightSensorCurrentState != NORMAL)
    {
        return 888.8;
    }
    else
    {
        if (mFilteredScaleValue < 0.05 && mFilteredScaleValue > -0.05)
        {
            return 0.0;
        }

        return mFilteredScaleValue;
    } 
}

float weight_sensor_read_weight()
{
    return mScaleValue;
}

void weight_sensor_sleep()
{
    nrf_drv_gpiote_in_event_enable(pin_DOUT, false);
    ADS123X_PowerOff(&scale);
    nrf_gpio_pin_set(pin_APWR);
}

void weight_sensor_wakeup()
{
    nrf_gpio_pin_clear(pin_APWR);
    ADS123X_PowerOn(&scale);

    while (!ADS123X_IsReady(&scale)){}

    ads123x_timeout_handler(NULL);

    nrf_drv_gpiote_in_event_enable(pin_DOUT, true);

    mWeightSensorCurrentState = START_TARING;
}

void weight_sensor_enable_weight_change_sense(void (*weightSenseTriggeredCallback)(void))
{
    mWeightSensorSenseState = SENSING_WEIGHT_CHANGE;
    mSenseThresholdValue = mFilteredScaleValue + 0.5;
    mWeightMovedFromZeroCallback = weightSenseTriggeredCallback;
}

void weight_sensor_get_stable_weight(void (*stableWeightAcheivedCallback)(void))
{
    mStableWeightRequested = true;
    mStableWeightAcheivedCallback = stableWeightAcheivedCallback;
}

void weight_sensor_set_weight_filter_input_coefficient(float coefficient)
{
    if (coefficient < 1.0)
    {
        mWeightFilterInputCoefficient = coefficient;
        mWeightFilterOutputCoefficient = 1.0 - mWeightFilterInputCoefficient;
    }

    NRF_LOG_INFO("Filter Output Coefficient:%s%d.%02d\n" , NRF_LOG_FLOAT(mWeightFilterOutputCoefficient) );
}

void weight_sensor_set_weight_filter_output_coefficient(float coefficient)
{
    if (coefficient < 1.0)
    {
        mWeightFilterOutputCoefficient = coefficient;
        mWeightFilterInputCoefficient = 1.0 - mWeightFilterOutputCoefficient;
    }

    NRF_LOG_INFO("Filter Output Coefficient:%s%d.%02d\n" , NRF_LOG_FLOAT(mWeightFilterOutputCoefficient) );
}

uint16_t weight_sensor_get_sampling_rate()
{
    return mSamplesPerSecond;
}

uint16_t weight_sensor_get_taring_attempts()
{
    return mTaringAttempts;
}

void weight_sensor_data_ready_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
   ads123x_timeout_handler(NULL);
}

void weight_sensor_tick_inc(uint32_t tickPeriod)
{
    mTimeMS += tickPeriod;
}

float weight_sensor_get_grams_per_second()
{
    return mGramsPerSecondFiltered;
}