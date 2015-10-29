#ifndef DYNAMIC_WLIST_H
#define DYNAMIC_WLIST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"

void wlist_destroy(dynamic_wlist_t *wl);
dynamic_wlist_t *wlist_create(void);

int wlist_append(dynamic_wlist_t* wl, const char* word);

dynamic_wlist_t *tokenize_wr_delim(const char* input, const char* delim);
dynamic_wlist_t *tokenize_wr_delim_tidy(const char* input, const char* delim);
dynamic_wlist_t *wlist_tidy(dynamic_wlist_t *wl);
char *wlist_join_with_delim(dynamic_wlist_t *wl, const char *delim);

void wlist_print(dynamic_wlist_t *wl);

#endif
