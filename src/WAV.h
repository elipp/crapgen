#ifndef WAV_H
#define WAV_H

#include "types.h"

extern const short SHORT_MAX;

WAV_hdr_t generate_WAV_header(output_t *o);
sgen_float32_buffer_t *read_WAV(const char* filename);

#endif
