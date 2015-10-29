#ifndef STRING_MANIP_H
#define STRING_MANIP_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

char *strip(const char*);
char *tidy_string(const char*);
char *substring(const char* str, int beg_pos, int nc);
char *str_tolower(const char* input);

int str_isall(const char* input, int (*func)(int));
int str_to_int_b10(const char* input);

#endif
