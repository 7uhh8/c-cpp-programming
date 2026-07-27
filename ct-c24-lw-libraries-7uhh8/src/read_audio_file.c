#include "read_audio_file.h"

#include "return_codes.h"

const enum AVCodecID supported_formats[] = { AV_CODEC_ID_FLAC, AV_CODEC_ID_MP3, AV_CODEC_ID_MP2, AV_CODEC_ID_OPUS, AV_CODEC_ID_AAC };

int is_supported_format(enum AVCodecID codec_id)
{
	size_t num_codecs = sizeof(supported_formats) / sizeof(supported_formats[0]);
	for (size_t i = 0; i < num_codecs; i++)
	{
		if (supported_formats[i] == codec_id)
			return 1;
	}
	return 0;
}

int init_audio_data(AudioData *audio)
{
	audio->format_context = NULL;
	audio->codec_context = NULL;
	audio->frame = av_frame_alloc();
	if (audio->frame == NULL)
	{
		fputs("Could not allocate memory for AVFrame\n", stderr);
		return ERROR_NOTENOUGH_MEMORY;
	}
	return SUCCESS;
}

int handle_av_error(int return_code)
{
	fprintf(stderr, "Error: %s\n", av_err2str(return_code));
	switch (return_code)
	{
	case AVERROR(EFBIG):
	case AVERROR(ENOMEM):
		return ERROR_NOTENOUGH_MEMORY;
	case AVERROR(ENOENT):
	case AVERROR(EACCES):
		return ERROR_CANNOT_OPEN_FILE;
	case AVERROR(ENAMETOOLONG):
	case AVERROR(EINVAL):
		return ERROR_ARGUMENTS_INVALID;
	case AVERROR_INVALIDDATA:
		return ERROR_DATA_INVALID;
	case AVERROR(ENOSYS):
		return ERROR_UNSUPPORTED;
	default:
		return ERROR_UNKNOWN;
	}
}

void process_frame_format(AudioData *audio, float **audio_buffer_1, float **audio_buffer_2, size_t *num_samples, int mode)
{
	int samples_count = audio->frame->nb_samples;
	switch (audio->frame->format)
	{
	case AV_SAMPLE_FMT_S16P:
	case AV_SAMPLE_FMT_S16:
		if (audio->frame->linesize[0] >= samples_count * 2 * sizeof(int16_t))
		{
			for (size_t i = 0; i < samples_count; i++)
			{
				(*audio_buffer_1)[*num_samples + i] = ((int16_t *)audio->frame->data[0])[2 * i] / (float)INT16_MAX;
				if (mode == 2)
					(*audio_buffer_2)[*num_samples + i] = ((int16_t *)audio->frame->data[0])[2 * i + 1] / (float)INT16_MAX;
			}
		}
		else
		{
			for (size_t i = 0; i < samples_count; i++)
			{
				(*audio_buffer_1)[*num_samples + i] = ((int16_t *)audio->frame->data[0])[i] / (float)INT16_MAX;
				if (mode == 2)
					(*audio_buffer_2)[*num_samples + i] = ((int16_t *)audio->frame->data[1])[i] / (float)INT16_MAX;
			}
		}
		break;
	case AV_SAMPLE_FMT_S32P:
	case AV_SAMPLE_FMT_S32:
		if (audio->frame->linesize[0] >= samples_count * 2 * sizeof(int32_t))
		{
			for (size_t i = 0; i < samples_count; i++)
			{
				(*audio_buffer_1)[*num_samples + i] = ((int32_t *)audio->frame->data[0])[2 * i] / (float)INT32_MAX;
				if (mode == 2)
					(*audio_buffer_2)[*num_samples + i] = ((int32_t *)audio->frame->data[0])[2 * i + 1] / (float)INT32_MAX;
			}
		}
		else
		{
			for (size_t i = 0; i < samples_count; i++)
			{
				(*audio_buffer_1)[*num_samples + i] = ((int32_t *)audio->frame->data[0])[i] / (float)INT32_MAX;
				if (mode == 2)
					(*audio_buffer_2)[*num_samples + i] = ((int32_t *)audio->frame->data[1])[i] / (float)INT32_MAX;
			}
		}
		break;
	case AV_SAMPLE_FMT_FLTP:
		for (size_t i = 0; i < samples_count; i++)
		{
			(*audio_buffer_1)[*num_samples + i] = ((float *)(audio->frame->data[0]))[i];
			if (mode == 2)
				(*audio_buffer_2)[*num_samples + i] = ((float *)(audio->frame->data[1]))[i];
		}
		break;
	default:
		for (size_t i = 0; i < samples_count; i++)
		{
			(*audio_buffer_1)[*num_samples + i] = ((float *)(audio->frame->data[0]))[2 * i];
			if (mode == 2)
				(*audio_buffer_2)[*num_samples + i] = ((float *)(audio->frame->data[0]))[2 * i + 1];
		}
		break;
	}
}

