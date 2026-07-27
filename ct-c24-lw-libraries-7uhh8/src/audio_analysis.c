#include "audio_analysis.h"

#include "audio_numerics.h"
#include "read_audio_file.h"
#include "return_codes.h"
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>

#include <fftw3.h>

int cross_correlation(float *stream1, float *stream2, size_t num_samples1, size_t num_samples2, float *result)
{
	fftwf_complex *joint_inputs = NULL, *out = NULL;
	float *zero_padded_streams = NULL;
	fftwf_plan forward_plan = NULL, inverse_plan = NULL;

	int ret = SUCCESS;

	size_t size = next_power_of_2(num_samples1 + num_samples2 - 1);
	size_t complex_size = (size >> 1) + 1;
	size_t total_input_size = 2 * complex_size;

	joint_inputs = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * total_input_size);
	out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * complex_size);

	if (joint_inputs == NULL || out == NULL)
	{
		fputs("Unable to allocate memory for FFTW-arrays\n", stderr);
		ret = ERROR_NOTENOUGH_MEMORY;
		goto cleanup;
	}

	fftwf_complex *input_1 = joint_inputs;
	fftwf_complex *input_2 = joint_inputs + complex_size;

	zero_padded_streams = (float *)calloc(2 * size, sizeof(float));
	if (zero_padded_streams == NULL)
	{
		fputs("Unable to allocate memory for the streams\n", stderr);
		ret = ERROR_NOTENOUGH_MEMORY;
		goto cleanup;
	}

	float *zero_padded_stream1 = zero_padded_streams;
	float *zero_padded_stream2 = zero_padded_streams + size;

	memcpy(zero_padded_stream1, stream1, num_samples1 * sizeof(float));
	memcpy(zero_padded_stream2, stream2, num_samples2 * sizeof(float));

	forward_plan = fftwf_plan_dft_r2c_1d(size, zero_padded_stream1, input_1, FFTW_ESTIMATE);
	if (forward_plan == NULL)
	{
		fputs("Unable to create forward FFT plan\n", stderr);
		ret = ERROR_UNKNOWN;
		goto cleanup;
	}

	fftwf_execute(forward_plan);

	fftwf_execute_dft_r2c(forward_plan, zero_padded_stream2, input_2);

	array_complex_multiplication(input_1, input_2, complex_size, out);

	inverse_plan = fftwf_plan_dft_c2r_1d(size, out, result, FFTW_ESTIMATE);
	if (inverse_plan == NULL)
	{
		fputs("Unable to create inverse FFT plan\n", stderr);
		ret = ERROR_UNKNOWN;
		goto cleanup;
	}

	fftwf_execute(inverse_plan);

cleanup:
	if (forward_plan)
		fftwf_destroy_plan(forward_plan);
	if (inverse_plan)
		fftwf_destroy_plan(inverse_plan);
	if (joint_inputs)
		fftwf_free(joint_inputs);
	if (out)
		fftwf_free(out);
	if (zero_padded_streams)
		free(zero_padded_streams);
	return ret;
}

int audio_delay(size_t num_samples1, size_t num_samples2, float *audio_buffer1, float *audio_buffer2, int *result)
{
	size_t n = next_power_of_2(num_samples1 + num_samples2 - 1);
	float *correlation_result = (float *)malloc(sizeof(float) * n);

	if (correlation_result == NULL)
	{
		fputs("Unable to allocate memory for correlation result\n", stderr);
		return ERROR_NOTENOUGH_MEMORY;
	}

	int return_code = cross_correlation(audio_buffer1, audio_buffer2, num_samples1, num_samples2, correlation_result);
	if (return_code != SUCCESS)
	{
		return return_code;
	}

	int ind = 0;
	for (size_t i = 1; i < n; i++)
	{
		if (correlation_result[i] > correlation_result[ind])
			ind = i;
	}

	free(correlation_result);
	*result = (ind >= num_samples1) ? ind - n : ind;

	return SUCCESS;
}

int resample(float *audio_input, int input_sample_rate, int output_sample_rate, int input_num_of_samples, float **audio_output, int *result_num_of_samples)
{
	AVChannelLayout ch_layout = AV_CHANNEL_LAYOUT_MONO;
	SwrContext *swr_ctx = swr_alloc();
	uint8_t **resampled_data = NULL;
	int ret = SUCCESS;

	if (!swr_ctx)
	{
		fputs("Could not allocate resampler context\n", stderr);
		return ERROR_NOTENOUGH_MEMORY;
	}

	av_opt_set_chlayout(swr_ctx, "in_chlayout", &ch_layout, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", input_sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

	av_opt_set_chlayout(swr_ctx, "out_chlayout", &ch_layout, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", output_sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

	ret = swr_init(swr_ctx) >= 0 ? SUCCESS : handle_av_error(swr_init(swr_ctx));

	if (ret != SUCCESS)
		goto cleanup;

	size_t output_samples_count =
		av_rescale_rnd(swr_get_delay(swr_ctx, input_sample_rate) + input_num_of_samples, output_sample_rate, input_sample_rate, AV_ROUND_UP);

	ret = av_samples_alloc_array_and_samples(&resampled_data, NULL, 1, output_samples_count, AV_SAMPLE_FMT_FLT, 0) >= 0
			? SUCCESS
			: handle_av_error(av_samples_alloc_array_and_samples(&resampled_data, NULL, 1, output_samples_count, AV_SAMPLE_FMT_FLT, 0));

	if (ret != SUCCESS)
		goto cleanup;

	const uint8_t *in_samples[1] = { (const uint8_t *)audio_input };
	int frame_count = swr_convert(swr_ctx, resampled_data, output_samples_count, in_samples, input_num_of_samples);

	if (frame_count < 0)
	{
		ret = handle_av_error(frame_count);
		goto cleanup;
	}

	while (1)
	{
		int frame_flushed = swr_convert(swr_ctx, resampled_data, output_samples_count, NULL, 0);
		if (frame_flushed <= 0)
			break;
		frame_count += frame_flushed;
	}

	*audio_output = (float *)malloc(frame_count * sizeof(float));
	if (!*audio_output)
	{
		fputs("Could not allocate memory for output\n", stderr);
		ret = ERROR_NOTENOUGH_MEMORY;
		goto cleanup;
	}

	memcpy(*audio_output, resampled_data[0], frame_count * sizeof(float));

	*result_num_of_samples = frame_count;

cleanup:
	if (resampled_data)
		av_freep(&resampled_data);
	swr_free(&swr_ctx);
	return ret;
}
