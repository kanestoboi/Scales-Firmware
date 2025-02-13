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

uint16_t saved_parameters_getButton1CSenseThreshold();
void saved_parameters_setButton1CSenseThreshold(uint16_t threshold);
uint16_t saved_parameters_getButton2CSenseThreshold();
void saved_parameters_setButton2CSenseThreshold(uint16_t threshold);
uint16_t saved_parameters_getButton3CSenseThreshold();
void saved_parameters_setButton3CSenseThreshold(uint16_t threshold);
uint16_t saved_parameters_getButton4CSenseThreshold();
void saved_parameters_setButton4CSenseThreshold(uint16_t threshold);

float saved_parameters_getWeightFilterOutputCoefficient();
void saved_parameters_setWeightFilterOutputCoefficient(float coefficient);

uint8_t saved_parameters_getWeighMode();
void saved_parameters_setWeighMode(uint8_t mode);

#endif