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
#include "utils.h"

static const float TWO_PI = (2*M_PI);
static const float PI_PER_TWO = (M_PI/2);

float waveform_debug(float freq, float t, float phi) {
	return sin(TWO_PI*freq*t + phi);
}

static float waveform_sine(float freq, float t, float phi) {
	return sin(TWO_PI*freq*t + phi);
}

static float waveform_square(float freq, float t, float phi) {
	float p = (TWO_PI*freq*t + phi)/TWO_PI;
	float phase = p - floor(p);

	return (phase > 0.5 ? 1 : -1);
}

static float waveform_triangle(float freq, float t, float phi) {
	float p = (TWO_PI*freq*t + phi)/TWO_PI;
	float phase = p - floor(p);
	return phase < 0.5 ? (-4*phase + 1) : (4*phase - 3);
}

static float waveform_sawtooth(float freq, float t, float phi) {
	float p = (TWO_PI*freq*t + phi)/(2*TWO_PI);
	float phase = p - floor(p);

	return (-2*phase + 1);
}

static float waveform_noise(float freq, float t, float phi) {
	return randomfloatminus1_1();
}

static float waveform_noise_8bit(float freq, float t, float phi) {
	int r = rand() % 256 - 128;
	return (r/127);
}

static float waveform_noise_4bit(float freq, float t, float phi) {
	int r = rand() % 16 - 8;
	return (r/8);
}

const sound_t sounds[] = {
	{ "default", waveform_sine },
	{ "sine", waveform_sine },
	{ "square", waveform_square },
	{ "triangle", waveform_triangle },
	{ "sawtooth", waveform_sawtooth },
	{ "noise", waveform_noise },
	{ "noise8", waveform_noise_8bit },
	{ "noise4", waveform_noise_4bit },
};

const size_t num_sounds = sizeof(sounds)/sizeof(sounds[0]);

int get_freq(note_t* n, float eqtemp_coef, int transpose) {

	static const float low_c = 32.7032;
	if (n->num_children == 0) {
		n->freq = low_c * pow(eqtemp_coef, n->pitch + transpose);
	}
	else {
		for (int i = 0; i < n->num_children; ++i) {
			get_freq(&n->children[i], eqtemp_coef, transpose);
		}
	}

	return 1;
}

