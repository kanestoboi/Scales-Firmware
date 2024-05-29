#include "stdint.h"

// Define constants for float32 and float16
#define FLOAT32_SIGN_MASK  0x80000000
#define FLOAT32_EXP_MASK   0x7F800000
#define FLOAT32_MANT_MASK  0x007FFFFF
#define FLOAT32_EXP_BIAS   127
#define FLOAT16_SIGN_MASK  0x8000
#define FLOAT16_EXP_MASK   0x7C00
#define FLOAT16_MANT_MASK  0x03FF
#define FLOAT16_EXP_BIAS   15

/**@brief SFLOAT format (IEEE-11073 16-bit FLOAT, defined as a 16-bit vlue with 12-bit mantissa and
 *        4-bit exponent. */
typedef struct
{
  int8_t  exponent;                                                         /**< Base 10 exponent, only 4 bits */
  int16_t mantissa;                                                         /**< Mantissa, only 12 bits */
} ieee_float16_t;

ieee_float16_t float32_to_float16(float f);
float float16_to_float32(ieee_float16_t sfloat);