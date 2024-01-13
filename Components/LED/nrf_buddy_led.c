#include "nrf_buddy_led.h"
#include "app_timer.h"

#include <stdint.h>
#include <stdbool.h>

static nrf_buddy_indication_t m_stable_state = NRF_BUDDY_INDICATE_IDLE;
static const uint8_t m_nrf_buddy_led_list[NRF_BUDDY_LEDS_NUMBER] = NRF_BUDDY_LEDS_LIST;

APP_TIMER_DEF(m_nrf_buddy_led_tmr);

/**@brief Handle events from leds timer.
 *
 * @note Timer handler does not support returning an error code.
 * Errors from bsp_led_indication() are not propagated.
 *
 * @param[in]   p_context   parameter registered in timer start function.
 */
static void nrf_buddy_led_timer_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    UNUSED_VARIABLE(nrf_buddy_led_indication(m_stable_state));
}

void nrf_buddy_leds_init()
{
  uint32_t i;
  for (i = 0; i < NRF_BUDDY_LEDS_NUMBER; ++i)
  {
      nrf_gpio_cfg_output(m_nrf_buddy_led_list[i]);
  }

  ret_code_t err_code = app_timer_create(&m_nrf_buddy_led_tmr, APP_TIMER_MODE_SINGLE_SHOT, nrf_buddy_led_timer_handler);
}

void nrf_buddy_led_on(uint32_t led_idx)
{
        ASSERT(led_idx < NRF_BUDDY_LEDS_NUMBER);
        nrf_gpio_pin_write(m_nrf_buddy_led_list[led_idx], NRF_BUDDY_LEDS_ACTIVE_STATE ? 1 : 0);
}

void nrf_buddy_led_off(uint32_t led_idx)
{
    ASSERT(led_idx < NRF_BUDDY_LEDS_NUMBER);
    nrf_gpio_pin_write(m_nrf_buddy_led_list[led_idx], NRF_BUDDY_LEDS_ACTIVE_STATE ? 0 : 1);
}

void nrf_buddy_leds_off(void)
{
    uint32_t i;
    for (i = 0; i < NRF_BUDDY_LEDS_NUMBER; ++i)
    {
        nrf_buddy_led_off(i);
    }
}

void nrf_buddy_leds_on(void)
{
    uint32_t i;
    for (i = 0; i < NRF_BUDDY_LEDS_NUMBER; ++i)
    {
        nrf_buddy_led_on(i);
    }
}

void nrf_buddy_led_invert(uint32_t led_idx)
{
    ASSERT(led_idx < NRF_BUDDY_LEDS_NUMBER);
    nrf_gpio_pin_toggle(m_nrf_buddy_led_list[led_idx]);
}

uint32_t nrf_buddy_led_indication(nrf_buddy_indication_t indicate)
{
  ret_code_t err_code;
  uint32_t next_delay;
  switch (indicate)
    {
        case NRF_BUDDY_INDICATE_IDLE:
            nrf_buddy_leds_off();
            err_code       = app_timer_stop(m_nrf_buddy_led_tmr);
            m_stable_state = indicate;
            break;

        case NRF_BUDDY_INDICATE_ADVERTISING:
            // in advertising blink LED_0
            nrf_buddy_led_invert(0);

                        // in advertising blink LED_0
            if (nrf_buddy_led_state_get(0))
            {
                nrf_buddy_led_on(0);
                next_delay = ADVERTISING_LED_ON_INTERVAL;
            }
            else
            {
                nrf_buddy_led_off(0);
                next_delay = ADVERTISING_LED_OFF_INTERVAL;
            }

            m_stable_state = indicate;
            err_code       = app_timer_start(m_nrf_buddy_led_tmr, APP_TIMER_TICKS(next_delay), NULL);
            break;

         case NRF_BUDDY_INDICATE_CONNECTED:
            nrf_buddy_led_on(0);
            m_stable_state = indicate;
            break;

         case NRF_BUDDY_INDICATE_DFU_INITIALIZED:
            // in advertising blink LED_0
            nrf_buddy_led_on(2);

            m_stable_state = indicate;
            break;

         case NRF_BUDDY_INDICATE_DFU_TRANSPORT:
            // in advertising blink LED_0
            nrf_buddy_led_invert(0);

            m_stable_state = indicate;
            err_code       = app_timer_start(m_nrf_buddy_led_tmr, APP_TIMER_TICKS(100), NULL);
            break;
            
    }

    return err_code;
}

bool nrf_buddy_led_state_get(uint32_t led_idx)
{
    ASSERT(led_idx < NRF_BUDDY_LEDS_NUMBER);
    bool pin_set = nrf_gpio_pin_out_read(m_nrf_buddy_led_list[led_idx]) ? true : false;
    return (pin_set == (NRF_BUDDY_LEDS_ACTIVE_STATE ? true : false));
}

