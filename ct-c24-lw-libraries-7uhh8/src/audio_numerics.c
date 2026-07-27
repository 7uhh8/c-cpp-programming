#include "audio_numerics.h"

void array_complex_multiplication(fftwf_complex *a, fftwf_complex *b, size_t size, fftwf_complex *result)
// a * conj(b)
{
	for (size_t i = 0; i < size; i++)
	{
		result[i][0] = a[i][0] * b[i][0] + a[i][1] * b[i][1];
		result[i][1] = a[i][1] * b[i][0] - a[i][0] * b[i][1];
	}
}

int next_power_of_2(int n)
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;
	return n;
}
