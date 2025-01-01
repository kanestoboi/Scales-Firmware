#ifndef IQS227D_h
#define IQS227D_h

#include <stdbool.h>
#include <stdint.h>

#include "nrfx_twi.h"
#include "nrf_drv_gpiote.h"

#define IQS227D_AADDRESS    0x44

typedef struct IQS227D 
{
  uint8_t pin_POUT;
  uint8_t pin_TOUT;
  uint8_t pin_VCC;

  const nrfx_twi_t *mHandle;

  void (*toutChangedFcn)(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
  void (*poutChangedFcn)(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

} IQS227D;

void iqs227d_init(IQS227D *sensor, const nrfx_twi_t *m_twi);

void iqs227d_power_on(IQS227D *sensor);

#endif