#include <stdbool.h>
#include <stdint.h>

#ifndef BIQUAD_h
#define BIQUAD_h

typedef struct {
    float b0, b1, b2;
    float a1, a2;
    float z1, z2;
} __attribute__((aligned(4))) Biquad;

// Initialize a biquad with coefficients
void biquad_init(Biquad *filt, float b0, float b1, float b2, float a1, float a2);

// Process one sample through one biquad section
float biquad_process(Biquad *filt, float in);

// Process sample through cascaded biquads
float filter_process(Biquad *filt, int num_sections, float in);

void biquad_reset(Biquad *filt, int num_sections);

#endif