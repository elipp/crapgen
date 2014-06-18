#ifndef WAVEFORMS_H
#define WAVEFORMS_H

#include <stdlib.h>
#include "types.h"

int freq_from_noteindex(note_t* note, float transpose, float eqtemp_coef, float *freqarr);

extern const sound_t sounds[];
extern const size_t num_sounds;

#endif 
