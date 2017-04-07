#ifndef SAMPLE_H
#define SAMPLE_H

#include "types.h"

sample_t *sample_file_as_U16_2ch_wav(const char* filename, const char* samplename, int OPENMODE);
float *sample_get_random_waveform(sample_t *sample, long num_samples);
float *sample_get_random_waveform_freq(sample_t *sample, float freq);

enum { SAMPLE_OPENMODE_NONE,
	SAMPLE_OPENMODE_WAV,
	SAMPLE_OPENMODE_BINARY };

#endif
