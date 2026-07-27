#ifndef AUDIO_ANALYSIS_H
#define AUDIO_ANALYSIS_H

#include <corecrt.h>

int cross_correlation(float *stream1, float *stream2, size_t num_samples1, size_t num_samples2, float *result);

int audio_delay(size_t num_samples1, size_t num_samples2, float *audio_buffer1, float *audio_buffer2, int *result);

int resample(float *audio_input, int input_sample_rate, int output_sample_rate, int input_num_of_samples, float **audio_output, int *result_num_of_samples);

#endif
