#ifndef ENVELOPE_H
#define ENVELOPE_H
	
float randomfloat01();
float randomfloatminus1_1();

envelope_t envelope_generate(char* name, float amplitude, float a, float d, float s, float sl, float r);
float *envelope_precalculate(envelope_t *env, long num_samples, float samplerate);
void envelope_destroy(envelope_t *env);
float envelope_get_amplitude_noprecalculate(int snum, int num_samples, envelope_t *env);

envelope_t random_envelope();

extern envelope_t default_envelope;

#endif
