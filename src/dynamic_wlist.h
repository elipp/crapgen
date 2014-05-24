#ifndef DYNAMIC_WLIST_H
#define DYNAMIC_WLIST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"

void dynamic_wlist_destroy(dynamic_wlist_t *wl);
dynamic_wlist_t *dynamic_wlist_create(void);

int dynamic_wlist_append(dynamic_wlist_t* wl, const char* word);

dynamic_wlist_t *tokenize_wr_delim(const char* input, const char* delim);
dynamic_wlist_t *tokenize_wr_delim_tidy(const char* input, const char* delim);
dynamic_wlist_t *dynamic_wlist_tidy(dynamic_wlist_t *wl);
char *dynamic_wlist_join_with_delim(dynamic_wlist_t *wl, const char *delim);

void dynamic_wlist_print(dynamic_wlist_t *wl);

#endif
