#include "string_manip.h"
#include "string_allocator.h"
#include "dynamic_wlist.h"

char *substring(const char* str, int beg_pos, int nc) {

	if (!str) { return NULL; }
	size_t str_len = strlen(str);
	if (str_len < 1) { return NULL; }

	if (str_len < beg_pos + nc) {
		nc = str_len - beg_pos;
	}

	char *sub = sa_alloc(nc+1);

	strncpy(sub, str+beg_pos, nc);

	sub[nc] = '\0';

	return sub;
}


char *strip(const char* input) {

	if (!input) { return NULL; }
	size_t str_len = strlen(input);

	if (str_len < 1) { return NULL; }

	int beg = 0;
	while (beg < str_len) {
		if (input[beg] == ' ' || input[beg] == '\t' || input[beg] == '\n') ++beg;
		else break;
	}

	int end = str_len-1;
	while (end > 0) {
		if (input[end] == ' ' || input[end] == '\t' || input[end] == '\n') --end;
		else break;
	}
	
	long len = end - beg + 1;
	if (len < 1) {
		fprintf(stderr, "sgen: strip: input string len < 1!\n");
		return NULL;
	}

	char *r = substring(input, beg, len);
	return r;
}

char *tidy_string(const char* input) {
	
	if (!input) return NULL;
	char *tidy = strip(input);
	if (!tidy) return NULL; 

	char *chrpos = strchr(tidy, '\n');
	while (chrpos != NULL) {
		*chrpos = ' ';
		chrpos = strchr(chrpos+1, '\n');
	}

	dynamic_wlist_t *t = tokenize_wr_delim(tidy, " \t");
//	fprintf(stderr, "tidy_string: tokenize_wr_delim \" \\t\": num_items = %d\n", t->num_items);
	char *r = dynamic_wlist_join_with_delim(t, " ");	
	// this should get rid of all duplicate whitespace (two or more consecutive)
//	fprintf(stderr, "tidy: \"%s\" -> \"%s\"\n", input, r);

	dynamic_wlist_destroy(t);

	sa_free(tidy);
	return r;

}

char *str_tolower(const char* input) {
	if (!input) return NULL;
	size_t ilen = strlen(input);
	if (ilen < 1) return NULL;

	char *out = malloc(ilen + 1);

	int i = 0;
	for (; i < ilen; ++i) {
		out[i] = tolower(input[i]);
	}
	out[i] = '\0';
	return out;

}

int str_all_digits(const char* input) {
	if (!input) return -1;

	size_t ilen = strlen(input);
	if (ilen < 1) return -1;

	int i = 0;
	for (; i < ilen; ++i) {
		if (!isdigit(input[i])) return 0;
	}

	return 1;
}

int str_to_int_b10(const char* input) {
	char *end_ptr;
	int r = (int)strtol(input, &end_ptr, 10);
	// TODO add end_ptr check stuff
	return r;
}
