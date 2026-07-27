#include "audio_analysis.h"
#include "audio_numerics.h"
#include "read_audio_file.h"
#include "return_codes.h"

#include <stdbool.h>

int main(int argc, char *argv[])
{
	if (argc == 1 || argc > 3)
	{
		fputs("Expected one or two audio files\n", stderr);
		return ERROR_ARGUMENTS_INVALID;
	}

	float *audio_buffer1 = NULL, *audio_buffer2 = NULL, *audio_buffer_resampled = NULL;
	size_t num_of_samples1 = 0, num_of_samples2 = 0;
	int num_of_samples_resampled = 0;
	int sample_rate1, sample_rate2, sample_rate;
	int delta = 0, return_code = SUCCESS;

	if (argc == 2)
	{
		return_code = read_and_decode_file(argv[1], &audio_buffer1, &audio_buffer2, &sample_rate, &num_of_samples1, 2);
		if (return_code != SUCCESS)
			goto cleanup;

		return_code = audio_delay(num_of_samples1, num_of_samples1, audio_buffer1, audio_buffer2, &delta);
		if (return_code != SUCCESS)
			goto cleanup;
	}
	else
	{
		return_code = read_and_decode_file(argv[1], &audio_buffer1, NULL, &sample_rate1, &num_of_samples1, 1);
		if (return_code != SUCCESS)
			goto cleanup;

		return_code = read_and_decode_file(argv[2], &audio_buffer2, NULL, &sample_rate2, &num_of_samples2, 1);
		if (return_code != SUCCESS)
			goto cleanup;

		if (sample_rate1 != sample_rate2)
		{
			bool is_greater = sample_rate1 > sample_rate2;

			sample_rate = is_greater ? sample_rate1 : sample_rate2;

			return_code = resample(
				is_greater ? audio_buffer2 : audio_buffer1,
				is_greater ? sample_rate2 : sample_rate1,
				sample_rate,
				is_greater ? num_of_samples2 : num_of_samples1,
				&audio_buffer_resampled,
				&num_of_samples_resampled);

			if (return_code != SUCCESS)
				goto cleanup;

			return_code = audio_delay(
				is_greater ? num_of_samples1 : num_of_samples_resampled,
				is_greater ? num_of_samples_resampled : num_of_samples2,
				is_greater ? audio_buffer1 : audio_buffer_resampled,
				is_greater ? audio_buffer_resampled : audio_buffer2,
				&delta);

			if (return_code != SUCCESS)
				goto cleanup;
		}
		else
		{
			sample_rate = sample_rate1;
			return_code = audio_delay(num_of_samples1, num_of_samples2, audio_buffer1, audio_buffer2, &delta);
			if (return_code != SUCCESS)
				goto cleanup;
		}
	}

	printf("delta: %i samples\nsample rate: %i Hz\ndelta time: %i ms\n", delta, sample_rate, (int)((double)delta * 1000 / sample_rate));

cleanup:
	if (audio_buffer1)
		free(audio_buffer1);
	if (audio_buffer2)
		free(audio_buffer2);
	if (audio_buffer_resampled)
		free(audio_buffer_resampled);
	return return_code;
}
