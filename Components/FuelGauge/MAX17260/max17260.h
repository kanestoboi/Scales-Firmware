#ifndef MAX17260_H__
#define MAX17260_H__

#include "nrfx_twi.h"

#define MAX17260_ADDRESS_LEN  1         //MAX17260
#define MAX17260_ADDRESS     0x36       //MAX17260 Device Address


// ModelGauge m5 EZ Configuration Registers
#define MAX17260_DESIGN_CAP_REG       0x18
#define MAX17260_V_EMPTY_REG          0x3A
#define MAX17260_MODEL_CONFIG_REG     0xDB
#define MAX17260_I_CHG_TERM_REG            0x1E
#define MAX17260_CONFIG_REG             0x1D
#define MAX17260_CONFIG_2_REG           0xBB

// ModelGauge m5 Algorithm Output Registers
#define MAX17260_REP_CAP_REG            0x05
#define MAX17260_REP_SOC_REG            0x06
#define MAX17260_FULL_CAP_REP_REG       0x10
#define MAX17260_TTE_REG                0x11    // time to empty
#define MAX17260_TTF_REG                0x20    // time to full
#define MAX17260_CYCLES_REG             0x17
#define MAX17260_STATUS_REG             0x00

// Voltage measurement registers
#define MAX17260_V_CELL_REG         0x09
#define MAX17260_AVG_V_CELL_REG     0x19 
#define MAX17260_MAX_MIN_VOLT_REG   0x1B

// Current measurement registers
#define MAX17260_CURRENT_REG            0x0A
#define MAX17260_AVG_CURRENT_REG        0x0B
#define MAX17260_MAX_MIN_CURRENT_REG    0x1C

// Temperature measurement registers
#define MAX17260_TEMP_REG               0x08
#define MAX17260_AVG_TEMP_REG           0x16
#define MAX17260_MAX_MIN_TEMP_REG       0x1A
#define MAX17260_DIE_TEMP_REG           0x34


#define MAX17260_SOC_HOLD_REG     0xD3
#define MAX17260_SC_OCV_LIM_REG   0xD1
#define MAX17260_CURVE_REG        0xB9
#define MAX17260_CG_TEMP_CO_REG      0xB8
#define MAX17260_AIN_REG          0x27
#define MAX17260_TEMP_GAIN_REG    0x2C
#define MAX17260_TEMP_OFF_REG       0x2D
#define MAX17260_MPP_CURRENT_REG    0xD9
#define MAX17260_SPP_CURRENT_REG    0xDA
#define MAX17260_MAX_PEAK_POWER_REG 0xD4
#define MAX17260_SUS_PEAK_POWER_REG 0xD5
#define MAX17260_PEAK_RESISTANCE_REG  0xD6
#define MAX17260_SYS_RESISTANCE_REG 0xD7
#define MAX17260_MIN_SYS_VOLTAGE_REG 0xD8
#define MAX17260_R_GAIN_REG          0x43




#define MAX17260_FULL_SOC_THR_REG     0x13
#define MAX17260_OCV_TABLE_0_REG      0x80
#define MAX17260_OCV_TABLE_1_REG      0x81
#define MAX17260_OCV_TABLE_2_REG      0x82
#define MAX17260_OCV_TABLE_3_REG      0x83
#define MAX17260_OCV_TABLE_4_REG      0x84
#define MAX17260_OCV_TABLE_5_REG      0x85
#define MAX17260_OCV_TABLE_6_REG      0x86
#define MAX17260_OCV_TABLE_7_REG      0x87
#define MAX17260_OCV_TABLE_8_REG      0x88
#define MAX17260_OCV_TABLE_9_REG      0x89
#define MAX17260_OCV_TABLE_10_REG      0x8A
#define MAX17260_OCV_TABLE_11_REG      0x8B
#define MAX17260_OCV_TABLE_12_REG      0x8C
#define MAX17260_OCV_TABLE_13_REG      0x8D
#define MAX17260_OCV_TABLE_14_REG      0x8E
#define MAX17260_OCV_TABLE_15_REG      0x8F
#define MAX17260_X_TABLE_0_REG         0x90
#define MAX17260_X_TABLE_1_REG         0x91
#define MAX17260_X_TABLE_2_REG         0x92
#define MAX17260_X_TABLE_3_REG         0x93
#define MAX17260_X_TABLE_4_REG         0x94
#define MAX17260_X_TABLE_5_REG         0x95
#define MAX17260_X_TABLE_6_REG         0x96
#define MAX17260_X_TABLE_7_REG         0x97
#define MAX17260_X_TABLE_8_REG         0x98
#define MAX17260_X_TABLE_9_REG         0x99
#define MAX17260_X_TABLE_10_REG         0x9A
#define MAX17260_X_TABLE_11_REG         0x9B
#define MAX17260_X_TABLE_12_REG         0x9C
#define MAX17260_X_TABLE_13_REG         0x9D
#define MAX17260_X_TABLE_14_REG         0x9E
#define MAX17260_X_TABLE_15_REG         0x9F


