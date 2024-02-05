#include "ADS123X/ADS123X.h"
#include "WeightSensor.h"
#include "nrf_log.h"
#include "app_timer.h"

ADS123X scale;

float mScaleValue;
float mFilteredScaleValue = 0;
float mRoundedValue;
int32_t mTaringSum = 0;
int32_t mTaringReadCount = 0;
int32_t mCalibrationSum = 0;
int32_t mCalibrationReadCount = 0;

weight_sensor_state_t mWeightSensorCurrentState = NORMAL;

const uint8_t pin_DOUT = 33;
const uint8_t pin_SCLK = 35;
const uint8_t pin_PWDN = 37;
const uint8_t pin_GAIN0 = 36;
const uint8_t pin_GAIN1 = 38;
const uint8_t pin_SPEED = 39;

void (*mCalibrationCompleteCallback)(float scaleFactor) = NULL;

APP_TIMER_DEF(m_read_ads123x__timer_id);

#define ADS123X_TIMER_INTERVAL_MS              12   // 12ms
#define ADS123X_TIMER_INTERVAL_TICKS           APP_TIMER_TICKS(ADS123X_TIMER_INTERVAL_MS)



static void start_read_ads123x__timer()
{
    ret_code_t err_code = app_timer_start(m_read_ads123x__timer_id, ADS123X_TIMER_INTERVAL_MS, NULL);
    APP_ERROR_CHECK(err_code);
}

static void stop_read_ads123x__timer()
{
    ret_code_t err_code = app_timer_stop(m_read_ads123x__timer_id);
    APP_ERROR_CHECK(err_code);
}

static void ads123x_timeout_handler(void * p_context)
{
    switch (mWeightSensorCurrentState)
    {
        case NORMAL:
        {
            ADS123X_getUnits(&scale, &mScaleValue, 1);
            mFilteredScaleValue = 0.5*mFilteredScaleValue + 0.5 * mScaleValue;
            break;
        }
        case START_TARING :
        {
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

            if (mTaringReadCount == 80)
            {
                ADS123X_setOffset(&scale, mTaringSum/80);
                mWeightSensorCurrentState = NORMAL;
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
                
                mWeightSensorCurrentState = NORMAL;

                mCalibrationCompleteCallback(scaleFactor);
            }

            break;
        }

        default:
            break;
    }
    

    start_read_ads123x__timer();
}
                                            
void weight_sensor_init(float scaleFactor)
{    
    ret_code_t err_code = app_timer_create(&m_read_ads123x__timer_id, APP_TIMER_MODE_SINGLE_SHOT, ads123x_timeout_handler);
    APP_ERROR_CHECK(err_code);

    ADS123X_Init(&scale, pin_DOUT, pin_SCLK, pin_PWDN, pin_GAIN0, pin_GAIN1, pin_SPEED);
    
    ADS123X_PowerOff(&scale);
    ADS123X_setGain(&scale, GAIN_128);
    ADS123X_setSpeed(&scale, SPEED_80SPS);

    ADS123X_PowerOn(&scale);

    ADS123X_setScaleFactor(&scale, scaleFactor);

    //ADS123X_calibrateOnNextConversion(&scale);
    weight_sensor_tare();
    start_read_ads123x__timer();    
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