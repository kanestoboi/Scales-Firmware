#ifndef NRF_BUDDY_LED_H
#define NRF_BUDDY_LED_H

#include "nrf_gpio.h"

#define NRF_BUDDY_LEDS_ACTIVE_STATE 1

#define NRF_BUDDY_LEDS_NUMBER 4

#define NRF_BUDDY_LED_1          NRF_GPIO_PIN_MAP(1,10)
#define NRF_BUDDY_LED_2          NRF_GPIO_PIN_MAP(1,11)
#define NRF_BUDDY_LED_3          NRF_GPIO_PIN_MAP(1,12)
#define NRF_BUDDY_LED_4          NRF_GPIO_PIN_MAP(0,13)

#define  NRF_BUDDY_LEDS_LIST { NRF_BUDDY_LED_1, NRF_BUDDY_LED_2,NRF_BUDDY_LED_3,  NRF_BUDDY_LED_4}

#define ADVERTISING_LED_ON_INTERVAL            50
#define ADVERTISING_LED_OFF_INTERVAL           4950

typedef enum
{
  NRF_BUDDY_INDICATE_IDLE,
  NRF_BUDDY_INDICATE_CONNECTED,
  NRF_BUDDY_INDICATE_ADVERTISING,
  NRF_BUDDY_INDICATE_DFU_INITIALIZED,
  NRF_BUDDY_INDICATE_DFU_TRANSPORT
} nrf_buddy_indication_t;

extern void nrf_buddy_leds_init();
extern void nrf_buddy_led_on(uint32_t led_idx);
extern void nrf_buddy_led_off(uint32_t led_idx);
extern void nrf_buddy_leds_off(void);
extern void nrf_buddy_leds_on(void);
extern void nrf_buddy_led_invert(uint32_t led_idx);
extern bool nrf_buddy_led_state_get(uint32_t led_idx);
extern uint32_t nrf_buddy_led_indication(nrf_buddy_indication_t indicate);

#endif /* NRF_BUDDY_LED_H */