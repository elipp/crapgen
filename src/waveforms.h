#ifndef WAVEFORMS_H
#define WAVEFORMS_H

#include <stdlib.h>
#include "types.h"

float *note_synthesize(note_t *note, PFNWAVEFORM wform);

extern const sound_t sounds[];
extern const size_t num_sounds;

#endif 
