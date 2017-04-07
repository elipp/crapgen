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
	
	const size_t alignment = 2*sizeof(short);

	s->sample_name = sa_strdup(filename);

	long s16bufsize_bytes = ((filesize/alignment)+1)*alignment;
	short *s16buffer = malloc(s16bufsize_bytes);

	s->bufsize_samples = s16bufsize_bytes/alignment;
	s->bufsize_bytes = s16bufsize_bytes * 2;
	
	size_t r = fread(s16buffer, 1, filesize, fp);

	fclose(fp);

	s->buffer = convert_s16tof(s16buffer, s->bufsize_samples);

	free(s16buffer);

	return s;
}

typedef struct abounds_t {
	float hi, lo;
} abounds_t;

static abounds_t find_amplitude_bounds(float *buffer, long num_samples) {
	abounds_t b;
	memset(&b, 0x0, sizeof(b));
	for (int i = 0; i < num_samples; ++i) {
		if (buffer[i] > b.hi) b.hi = buffer[i];
		if (buffer[i] < b.lo) b.lo = buffer[i];
	}

	return b;
}

static float find_ampl_factor(float *buffer, long num_samples) {
	abounds_t b = find_amplitude_bounds(buffer, num_samples);
	float F = 0.9*MAX(fabs(1.0/b.hi), fabs(1.0/b.lo));
	return F;
}

static void multiply_buffer_by(float *buffer, long num_samples, float C) {
	for (long i = 0; i < num_samples; ++i) {
		buffer[i] *= C;
	}
}

static int normalize(float *buffer, long num_samples) {
	float A = find_ampl_factor(buffer, num_samples);
	if (A > 80) { return 0; } // a big A means the signal had rather low dB

	multiply_buffer_by(buffer, num_samples, find_ampl_factor(buffer, num_samples));
	return 1;
}

static long copy_random_splice(sample_t *sample, float* buffer, long num_samples) {

	long random_start = random_int_between(0, sample->bufsize_samples - num_samples * 2);
	memcpy(buffer, sample->buffer + random_start, num_samples * sizeof(float) * 2);

	return random_start;

}

float *sample_get_random_waveform(sample_t *sample, long num_samples) {
	float *buffer = malloc(num_samples * sizeof(float) * 2);

	copy_random_splice(sample, buffer, num_samples);

	while (!normalize(buffer, num_samples)) copy_random_splice(sample, buffer, num_samples);

	return buffer;
}

float *sample_get_random_waveform_freq(sample_t *sample, float freq) {
	long num_samples = 44100.0 / freq;
	printf("getting random waveform with freq %f, num_samples = %ld\n", freq, num_samples);
	return sample_get_random_waveform(sample, num_samples);
}

