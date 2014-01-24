module waveforms;

import std.math;

static const float TWO_PI = (2*PI);
static const float PI_PER_TWO = (PI/2);

struct envelope { 
	float decay;
	float attack;
}; // nyi

alias float function(float, float, float) wformfptr;

float waveform_sine(float freq, float t, float phi) {
	return sin(TWO_PI*freq*t + phi);
}

float waveform_square(float freq, float t, float phi) {
	return (TWO_PI*freq*t + phi) > PI ? 1 : -1;
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

float[] note_synthesize(float freq, float duration, envelope env, wformfptr wform) {
	float samplerate = 44100;
	long num_samples = cast(long)(std.math.ceil(duration*samplerate));
	float[] samples = new float[num_samples]; // TODO: replace hard-coded values with output struct values!

	float dt = 1.0/samplerate;
	float t = 0;

	for (long i = 0; i < num_samples; ++i) {
		samples[i] = std.math.exp(-2*t)*wform(freq, t, 0);
		t += dt;
	}

	return samples;
}
