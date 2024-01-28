#ifndef SAVED_PARAMETERS_H
#define SAVED_PARAMETERS_H

#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "fds.h"

/* File ID and Key used for the configuration record. */
#define CONFIG_FILE     (0x8010)
#define CONFIG_REC_KEY  (0x7010)


void saved_parameters_init();

float saved_parameters_getSavedScaleFactor();
void saved_parameters_SetSavedScaleFactor(float scaleFactor);

uint8_t saved_parameters_getCoffeeToWaterRatioNumerator();
void saved_parameters_setCoffeeToWaterRatioNumerator(uint8_t numerator);
uint8_t saved_parameters_getCoffeeToWaterRatioDenominator();
void saved_parameters_setCoffeeToWaterRatioDenominator(uint8_t denominator);

uint8_t saved_parameters_getWeighMode();
void saved_parameters_setWeighMode(uint8_t mode);

#endif