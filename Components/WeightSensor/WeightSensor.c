#include "ADS123X/ADS123X.h"
#include "WeightSensor.h"
#include "nrf_log.h"
#include "app_timer.h"
#include "nrf_gpio.h"


#include <math.h>

ADS123X scale;

float mScaleValue;
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

APP_TIMER_DEF(m_read_ads123x_timer_id);

#define ADS123X_TIMER_INTERVAL_MS              12   // 12ms
#define ADS123X_TIMER_INTERVAL_TICKS           APP_TIMER_TICKS(ADS123X_TIMER_INTERVAL_MS)

static void start_read_ads123x_timer()
{
    ret_code_t err_code = app_timer_start(m_read_ads123x_timer_id, ADS123X_TIMER_INTERVAL_MS, NULL);
    APP_ERROR_CHECK(err_code);
}

static void stop_read_ads123x_timer()
{
    ret_code_t err_code = app_timer_stop(m_read_ads123x_timer_id);
    APP_ERROR_CHECK(err_code);
}

static void ads123x_timeout_handler(void * p_context)
{
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

            mFilteredScaleValue = mWeightFilterOutputCoefficient * mFilteredScaleValue + mWeightFilterInputCoefficient * mScaleValue;

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

    start_read_ads123x_timer();
}
                                            
void weight_sensor_init(float scaleFactor)
{    
    ret_code_t err_code = app_timer_create(&m_read_ads123x_timer_id, APP_TIMER_MODE_SINGLE_SHOT, ads123x_timeout_handler);
    APP_ERROR_CHECK(err_code);

    nrf_gpio_cfg_output(pin_APWR);
    nrf_gpio_pin_clear(pin_APWR);


    ADS123X_Init(&scale, pin_DOUT, pin_SCLK, pin_PWDN, pin_GAIN0, pin_GAIN1, pin_SPEED);

    ADS123X_setGain(&scale, GAIN_128);
    ADS123X_setSpeed(&scale, SPEED_80SPS);
    ADS123X_setScaleFactor(&scale, scaleFactor);

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
    nrf_gpio_pin_clear(pin_APWR);
    ADS123X_PowerOn(&scale);
    float readValue;
    ADS123X_getUnits(&scale, &readValue, 1); 
    ADS123X_PowerOff(&scale); 
    nrf_gpio_pin_set(pin_APWR);

    return readValue;
}

void weight_sensor_sleep()
{
    stop_read_ads123x_timer();
    ADS123X_PowerOff(&scale);
    nrf_gpio_pin_set(pin_APWR);
}

void weight_sensor_wakeup()
{
    nrf_gpio_pin_clear(pin_APWR);
    ADS123X_PowerOn(&scale);
    start_read_ads123x_timer();

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

uint16_t weight_sensor_get_taring_attempts()
{
    return mTaringAttempts;
}