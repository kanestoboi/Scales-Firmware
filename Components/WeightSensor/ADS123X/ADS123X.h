#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef ADS123X_h
#define ADS123X_h

typedef enum ADS123X_ERROR_t {
	NoERROR,
	TIMEOUT_HIGH,     // Timeout waiting for HIGH
	TIMEOUT_LOW,      // Timeout waiting for LOW
	WOULD_BLOCK,      // weight not measured, measuring takes too long
	STABLE_TIMEOUT,   // weight not stable within timeout
	DIVIDED_by_ZERO,
        SENSOR_NOT_READY,    
} ADS123X_ERROR_t;

typedef enum ADS123X_SPEED_t 
{
    SPEED_10SPS,
    SPEED_80SPS
} ADS123X_SPEED_t;

typedef enum ADS123X_GAIN_t 
{
    GAIN_1,
    GAIN_2,
    GAIN_64,
    GAIN_128
} ADS123X_GAIN_t;

typedef struct ADS123X 
{
  uint8_t pin_DOUT;
  uint8_t pin_SCLK;
  uint8_t pin_PDWN;
  uint8_t pin_GAIN0;
  uint8_t pin_GAIN1;
  uint8_t pin_SPEED;

  uint8_t gain;
  float scaleFactor;
  float offset;
  bool calibrateOnNextConversion;

} ADS123X;

// Initialize library
void ADS123X_Init(ADS123X *device, uint8_t pin_DOUT, uint8_t pin_SCLK, uint8_t pin_PDWN, uint8_t pin_GAIN0, uint8_t pin_GAIN1, uint8_t pin_SPEED);

// check if chip is ready
// from the datasheet: When output data is not ready for retrieval, digital output pin DOUT is high. Serial clock
// input PD_SCK should be low. When DOUT goes to low, it indicates data is ready for retrieval.
bool ADS123X_IsReady(ADS123X *device);

// waits for the chip to be ready and returns a reading
ADS123X_ERROR_t ADS123X_read(ADS123X *device, int32_t *value);

void ADS123X_setGain(ADS123X *device, ADS123X_GAIN_t gain);

void ADS123X_setSpeed(ADS123X *device, ADS123X_SPEED_t speed);

// returns an average reading; times = how many times to read
ADS123X_ERROR_t ADS123X_readAverage(ADS123X *device, float *value, uint8_t times);

// returns (read_average() - OFFSET), that is the current value without the tare weight; times = how many readings to do
ADS123X_ERROR_t ADS123X_getValue(ADS123X *device, float *value, uint8_t times);

// returns getValue() divided by scaleFactor, that is the raw value divided by a value obtained via calibration
// times = how many readings to do
ADS123X_ERROR_t ADS123X_getUnits(ADS123X *device, float *value, uint8_t times);

// set the OFFSET value for tare weight; times = how many times to read the tare value
ADS123X_ERROR_t ADS123X_tare(ADS123X *device, uint8_t times);

// set the scaleFactor value; this value is used to convert the raw data to "human readable" data (measure units)
void ADS123X_setScaleFactor(ADS123X *device, float scaleFactor);

// set OFFSET, the value that's subtracted from the actual reading (tare weight)
void ADS123X_setOffset(ADS123X *device, float offset);

float ADS123X_getOffset(ADS123X *device);

// puts the chip into power down mode
void ADS123X_PowerOff(ADS123X *device);

// wakes up the chip after power down mode
void ADS123X_PowerOn(ADS123X *device);

void ADS123X_calibrateOnNextConversion(ADS123X *device);

#endif /* #ifndef ADS123X_h */