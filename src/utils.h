#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

#define SGEN_ERROR(fmt, ...) do {\
	fprintf(stderr, "sgen: %s (@%s:%d) error: " fmt, __func__, __FILE__,  __LINE__, ## __VA_ARGS__);\
	} while (0)

#define SGEN_WARNING(fmt, ...) do {\
	printf("sgen: %s (@%s:%d) warning: " fmt, __func__, __FILE__, __LINE__, ## __VA_ARGS__);\
	} while (0)

int convert_string_to_float(const char* str, float *out);
int convert_string_to_double(const char* str, double *out);
int find_stuff_between(char beg, char end, char* input, char** output);
long get_filesize(FILE *fp);

int sgn(int n);

#endif
