#include "stdint.h"
#include "sfloat.h"
#include <math.h>

ieee_float16_t float32_to_float16(float input) 
{
    ieee_float16_t result;

    // Handle the case for zero input
    if (input == 0.0f) {
        result.exponent = 0;
        result.mantissa = 0;
        return result;
    }

    // Calculate the base-10 exponent and normalized mantissa
    int16_t exp = (int16_t)floor(log10f(fabsf(input)));
    float normalized = input / powf(10, (int16_t)exp);

    // Scale the mantissa to fit into 12 bits (considering range -2048 to 2047)
    int16_t mantissa = (int16_t)(normalized * 2048.0f);

    // Ensure the exponent fits into 4 bits (range -8 to 7)
    if (exp < -8) {
        exp = -8;
    } else if (exp > 7) {
        exp = 7;
    }

    // Pack the exponent and mantissa into the struct
    result.exponent = (int8_t)exp;
    result.mantissa = mantissa;

    return result;
}

// Function to convert ieee_float16_t back to float
float float16_to_float32(ieee_float16_t sfloat)
{
    // Calculate the normalized mantissa
    float normalized = sfloat.mantissa / 2048.0f;

    // Reconstruct the original float value
    float result = normalized * powf(10, sfloat.exponent);

    return result;
}