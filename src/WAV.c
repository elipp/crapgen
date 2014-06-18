#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "types.h"
#include "WAV.h"
#include "utils.h"

const short SHORT_MAX = 0x7FFF;

static inline int eswap_s32(int n) {
	return ((n>>24)&0x000000FF) |
		((n>>8)&0x0000FF00) |
		((n<<8)&0x00FF0000) |
		((n<<24)&0xFF000000);
}

static inline short eswap_s16(short n) {
	return (n>>8) | (n<<8);
}

// for whatever reason, these are in big endian form

static const int WAV_hdr_RIFF = 0x46464952; 
//static const int WAV_hdr_RIFF = eswap_s32(0x52494646); // "RIFF"

static const int WAV_hdr_WAVE = 0x45564157;
//static const int WAV_hdr_WAVE = eswap_s32(0x57415645); // "WAVE"

static const int WAV_hdr_fmt = 0x20746d66;
//static const int WAV_hdr_fmt = eswap_s32(0x666d7420); // "fmt "

static const int WAV_hdr_data = 0x61746164;
//static const int WAV_hdr_data = eswap_s32(0x64617461); // "data"

WAV_hdr_t generate_WAV_header(output_t *o) {

	WAV_hdr_t hdr;

	hdr.ChunkID_BE = WAV_hdr_RIFF;
	int bytes_per_sample = o->bitdepth/8;
	hdr.Subchunk2Size_LE = o->float32_buffer.num_samples_per_channel*o->channels*bytes_per_sample;

	hdr.ChunkSize_LE = 36 + hdr.Subchunk2Size_LE;
	hdr.Format_BE = WAV_hdr_WAVE;
	hdr.Subchunk1ID_BE = WAV_hdr_fmt;
	hdr.Subchunk1Size_LE = 16; // 16 for PCM
	hdr.AudioFormat_LE = 1; // 1 for PCM
	hdr.NumChannels_LE = o->channels;
	hdr.SampleRate_LE = o->samplerate;
	hdr.ByteRate_LE = o->samplerate * o->channels * bytes_per_sample;
	hdr.BlockAlign_LE = o->channels * bytes_per_sample;
	hdr.BitsPerSample_LE = o->bitdepth;
	hdr.Subchunk2ID_BE = WAV_hdr_data;
	return hdr;
}

sgen_float32_buffer_t read_WAV(const char* filename) { 

	sgen_float32_buffer_t b;

	FILE *fp = fopen(filename, "r");
	if (!fp) { 
		SGEN_ERROR("couldn't open file \"%s\": \n", strerror(errno));
		return b; 
	}

	size_t sz = get_filesize(fp);
	WAV_hdr_t wh;
	unsigned short *buf = malloc(sz);

	fread(&wh, sizeof(WAV_hdr_t), 1, fp);

	fread(buf, 1, sz-sizeof(WAV_hdr_t), fp);
	fclose(fp);

	size_t num_samples = sz/sizeof(unsigned short);

	b.buffer = malloc(sz);
	b.num_samples_per_channel = num_samples/2;

	for (int i = 0; i < num_samples; ++i) {
		b.buffer[i] = (float)buf[i]/SHORT_MAX;
	}

	free(buf);

	return b;
}
