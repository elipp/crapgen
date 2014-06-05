#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#ifndef M_PI
#define M_PI 3.14159265359
#endif

#include "waveforms.h"
#include "types.h"

static const float TWO_PI = (2*M_PI);
static const float PI_PER_TWO = (M_PI/2);


float waveform_sine(float freq, float t, float phi) {
	return sin(TWO_PI*freq*t + phi);
}

float waveform_square(float freq, float t, float phi) {
	float p = (TWO_PI*freq*t + phi)/TWO_PI;
	float phase = p - floor(p);

	return (phase > 0.5 ? 1 : -1);
}

float waveform_triangle(float freq, float t, float phi) {
	float p = (TWO_PI*freq*t + phi)/TWO_PI;
	float phase = p - floor(p);
	return phase < 0.5 ? (-4*phase + 1) : (4*phase - 3);
	
}

float waveform_sawtooth(float freq, float t, float phi) {
	float p = (TWO_PI*freq*t + phi)/(2*TWO_PI);
	float phase = p - floor(p);

	return (-2*phase + 1);
}

float amplitude_envelope(float t, float d) {
	float d_recip = 1.0/d;
	// wtf. compute these from the envelope struct :dd
	if (t < (1.0/4.0)*d) {
		return ((4.0*d_recip)*t);
	} else if (t < (2.0/4.0)*d) {
		return ((-2.0)*d_recip)*t + (3.0/2.0);
	} else if (t < (3.0/4.0)*d) {
		return 0.5;
	} else {
		return ((-2.0*d_recip)*t + 2.0);
	}

}

static float *freq_from_noteindex(note_t* note, float transpose) {
	static const float twelve_equal_coeff = 1.05946309436; // 2^(1/12)
	static const float low_c = 32.7032;
	float *pitches = malloc(note->num_values*sizeof(float));
//	fprintf(stderr, "note->num_values: %d, note->values[0] = %d\n", note->num_values, note->values[0]);
	for (int i = 0; i < note->num_values; ++i) {
		pitches[i] = low_c * pow(twelve_equal_coeff, note->values[i] + transpose);
//		fprintf(stderr, "pitch: %f\n", pitches[i]);
	}

	return pitches;
}


float *note_synthesize(note_t *note, PFNWAVEFORM wform) {
// TODO: replace hard-coded values with output_t struct values!
	float samplerate = 44100;
	long num_samples = note->duration_s*samplerate;

	float *samples = malloc(num_samples*sizeof(float)); 
	memset(samples, 0x0, num_samples*sizeof(float));

	float dt = 1.0/samplerate;
	float A = 1.0/note->num_values;

	float *freqs = freq_from_noteindex(note, note->transpose);

	for (int i = 0; i < note->num_values; ++i) {
		float f = freqs[i];
//		fprintf(stderr, "freq: %f, i = %d\n", f, i);
		long j = 0;
		float t = 0;
		for (; j < num_samples; ++j) {
			samples[j] += A*amplitude_envelope(t, note->duration_s)*wform(f, t, 0);
			t += dt;
		}
	}

	free(freqs);

	return samples;
}
