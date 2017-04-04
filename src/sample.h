#ifndef SAMPLE_H
#define SAMPLE_H

#include <stdint.h>

size_t sample_file_as_U16_2ch_wav(const char* filename, uint16_t **buffer, long *size);


#endif
