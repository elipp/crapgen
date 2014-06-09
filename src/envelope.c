#include <stdio.h>
#include "types.h"

envelope_t *default_envelope;

envelope_t *envelope_generate(float a, float d, float s, float sl, float r) {

	envelope_t *e = malloc(sizeof(envelope_t));
	float sum = a + d + s + r;
	// normalize parms

	e->attack = a/sum;
	e->decay = d/sum;
	e->sustain = s/sum;
	e->release = r/sum;
	e->sustain_level = sl;

	fprintf(stderr, "sgen: envelope_generate: a = %f, d = %f, s = %f, r = %f, sl = %f\n", e->attack, e->decay, e->sustain, e->release, e->sustain_level);
	return e;
}

float *envelope_precalculate(envelope_t *env, long num_samples, float samplerate) {

	float *envelope = malloc(num_samples*samplerate);
	float sa = env->attack * num_samples;
	float sd = env->decay * num_samples;
	float ss = env->sustain * num_samples;
	float sr = env->release * num_samples;
	float sl = env->sustain_level;

	int i = 0;
	for (i = 0; i < sa; ++i) {
		envelope[i] = i/sa;
	}

	float kAD = (sl - 1.0)/sd;
	for (; i < sa+sd; ++i) {
		envelope[i] = kAD*(i - sa) + 1.0;
	}

	for (; i < sa+sd+ss; ++i) {
		envelope[i] = sl;
	}

	float kSR = -(sl/sr);
	for (; i < sa+sd+ss+sr; ++i) {
		envelope[i] = kSR * (i - num_samples);
	}

	return envelope;
}

void envelope_destroy(envelope_t *env) {
	free(env->envelope);
	free(env);
}

float envelope_get_amplitude_noprecalculate(int snum, int num_samples, envelope_t *env) {
	float ns = (float)num_samples;
	float sa = env->attack * ns;
	float sd = env->decay * ns;
	float ss = env->sustain * ns;
	float sr = env->release * ns;
	float sl = env->sustain_level;

	float kAD = (sl - 1.0)/sd;
	float kSR = -(sl/sr);

	if (snum < sa) return snum/sa;
	else if (snum < sa + sd) return kAD*(snum - sa) + 1.0;
	else if (snum < sa + sd + ss) return sl;
	else return kSR * (snum - num_samples);

}	
