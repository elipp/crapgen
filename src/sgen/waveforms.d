module waveforms;

import std.math;
import std.c.string;

static const float TWO_PI = (2*PI);
static const float PI_PER_TWO = (PI/2);

struct envelope { 
	float attack;
	float decay;
	float sustain;
	float release;
}; 

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

float amplitude_envelope(float t, float d) {
	float d_recip = 1.0/d;
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

float[] note_synthesize(float[] freqs, float duration, envelope env, wformfptr wform) {
	float samplerate = 44100;
	long num_samples = cast(long)(std.math.ceil(duration*samplerate));
	float[] samples = new float[num_samples]; // TODO: replace hard-coded values with output struct values!

	std.c.string.memset(cast(void*)samples, 0, samples.length*float.sizeof);

	float dt = 1.0/samplerate;

	float A = 1.0/freqs.length;

	foreach (f; freqs) {
		long i = 0;
		float t = 0;
		for (; i < num_samples; ++i) {
			samples[i] += A*amplitude_envelope(t, duration)*wform(f, t, 0);
			t += dt;
		}
	}

	return samples;
}
