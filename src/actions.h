#ifndef ACTIONS_H
#define ACTIONS_H

#include "types.h"
#include "parse.h"
#include "envelope.h"

extern const keyword_action_pair_t keyword_action_pairs[];
extern const size_t num_keyword_action_pairs;

keywordfunc get_matching_action(const char* w);

#endif
