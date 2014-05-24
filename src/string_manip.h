#ifndef STRING_MANIP_H
#define STRING_MANIP_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char *strip(const char*);
char *tidy_string(const char*);
char *substring(const char* str, int beg_pos, int nc);

#endif