#define MAX17260_QR_TABLE_00_REG        0x12

// Scale Factors defined in the MAX17260 datasheet page 16
#define MAX17260_R_SENSE                    0.010f   // 10 milliohm current sense resistor
#define MAX17260_CAPACITY_SCALE_FACTOR      (0.000005f / MAX17260_R_SENSE) // milli-amphours
#define MAX17260_PERCENTAGE_SCALE_FACTOR    (1.0f/256.0f)
#define MAX17260_VOLTAGE_SCALE_FACTOR       0.000078125f
#define MAX17260_CURRENT_SCALE_FACTOR       (0.0000015625f /  MAX17260_R_SENSE)
#define MAX17260_TEMPERATURE_SCALE_FACTOR   (1.0f/256.0f)
#define MAX17260_RESISTANCE_SCALE_FACTOR    (1.0f/4056.0f)
#define MAX17260_TIME_SCALE_FACTOR          5.625f          // (Seconds)


typedef struct MAX17260 
{
  const nrfx_twi_t *mHandle;
  bool initialised;
} MAX17260;

bool max17260_init(MAX17260 *sensor, const nrfx_twi_t *m_twi);

/**
  @brief Function for writing a MAX17260 register contents over TWI.
  @param[in] sensor Pointer to MAX17260 structure
  @param[in] register_address adress of register to write to
  @param[in] value Value to write into register_address
  @retval true write succeeded
  @retval false write failed
*/
bool max17260_register_write(MAX17260 *sensor, uint8_t register_address, const uint8_t value);

bool max17260_register_write16(MAX17260 *sensor, uint8_t register_address, uint16_t value);

/**
  @brief Function for reading MAX17260 register contents over TWI.
  Reads one or more consecutive registers.
  @param[in] sensor Pointer to MAX17260 structure
  @param[in]  register_address Register address to start reading from
  @param[in]  number_of_bytes Number of bytes to read
  @param[out] destination Pointer to a data buffer where read data will be stored
  @retval true Register read succeeded
  @retval false Register read failed
*/
bool max17260_register_read(MAX17260 *sensor, uint8_t register_address, uint8_t *destination, uint8_t number_of_bytes);

bool max17260_setDesignCapacity(MAX17260 *sensor, float capacity);

bool max17260_getDesignCapacity(MAX17260 *sensor, float *capacity);

bool max17260_setVoltageEmpty(MAX17260 *sensor, float voltage);

bool max17260_getVoltageEmpty(MAX17260 *sensor, float *voltage);

bool max17260_setTerminationChargeCurrent(MAX17260 *sensor, float current);

bool max17260_getTerminationChargeCurrent(MAX17260 *sensor, float *current);

bool max17260_getStatusRegisterValue(MAX17260 *sensor, uint16_t *status);

bool max17260_getPowerOnReset(MAX17260 *sensor, uint16_t *por);

bool max17260_getCellVoltage(MAX17260 *sensor, float *destination);

bool max17260_getAverageCellVoltage(MAX17260 *sensor, float *destination);

bool max17260_getCurrent(MAX17260 *sensor, float *destination);

bool max17260_getAvgCurrent(MAX17260 *sensor, float *destination);

bool max17260_getTemperature(MAX17260 *sensor, float *destination);

bool max17260_getStateOfCharge(MAX17260 *sensor, float *destination);

bool max17260_getTimeToFull(MAX17260 *sensor, float *destination);

bool max17260_getTimeToEmpty(MAX17260 *sensor, float *destination);

bool max17260_getRemainingCapacity(MAX17260 *sensor, float *destination);

bool max17260_getFullCapacity(MAX17260 *sensor, float *destination);

bool max17260_getCycles(MAX17260 *sensor, float *destination);


#endif
