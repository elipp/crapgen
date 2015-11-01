#ifndef PARSE_H
#define PARSE_H

#include <errno.h>
#include <math.h>

#include "types.h"
#include "utils.h"
#include "string_manip.h"
#include "string_allocator.h"
#include "dynamic_wlist.h"
#include "envelope.h"
#include "waveforms.h"
#include "track.h"

input_t input_construct(const char* filename);

char* get_primitive_identifier(expression_t *track_expr);
dynamic_wlist_t *get_primitive_args(expression_t *prim_expr);

int read_track(expression_t *track_expr, track_t *t, sgen_ctx_t *c);
int read_song(expression_t *arg, song_t *s, sgen_ctx_t *c);

#endif
