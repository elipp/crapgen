#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "string_allocator.h"

#define SA_POOL_SIZE (8*1024)

static char sa_buffer[SA_POOL_SIZE];

struct sa_auxbuf {
	char *buf;
	size_t size;
	size_t char_index;
};

static struct sa_auxbuf *sa_aux_buffers = NULL;

static int sa_char_index = 0;
static int sa_aux_num_buffers = 0;

static void free_all_aux_buffers() {
	int i = 0;
	for (; i < sa_aux_num_buffers; ++i) {
		free(sa_aux_buffers[i].buf);
	}
	free(sa_aux_buffers); 
	sa_aux_buffers = NULL;
	sa_aux_num_buffers = 0;
}

void sa_clearbuf() {
	if (sa_aux_buffers) { 
		free_all_aux_buffers();
	}
	sa_char_index = 0;
}

static struct sa_auxbuf *allocate_additional_aux_buffer(size_t size) {

	++sa_aux_num_buffers;
//	fprintf(stderr, "string_allocator: allocating %luK aux buffer #%d\n", size/1024, sa_aux_num_buffers);

	struct sa_auxbuf b;
	b.buf = malloc(size);
	b.size = size;
	b.char_index = 0;

	sa_aux_buffers = realloc(sa_aux_buffers, sa_aux_num_buffers*sizeof(struct sa_auxbuf));

	if (!sa_aux_buffers) { fprintf(stderr, "alloc_additional_aux_buffer: realloc() error!\n"); return NULL; }

	sa_aux_buffers[sa_aux_num_buffers - 1] = b;
	return &sa_aux_buffers[sa_aux_num_buffers-1];

}

void *sa_alloc(size_t size) {
	size_t num_chars = size/(sizeof(char));

	if (sa_char_index + num_chars > SA_POOL_SIZE - 1) {
		#define SA_AUX_DEFAULT_SIZE (32*1024)
	
		if (!sa_aux_buffers) {
			sa_aux_buffers = malloc(sa_aux_num_buffers * sizeof(char*));
			size_t newbuf_size = num_chars < SA_AUX_DEFAULT_SIZE ? SA_AUX_DEFAULT_SIZE : num_chars + 1;
			allocate_additional_aux_buffer(newbuf_size);
		}

		struct sa_auxbuf *cur_auxbuf = &sa_aux_buffers[sa_aux_num_buffers-1];
	
		if (cur_auxbuf->char_index + num_chars > cur_auxbuf->size) {
//			fprintf(stderr, "cur_auxbuf->char_index (%lu) + num_chars (%lu) > cur_auxbuf->size (%lu)!\n", cur_auxbuf->char_index, num_chars, cur_auxbuf->size);
			size_t newbuf_size = num_chars < SA_AUX_DEFAULT_SIZE ? SA_AUX_DEFAULT_SIZE : num_chars + 1;
			cur_auxbuf = allocate_additional_aux_buffer(newbuf_size);
		}

		char *ptr = cur_auxbuf->buf + cur_auxbuf->char_index;
		cur_auxbuf->char_index += num_chars + 1;
		return (void*)ptr;
	}
	else {
		char *ptr = sa_buffer + sa_char_index;
		sa_char_index += num_chars + 1;	// +1 for the '\0' char
		return (void*)ptr;
	}
}

void sa_free(void *mem) {};

char *sa_strdup(const char* str) {
	size_t str_len = strlen(str);

	char *n = sa_alloc(str_len+1);
	strncpy(n, str, str_len);
	n[str_len] = '\0';

	return n;
}
