#ifndef READ_AUDIO_FILE_H
#define READ_AUDIO_FILE_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct AudioData
{
	AVFormatContext *format_context;
	AVCodecContext *codec_context;
	const AVCodec *codec;
	AVPacket packet;
	AVFrame *frame;
} AudioData;

int init_audio_data(AudioData *audio);

int handle_av_error(int return_code);

void process_frame_format(AudioData *audio, float **audio_buffer_1, float **audio_buffer_2, size_t *num_samples, int mode);

int process_frames(AudioData *audio, float **audio_buffer_1, float **audio_buffer_2, size_t *num_samples, size_t *buffer_capacity, int mode);

int find_audio_stream(AudioData *audio, int *audio_stream_index, int mode);

int allocate_audio_buffer(float **audio_buffer, size_t capacity);

int read_and_decode_file(const char *filename, float **audio_buffer_1, float **audio_buffer_2, int *sample_rate, size_t *num_samples, int mode);

#endif
