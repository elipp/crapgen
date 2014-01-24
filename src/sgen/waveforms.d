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

float[] note_synthesize(float freq, float duration, envelope env, wformfptr wform) {
	float samplerate = 44100;
	long num_samples = cast(long)(std.math.ceil(duration*samplerate));
	float[] samples = new float[num_samples]; // TODO: replace hard-coded values with output struct values!

	float dt = 1.0/samplerate;
	float t = 0;

	long i = 0;
	long fadein_samples = 2500;
	long fadeout_samples = 1500;

	static float e(float t) { return std.math.exp(-3*t); }

	for (; i < fadein_samples; ++i) {
		samples[i] = exp(100*(t - (fadein_samples/samplerate)))*e(t)*wform(freq, t, 0);
		t += dt;
	}
	for (; i < num_samples - fadeout_samples; ++i) {
		samples[i] = e(t)*wform(freq, t, 0);
		t += dt;
	}

	for (; i < num_samples; ++i) {
		samples[i] = exp(70*((duration-t) - (fadeout_samples/samplerate)))*e(t)*wform(freq,t,0);
//		samples[i] = e(t)*wform(freq,t,0);
		t += dt;
	}

	return samples;
}
