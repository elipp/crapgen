#include <stdio.h>
#include <math.h>
#include <time.h>
#include "types.h"


static const float HALF_RAND_MAX = 0.5*(float)RAND_MAX;

float randomfloat01() { return (float)rand()/(float)RAND_MAX; }
float randomfloatminus1_1() { return (float)rand()/HALF_RAND_MAX - 1.0; }

const envelope_t default_envelope = {
	"default_envelope",
	{ 1.0, 0.1, 0.1, 5, 0.1, 2, 1 },
	NULL,
	-1	
};

envelope_t envelope_generate(char* name, float amplitude, float A, float D, float S, float SL, float R) {

	envelope_t e;
	e.name = name;
	float sum = A + D + S + R;
	// normalize parms

	e.parms[ENV_AMPLITUDE] = amplitude;
	e.parms[ENV_ATTACK] = A/sum;
	e.parms[ENV_DECAY] = D/sum;
	e.parms[ENV_SUSTAIN] = S/sum;
	e.parms[ENV_SUSTAIN_LEVEL] = SL;
	e.parms[ENV_RELEASE] = R/sum;

	printf("sgen: envelope_generate: id = %s, A = %f, D = %f, S = %f, SL = %f, R = %f\n", name, 
	e.parms[ENV_ATTACK], e.parms[ENV_DECAY], e.parms[ENV_SUSTAIN], e.parms[ENV_SUSTAIN_LEVEL], e.parms[ENV_RELEASE]);

	return e;
}

float *envelope_precalculate(envelope_t *env, long num_samples, float samplerate) {

	float *envelope = malloc(num_samples*samplerate);
	float sa = env->parms[ENV_ATTACK] * num_samples;
	float sd = env->parms[ENV_DECAY] * num_samples;
	float ss = env->parms[ENV_SUSTAIN] * num_samples;
	float sl = env->parms[ENV_SUSTAIN_LEVEL];
	float sr = env->parms[ENV_RELEASE] * num_samples;

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
	float sa = env->parms[ENV_ATTACK] * ns;
	float sd = env->parms[ENV_DECAY] * ns;
	float ss = env->parms[ENV_SUSTAIN] * ns;
	float sl = env->parms[ENV_SUSTAIN_LEVEL];
	float sr = env->parms[ENV_RELEASE] * ns;


	float kAD = (sl - 1.0)/sd;
	float kSR = -(sl/sr);

	float R = 0;

	if (snum < sa) R = snum/sa;
	else if (snum < sa + sd) R = kAD*(snum - sa) + 1.0;
	else if (snum < sa + sd + ss) R = sl;
	else R = kSR * (snum - num_samples);

	return R;

}	


envelope_t random_envelope() {
	envelope_t e = 
	envelope_generate("rnd", randomfloat01(), 0.01, randomfloat01(), randomfloat01(), randomfloat01(), randomfloat01());
	return e;
}
