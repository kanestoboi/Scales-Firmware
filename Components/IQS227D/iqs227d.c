#include "Components/IQS227D/iqs227d.h"
#include "nrf_log.h"


#include "nrf_drv_gpiote.h"

#include <math.h>

static bool iqs227d_register_read(IQS227D *sensor, uint8_t register_address, uint8_t * destination, uint8_t number_of_bytes)
{
    ret_code_t err_code;
    
    // Send the Register address where we want to write the data
    err_code = nrfx_twi_tx(sensor->mHandle, IQS227D_AADDRESS, &register_address, 1, true);
    APP_ERROR_CHECK(err_code);
    
    while(nrfx_twi_is_busy(sensor->mHandle)){}

    // Receive the data from the MAX17260
    err_code = nrfx_twi_rx(sensor->mHandle, IQS227D_AADDRESS, destination, number_of_bytes);
    APP_ERROR_CHECK(err_code);

    if (err_code != NRF_SUCCESS) {
        // Handle the error here
        // for example print the error code
        NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }
	
    // if data was successfully read, return true else return false
    if (NRF_SUCCESS != err_code)
    {
        return false;
    }
    
    return true;
}
                                            
void iqs227d_init(IQS227D *sensor, const nrfx_twi_t *m_twi)
{    
    ret_code_t err_code;

    sensor->mHandle = m_twi;

    uint8_t bank0;
    
    //iqs227d_register_read(sensor, 0, &bank0, 1);

    // nrf_gpio_cfg_input(sensor->pin_POUT, NRF_GPIO_PIN_PULLUP);
    // nrf_gpio_cfg_input(sensor->pin_TOUT, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_output(sensor->pin_VCC);
    nrf_gpio_pin_set(sensor->pin_VCC);

    if (!nrfx_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }
    nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_NOPULL;
  
    if (sensor->toutChangedFcn != NULL)
    {
        err_code = nrf_drv_gpiote_in_init(sensor->pin_TOUT, &in_config, sensor->toutChangedFcn);
        APP_ERROR_CHECK(err_code);

        nrf_drv_gpiote_in_event_enable(sensor->pin_TOUT, true);
    }

    if (sensor->poutChangedFcn != NULL)
    {
        err_code = nrf_drv_gpiote_in_init(sensor->pin_POUT, &in_config, sensor->poutChangedFcn);
        APP_ERROR_CHECK(err_code);

        //nrf_drv_gpiote_in_event_enable(sensor->pin_POUT, true);
    }
    

    NRF_LOG_INFO("iqs227d initialised");
}


