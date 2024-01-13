
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrfx_twi.h"
#include "max17260.h"
#include "nrf_delay.h"

float DesignCap = 0.5; // 500 mAh
float IchgTerm = 0.01;   // 10 mA
float VEmpty = 2.9;     // 2.9V
/*
  Function to initialize the MAX17260
*/ 
bool max17260_init(MAX17260 *sensor, const nrfx_twi_t *m_twi)
{   
    sensor->mHandle = m_twi;
    sensor->mTransferDone = false;
    sensor->initialised = false;

    uint16_t statusRegisterValue;

    uint16_t statusPOR;

    if (!max17260_getPowerOnReset(sensor, &statusPOR))
    {
        return false;
    }

    if (statusPOR == 1)
    {
       uint16_t socHold = 1;

       // After power-up, wait for the ICto complete its startup operations
        //10ms Wait Loop. Do not continue until FSTAT.DNR==0
        while(socHold & 0x01)
        {
            max17260_register_read(sensor, 0x3D, (uint8_t*)&socHold, 2);
            nrf_delay_ms(10);
        }

       uint16_t HibCFG;
       max17260_register_read(sensor, 0xBA, (uint8_t*)&HibCFG, 2);

       max17260_register_write16(sensor, 0x60, 0x90);

       max17260_setDesignCapacity(sensor, DesignCap);
       max17260_setTerminationChargeCurrent(sensor, IchgTerm);
        max17260_setVoltageEmpty(sensor, VEmpty);

        uint16_t ModelCFGRegValue = 0x8000;
        max17260_register_write16(sensor, MAX17260_MODEL_CONFIG_REG, ModelCFGRegValue);

        // Reset the m5 EZ model
       //Poll ModelCFG.Refresh(highest bit),
       //proceed to Step 3 when ModelCFG.Refresh=0.
       while(ModelCFGRegValue & 0x8000)
       {
           max17260_register_read(sensor, MAX17260_MODEL_CONFIG_REG, (uint8_t*)&ModelCFGRegValue, 2);
           nrf_delay_ms(10);
       }

       max17260_register_read(sensor, MAX17260_STATUS_REG, (uint8_t*)&statusRegisterValue, 2);
       max17260_register_write16(sensor, MAX17260_STATUS_REG, statusRegisterValue & 0xFFFD);

       max17260_register_write16(sensor, 0x60, 0x00);
       max17260_register_write16(sensor, 0xBA , HibCFG)  ; // Restore Original HibCFGvalue
    }

    return true;
}

bool max17260_register_write(MAX17260 *sensor, uint8_t register_address, uint8_t value)
{
    ret_code_t err_code;
    uint8_t tx_buf[MAX17260_ADDRESS_LEN+1];
	
    //Write the register address and data into transmit buffer
    tx_buf[0] = register_address;
    tx_buf[1] = value;

    //Set the flag to false to show the transmission is not yet completed
    sensor->mTransferDone = false;
    
    //Transmit the data over TWI Bus
    err_code = nrfx_twi_tx(sensor->mHandle, MAX17260_ADDRESS, tx_buf, MAX17260_ADDRESS_LEN+1, false);
    APP_ERROR_CHECK(err_code);

    // if there is no error then return true else return false
    if (NRF_SUCCESS != err_code)
    {
        return false;
    }
    
    //Wait until the transmission of the data is finished
    while (sensor->mTransferDone == false) {}

    
    
    return true;	
}

