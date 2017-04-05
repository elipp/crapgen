#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include "types.h"

#define SGEN_ERROR(fmt, ...) do {\
	fprintf(stderr, "sgen: %s (@%s:%d) error: " fmt, __func__, __FILE__,  __LINE__, ## __VA_ARGS__);\
	void* callstack[128];\
	int i, frames = backtrace(callstack, 128);\
	char** strs = backtrace_symbols(callstack, frames);\
       	for (i = 0; i < frames; ++i) {\
	       	fprintf(stderr, "%s\n", strs[i]);\
       	}\
       	free(strs);\
       	abort();\
} while (0)

#define SGEN_WARNING(fmt, ...) do {\
	printf("sgen: %s (@%s:%d) warning: " fmt, __func__, __FILE__, __LINE__, ## __VA_ARGS__);\
	} while (0)

int convert_string_to_float(const char* str, float *out);
int convert_string_to_double(const char* str, double *out);
int find_stuff_between(char beg, char end, char* input, char** output);
long get_filesize(FILE *fp);

int sgn(int n);

void note_inherit_value_from_parent(note_t *note);
void note_disinherit(note_t *note);
void note_disinherit_all(note_t *note);

float randomfloat01();
float randomfloatminus1_1();

long random_int_between(long min, long max);

#endif
