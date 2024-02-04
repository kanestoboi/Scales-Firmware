#ifndef WEIGHT_SENSOR_h
#define WEIGHT_SENSOR_h

typedef enum  
{
    NORMAL,
    START_TARING,
    TARING,
    START_CALIBRATION,
    CALIBRATING,
} weight_sensor_state_t;

void weight_sensor_init(float scaleFactor);
float weight_sensor_get_weight();
float weight_sensor_get_weight_filtered();
void weight_sensor_tare();
void weight_sensor_calibrate(void (*calibrationCompleteCallback)(float scaleFactor ));

#endif