#ifndef ENVELOPE_H
#define ENVELOPE_H
	
envelope_t *envelope_generate(char* name, float amplitude, float a, float d, float s, float sl, float r, float rl);
float *envelope_precalculate(envelope_t *env, long num_samples, float samplerate);
void envelope_destroy(envelope_t *env);
float envelope_get_amplitude_noprecalculate(int snum, int num_samples, const envelope_t *env);

envelope_t *random_envelope();

envelope_t *ctx_get_envelope(sgen_ctx_t* ctx, const char* name);
int ctx_add_envelope(sgen_ctx_t* ctx, envelope_t *env);

#endif