bool max17260_register_write16(MAX17260 *sensor, uint8_t register_address, uint16_t value)
{
    ret_code_t err_code;
    uint8_t tx_buf[MAX17260_ADDRESS_LEN+2];
	
    //Write the register address and data into transmit buffer
    tx_buf[0] = register_address;
    tx_buf[1] = (uint8_t)(value & 0xFF);
    tx_buf[2] = (uint8_t)(value >> 8);


    //Set the flag to false to show the transmission is not yet completed
    sensor->mTransferDone = false;
    
    //Transmit the data over TWI Bus
    err_code = nrfx_twi_tx(sensor->mHandle, MAX17260_ADDRESS, tx_buf, MAX17260_ADDRESS_LEN+2, false);
    APP_ERROR_CHECK(err_code);

    // if there is no error then return true else return false
    if (NRF_SUCCESS != err_code)
    {
        return false;
    }
    
    //Wait until the transmission of the data is finished
    while (sensor->mTransferDone == false) {}

    
    
    return true;	
}


bool max17260_register_read(MAX17260 *sensor, uint8_t register_address, uint8_t * destination, uint8_t number_of_bytes)
{
    ret_code_t err_code;

    //Set the flag to false to show the receiving is not yet completed
    sensor->mTransferDone = false;
    
    // Send the Register address where we want to write the data
    err_code = nrfx_twi_tx(sensor->mHandle, MAX17260_ADDRESS, &register_address, 1, true);
    APP_ERROR_CHECK(err_code);

    //Wait for the transmission to get completed
    //while (sensor->mTransferDone == false){}
    
    while(nrfx_twi_is_busy(sensor->mHandle)){}


    //set the flag again so that we can read data from the MAX17260's internal register
    sensor->mTransferDone = false;
	  
    // Receive the data from the MAX17260
    err_code = nrfx_twi_rx(sensor->mHandle, MAX17260_ADDRESS, destination, number_of_bytes);
    APP_ERROR_CHECK(err_code);

    if (err_code != NRF_SUCCESS) {
        // Handle the error here
        // for example print the error code
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    //wait until the transmission is completed
    while (sensor->mTransferDone == false){}
	
    // if data was successfully read, return true else return false
    if (NRF_SUCCESS != err_code)
    {
        return false;
    }
    
    return true;
}

bool max17260_setDesignCapacity(MAX17260 *sensor, float capacity)
{
    uint16_t designCapacityRegisterValue = (uint16_t)(capacity / MAX17260_CAPACITY_SCALE_FACTOR);
    return max17260_register_write16(sensor, MAX17260_DESIGN_CAP_REG, designCapacityRegisterValue);
}

bool max17260_getDesignCapacity(MAX17260 *sensor, float *capacity)
{
    uint16_t designCapacityRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_DESIGN_CAP_REG, (uint8_t*)&designCapacityRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *capacity = (float)(designCapacityRegisterValue) * MAX17260_CAPACITY_SCALE_FACTOR;

    return true;
}

bool max17260_setVoltageEmpty(MAX17260 *sensor, float voltage)
{
    uint16_t voltageEmptyRegisterValue = (uint16_t)(voltage / MAX17260_VOLTAGE_SCALE_FACTOR);
    return max17260_register_write16(sensor, MAX17260_V_EMPTY_REG, voltageEmptyRegisterValue);
}

bool max17260_getVoltageEmpty(MAX17260 *sensor, float *voltage)
{
    uint16_t voltageEmptyRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_V_EMPTY_REG, (uint8_t*)&voltageEmptyRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *voltage = (float)(voltageEmptyRegisterValue) * MAX17260_VOLTAGE_SCALE_FACTOR;

    return true;
}

bool max17260_setTerminationChargeCurrent(MAX17260 *sensor, float current)
{
    uint16_t chargeTerminationCurrentRegisterValue = (uint16_t)(current / MAX17260_CURRENT_SCALE_FACTOR);
    return max17260_register_write16(sensor, MAX17260_I_CHG_TERM_REG, chargeTerminationCurrentRegisterValue);
}

bool max17260_getTerminationChargeCurrent(MAX17260 *sensor, float *current)
{
    uint16_t chargeTerminationCurrentRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_I_CHG_TERM_REG, (uint8_t*)&chargeTerminationCurrentRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *current = (float)(chargeTerminationCurrentRegisterValue) * MAX17260_CURRENT_SCALE_FACTOR;

    return true;
}

bool max17260_getStatusRegisterValue(MAX17260 *sensor, uint16_t *status)
{
    uint16_t statusRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_STATUS_REG, (uint8_t*)&statusRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *status = statusRegisterValue;

    return true;
}

bool max17260_getPowerOnReset(MAX17260 *sensor, uint16_t *por)
{
    uint16_t statusRegisterValue;

    bool err_code = max17260_getStatusRegisterValue(sensor, &statusRegisterValue);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *por = (statusRegisterValue & 0x0002) == 0x0002;

    return true;
}

bool max17260_getCellVoltage(MAX17260 *sensor, float *destination)
{
    uint16_t voltageRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_V_CELL_REG, (uint8_t*)&voltageRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *destination = (float)(voltageRegisterValue) * MAX17260_VOLTAGE_SCALE_FACTOR;

    return true;
}

bool max17260_getAverageCellVoltage(MAX17260 *sensor, float *destination)
{
    uint16_t voltageRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_AVG_V_CELL_REG, (uint8_t*)&voltageRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *destination = (float)(voltageRegisterValue) * MAX17260_VOLTAGE_SCALE_FACTOR;

    return true;
}

bool max17260_getCurrent(MAX17260 *sensor, float *destination)
{
    int16_t currentRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_CURRENT_REG, (uint8_t*)&currentRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *destination = (float)(currentRegisterValue) * MAX17260_CURRENT_SCALE_FACTOR;

    return true;
}

bool max17260_getTemperature(MAX17260 *sensor, float *destination)
{
    int16_t temperatureRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_TEMP_REG, (int8_t*)&temperatureRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *destination = (float)(temperatureRegisterValue) * MAX17260_TEMPERATURE_SCALE_FACTOR;

    return true;
}


bool max17260_getStateOfCharge(MAX17260 *sensor, float *destination)
{
    uint16_t RepSOCRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_REP_SOC_REG, (uint8_t*)&RepSOCRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *destination = (float)(RepSOCRegisterValue) * MAX17260_PERCENTAGE_SCALE_FACTOR;

    return true;
}

bool max17260_getTimeToFull(MAX17260 *sensor, float *destination)
{
    uint16_t RepTTFRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_TTF_REG, (uint8_t*)&RepTTFRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *destination = (float)(RepTTFRegisterValue) * MAX17260_TIME_SCALE_FACTOR;

    return true;
}

bool max17260_getTimeToEmpty(MAX17260 *sensor, float *destination)
{
    uint16_t RepTTERegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_TTE_REG, (uint8_t*)&RepTTERegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *destination = (float)(RepTTERegisterValue) * MAX17260_TIME_SCALE_FACTOR;

    return true;
}

bool max17260_getRemainingCapacity(MAX17260 *sensor, float *destination)
{
    uint16_t RepCapRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_REP_CAP_REG, (uint8_t*)&RepCapRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *destination = (float)(RepCapRegisterValue) * MAX17260_CAPACITY_SCALE_FACTOR;

    return true;
}

bool max17260_getFullCapacity(MAX17260 *sensor, float *destination)
{
    uint16_t FullCapacityRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_FULL_CAP_REP_REG, (uint8_t*)&FullCapacityRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *destination = (float)(FullCapacityRegisterValue) * MAX17260_CAPACITY_SCALE_FACTOR;

    return true;
}


bool max17260_getCycles(MAX17260 *sensor, float *destination)
{
    uint16_t cyclesRegisterValue;

    bool err_code = max17260_register_read(sensor, MAX17260_CYCLES_REG, (uint8_t*)&cyclesRegisterValue, 2);

    if (err_code != true) {
        // NRF_LOG_INFO("Error code: %d\n", err_code);
        return false;
    }

    *destination = (float)(cyclesRegisterValue) / 100.0;

    return true;

}