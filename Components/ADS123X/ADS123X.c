#include "ADS123X.h"
#include "nrf_gpio.h"

// Initialize library

void ADS123X_Init(ADS123X *device, uint8_t pin_DOUT, uint8_t pin_SCLK, uint8_t pin_PDWN, uint8_t pin_GAIN0, uint8_t pin_GAIN1, uint8_t pin_SPEED)
{
  device->pin_DOUT = pin_DOUT;
  device->pin_SCLK = pin_SCLK;
  device->pin_PDWN = pin_PDWN;
  device->pin_GAIN0 = pin_GAIN0;
  device->pin_GAIN1 = pin_GAIN1;
  device->pin_SPEED = pin_SPEED;

  nrf_gpio_cfg_input(device->pin_DOUT, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_output(device->pin_SCLK);
  nrf_gpio_cfg_output(device->pin_PDWN);
  nrf_gpio_cfg_output(device->pin_GAIN0);
  nrf_gpio_cfg_output(device->pin_GAIN1);
  nrf_gpio_cfg_output(device->pin_SPEED);

  device->scaleFactor = 0.0f;
  device->offset = 0.0f;
  device->calibrateOnNextConversion = false;

  ADS123X_PowerOn(device);
}

bool ADS123X_IsReady(ADS123X *device)
{
  return nrf_gpio_pin_read(device->pin_DOUT) == 0U;
}

ADS123X_ERROR_t ADS123X_read(ADS123X *device, int32_t *value)
{
  while(!ADS123X_IsReady(device)) {}
  int32_t readValue = 0;
  
  // Read 24 bits
  for(int16_t i=23 ; i >= 0; i--) {
      nrf_gpio_pin_set(device->pin_SCLK);
      readValue = (readValue << 1) + nrf_gpio_pin_read(device->pin_DOUT);
      nrf_gpio_pin_clear(device->pin_SCLK);
  }

  // One last pulse on SCLK needed to ensure DOUT goes high at end of transmission
  nrf_gpio_pin_set(device->pin_SCLK);
  nrf_gpio_pin_clear(device->pin_SCLK);

  if (device->calibrateOnNextConversion)
  {
    nrf_gpio_pin_set(device->pin_SCLK);
    nrf_gpio_pin_clear(device->pin_SCLK);

    device->calibrateOnNextConversion = false;
  }

  *value &= 0xFFFFFFC0;
  //Bit 23 is the signed bit so shift left and divide by 256
  *value = (readValue << 8) / 256;

  return NoERROR;
}

void ADS123X_setGain(ADS123X *device, ADS123X_GAIN_t gain)
{
    switch (gain)
    {
        case GAIN_1:
        {
            nrf_gpio_pin_clear(device->pin_GAIN1);
            nrf_gpio_pin_clear(device->pin_GAIN0);
            break;
        }
        case GAIN_2:
        {
            nrf_gpio_pin_clear(device->pin_GAIN1);
            nrf_gpio_pin_set(device->pin_GAIN0);
            break;
        }
        case GAIN_64:
        {
            nrf_gpio_pin_set(device->pin_GAIN1);
            nrf_gpio_pin_clear(device->pin_GAIN0);
            break;
        }
        case GAIN_128:
        {
            nrf_gpio_pin_set(device->pin_GAIN1);
            nrf_gpio_pin_set(device->pin_GAIN0);
            break;
        }
    }
}

void ADS123X_setSpeed(ADS123X *device, ADS123X_SPEED_t speed)
{
    switch (speed)
    {
        case SPEED_10SPS:
        {
            nrf_gpio_pin_clear(device->pin_SPEED);
            break;
        }
        case SPEED_80SPS:
        {
            nrf_gpio_pin_set(device->pin_SPEED);
            break;
        }
    }
}

ADS123X_ERROR_t ADS123X_readAverage(ADS123X *device, float *value, uint8_t times)
{
  int32_t sum = 0;
  
  int32_t readValue;

  for(uint16_t index = 0; index < times; index++)
  {
    ADS123X_read(device, &readValue);

    sum += readValue;
  }

  *value = (float)sum/(float)times;

  return NoERROR;
}

void ADS123X_PowerOn(ADS123X *device)
{
  nrf_gpio_pin_set(device->pin_PDWN);
}

void ADS123X_PowerOff(ADS123X *device)
{
  nrf_gpio_pin_clear(device->pin_PDWN);
}

ADS123X_ERROR_t ADS123X_tare(ADS123X *device, uint8_t times)
{
  int32_t readValue = 0;

  int32_t sum = 0;

  for( uint32_t index = 0; index < times; index++)
  {
    ADS123X_read(device, &readValue);
    sum += readValue;
  }

  ADS123X_setOffset(device, sum/times);
  return NoERROR;
}

void ADS123X_setOffset(ADS123X *device, float offset)
{
  device->offset = offset;
}

float ADS123X_getOffset(ADS123X *device) {
  return device->offset;
}

void ADS123X_setScaleFactor(ADS123X *device, float scaleFactor)
{
  device->scaleFactor = scaleFactor;
}

float ADS123X_getScaleFactor(ADS123X *device)
{
  return device->scaleFactor;
}

ADS123X_ERROR_t ADS123X_getUnits(ADS123X *device, float *value, uint8_t times)
{
  float val = 0;
  ADS123X_ERROR_t err;
  
  err = ADS123X_getValue(device, &val, times);

  if(err!=NoERROR) return err;
  
  if(device->scaleFactor==0) 
    return DIVIDED_by_ZERO;
  
  *value = val / ADS123X_getScaleFactor(device);

  return NoERROR;
}

ADS123X_ERROR_t ADS123X_getValue(ADS123X *device, float *value, uint8_t times)
{
  float readValue;

  ADS123X_readAverage(device, &readValue, times);

  *value = readValue - ADS123X_getOffset(device);

  return NoERROR;
}

void  ADS123X_calibrateOnNextConversion(ADS123X *device)
{
  device->calibrateOnNextConversion = true;
}