int process_frames(AudioData *audio, float **audio_buffer_1, float **audio_buffer_2, size_t *num_samples, size_t *buffer_capacity, int mode)
{
	int ret = 0;
	while (ret >= 0)
	{
		ret = avcodec_receive_frame(audio->codec_context, audio->frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			break;
		}
		else if (ret < 0)
		{
			return handle_av_error(ret);
		}

		if (*num_samples + audio->frame->nb_samples > *buffer_capacity)
		{
			*buffer_capacity *= 2;
			float *temp_buffer_1 = realloc(*audio_buffer_1, sizeof(float) * (*buffer_capacity));
			float *temp_buffer_2 = (mode == 2) ? realloc(*audio_buffer_2, sizeof(float) * (*buffer_capacity)) : NULL;

			if (temp_buffer_1 == NULL || ((mode == 2) && (temp_buffer_2 == NULL)))
			{
				fputs("Unable to reallocate memory for audio buffers\n", stderr);
				free(*audio_buffer_1);
				*audio_buffer_1 = NULL;
				if (mode == 2)
				{
					free(*audio_buffer_2);
					*audio_buffer_2 = NULL;
				}
				return ERROR_NOTENOUGH_MEMORY;
			}

			*audio_buffer_1 = temp_buffer_1;
			if (mode == 2)
				*audio_buffer_2 = temp_buffer_2;
		}

		process_frame_format(audio, audio_buffer_1, audio_buffer_2, num_samples, mode);

		*num_samples += audio->frame->nb_samples;
	}
	return SUCCESS;
}

int find_audio_stream(AudioData *audio, int *audio_stream_index, int mode)
{
	for (size_t i = 0; i < audio->format_context->nb_streams; i++)
	{
		AVCodecParameters *codec_parameters = audio->format_context->streams[i]->codecpar;
		if (codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio->codec = avcodec_find_decoder(codec_parameters->codec_id);
			if (!audio->codec)
			{
				fputs("This codec is not supported in FFmpeg\n", stderr);
				return ERROR_UNSUPPORTED;
			}

			if (!is_supported_format(codec_parameters->codec_id))
			{
				fputs("This audio format is not supported\n", stderr);
				return ERROR_FORMAT_INVALID;
			}

			if ((mode == 1) || ((mode == 2) && codec_parameters->ch_layout.nb_channels == 2))
			{
				*audio_stream_index = i;
				return SUCCESS;
			}
		}
	}
	fputs("Could not find any suitable audio streams in the file\n", stderr);
	avformat_close_input(&audio->format_context);
	return ERROR_FORMAT_INVALID;
}

int allocate_audio_buffer(float **audio_buffer, size_t capacity)
{
	*audio_buffer = (float *)malloc(sizeof(float) * capacity);
	if (!*audio_buffer)
	{
		fputs("Unable to allocate memory for audio buffer\n", stderr);
		return ERROR_NOTENOUGH_MEMORY;
	}
	return SUCCESS;
}

int read_and_decode_file(const char *filename, float **audio_buffer_1, float **audio_buffer_2, int *sample_rate, size_t *num_samples, int mode)
{
	av_log_set_level(AV_LOG_QUIET);

	AudioData audio;

	int ret = init_audio_data(&audio);
	if (ret != SUCCESS)
	{
		return ret;
	}

	int audio_stream_index = -1;

	ret = avformat_open_input(&audio.format_context, filename, NULL, NULL) >= 0
			? SUCCESS
			: handle_av_error(avformat_open_input(&audio.format_context, filename, NULL, NULL));

	if (ret != SUCCESS)
		goto cleanup;

	ret = avformat_find_stream_info(audio.format_context, NULL) >= 0
			? SUCCESS
			: handle_av_error(avformat_find_stream_info(audio.format_context, NULL));

	if (ret != SUCCESS)
		goto cleanup;

	ret = find_audio_stream(&audio, &audio_stream_index, mode);
	if (ret != SUCCESS)
		goto cleanup;

	audio.codec_context = avcodec_alloc_context3(audio.codec);
	if (!audio.codec_context)
	{
		fputs("Unable to allocate audio codec context\n", stderr);
		ret = ERROR_NOTENOUGH_MEMORY;
		goto cleanup;
	}

	ret = avcodec_parameters_to_context(audio.codec_context, audio.format_context->streams[audio_stream_index]->codecpar) >= 0
			? SUCCESS
			: handle_av_error(
				  avcodec_parameters_to_context(audio.codec_context, audio.format_context->streams[audio_stream_index]->codecpar));

	if (ret != SUCCESS)
		goto cleanup;

	ret = avcodec_open2(audio.codec_context, audio.codec, NULL) >= 0
			? SUCCESS
			: handle_av_error(avcodec_open2(audio.codec_context, audio.codec, NULL));
	if (ret != SUCCESS)
	{
		goto cleanup;
	}

	*sample_rate = audio.codec_context->sample_rate;
	*num_samples = 0;
	size_t buffer_capacity = audio.codec_context->sample_rate;

	ret = allocate_audio_buffer(audio_buffer_1, buffer_capacity);
	if (ret != SUCCESS)
	{
		goto cleanup;
	}

	if (mode == 2)
	{
		ret = allocate_audio_buffer(audio_buffer_2, buffer_capacity);
		if (ret != SUCCESS)
			goto cleanup;
	}

	while (av_read_frame(audio.format_context, &audio.packet) >= 0)
	{
		if (audio.packet.stream_index == audio_stream_index)
		{
			ret = avcodec_send_packet(audio.codec_context, &audio.packet) >= 0
					? SUCCESS
					: handle_av_error(avcodec_send_packet(audio.codec_context, &audio.packet));

			if (ret != SUCCESS)
				goto cleanup;

			ret = (mode == 1) ? process_frames(&audio, audio_buffer_1, NULL, num_samples, &buffer_capacity, mode)
							  : process_frames(&audio, audio_buffer_1, audio_buffer_2, num_samples, &buffer_capacity, mode);

			if (ret != SUCCESS)
				goto cleanup;
		}
		av_packet_unref(&audio.packet);
	}

cleanup:
	if (audio.codec_context)
		avcodec_free_context(&audio.codec_context);
	if (audio.format_context)
		avformat_close_input(&audio.format_context);
	if (audio.frame)
		av_frame_free(&audio.frame);
	return ret;
}
