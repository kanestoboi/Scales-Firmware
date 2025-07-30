#include "biquad.h"

// Initialize a biquad with coefficients
void biquad_init(Biquad *filt, float b0, float b1, float b2, float a1, float a2) {
    filt->b0 = b0;
    filt->b1 = b1;
    filt->b2 = b2;
    filt->a1 = a1;
    filt->a2 = a2;
    filt->z1 = 0;
    filt->z2 = 0;
}

// Process one sample through one biquad section
float biquad_process(Biquad *filt, float in) {
    float out = filt->b0*in + filt->z1;
    filt->z1 = filt->b1*in - filt->a1*out + filt->z2;
    filt->z2 = filt->b2*in - filt->a2*out;
    return out;
}

// Process sample through cascaded biquads
float filter_process(Biquad *filt, int num_sections, float in) {
    float out = in;
    for (int i = 0; i < num_sections; i++) {
        out = biquad_process(&filt[i], out);
    }
    return out;
}

void biquad_reset(Biquad *filt, int num_sections) {
    for (int i = 0; i < num_sections; i++) {
        filt[i].b0 = 0.0f;
        filt[i].b1 = 0.0f;
        filt[i].b2 = 0.0f;
        filt[i].a1 = 0.0f;
        filt[i].a2 = 0.0f;
        filt[i].z1 = 0.0f;
        filt[i].z2 = 0.0f;
    }
}