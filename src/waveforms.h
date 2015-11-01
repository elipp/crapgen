#ifndef WAVEFORMS_H
#define WAVEFORMS_H

#include <stdlib.h>
#include "types.h"

int get_freq(note_t* n, float eqtemp_coef);

extern const sound_t sounds[];
extern const size_t num_sounds;

float waveform_debug(float freq, float t, float phi);

#endif 
