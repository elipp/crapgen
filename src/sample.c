#include "utils.h"
#include "sample.h"
#include "string_allocator.h"

sample_t *sample_file_as_U16_2ch_wav(const char* filename, const char* samplename, int OPENMODE) {

	sample_t *s = malloc(sizeof(sample_t));

	FILE *fp = fopen(filename, "rb");
	if (!fp) { 
		SGEN_ERROR("unable to open file %s!\n", filename);
		return 0;
       	}

	long filesize = get_filesize(fp);
	
	const size_t alignment = 2*sizeof(uint16_t);

	s->sample_name = sa_strdup(filename);

	s->bufsize_bytes = ((filesize/alignment)+1)*alignment;
	s->buffer = malloc(s->bufsize_bytes);

	s->bufsize_samples = s->bufsize_bytes/alignment;
	
	size_t r = fread(s->buffer, 1, filesize, fp);

	fclose(fp);

	return s;
}

uint16_t *sample_get_random_waveform(sample_t *sample, long num_samples) {
	uint16_t *buffer = malloc(num_samples*2*sizeof(uint16_t));
	long random_start = random_int_between(0, sample->bufsize_samples - num_samples);

	memcpy(buffer, sample->buffer + random_start, num_samples * sizeof(uint16_t) * 2);

	return buffer;
}

uint16_t *sample_get_random_waveform_freq(sample_t *sample, float freq) {
	long num_samples = 44100.0 / freq;
	return sample_get_random_waveform(sample, num_samples);
}
