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
#include "envelope.h"

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

float waveform_noise(float freq, float t, float phi) {
	return randomfloatminus1_1();
}

const sound_t sounds[] = {
	{ "default", waveform_sine },
	{ "sine", waveform_sine },
	{ "square", waveform_square },
	{ "triangle", waveform_triangle },
	{ "sawtooth", waveform_sawtooth },
	{ "noise", waveform_noise }
};

const size_t num_sounds = sizeof(sounds)/sizeof(sounds[0]);

int get_freq(note_t* n, float eqtemp_coef) {

	static const float low_c = 32.7032;
	if (n->num_children == 0) {
		n->freq = low_c * pow(eqtemp_coef, n->pitch);
	}
	else {
		for (int i = 0; i < n->num_children; ++i) {
			get_freq(&n->children[i], eqtemp_coef);
		}
	}

	return 1;
}

