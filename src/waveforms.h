#ifndef WAVEFORMS_H
#define WAVEFORMS_H

#include "types.h"

typedef float (*PFNWAVEFORM)(float, float, float);

float waveform_sine(float freq, float t, float phi);
float waveform_square(float freq, float t, float phi);
float waveform_triangle(float freq, float t, float phi);
float waveform_sawtooth(float freq, float t, float phi);

float amplitude_envelope(float t, float d);
float *note_synthesize(note_t *note, float duration, envelope_t env, PFNWAVEFORM wform);

#endif 
