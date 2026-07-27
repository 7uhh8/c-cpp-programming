#ifndef AUDIO_NUMERICS_H
#define AUDIO_NUMERICS_H

#include <fftw3.h>

void array_complex_multiplication(fftwf_complex *a, fftwf_complex *b, size_t size, fftwf_complex *result);

int next_power_of_2(int n);

#endif
