#include "utils.h"
#include "sample.h"


int sample_file_as_U16_2ch_wav(const char* filename, uint16_t **buffer, long *size) {

	FILE *fp = fopen(filename, "rb");
	if (!fp) return 0;

	long filesize = get_filesize(fp);
	
	*size = (filesize+1)/2 * 2;
	*buffer = malloc(*size);
	
	size_t r = fread(*buffer, 1, filesize, fp);

	return 1;
}
