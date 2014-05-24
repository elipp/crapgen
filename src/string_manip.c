#include "string_manip.h"
#include "string_allocator.h"
#include "dynamic_wlist.h"

char *substring(const char* str, int beg_pos, int nc) {

	if (!str) { return NULL; }
	size_t str_len = strlen(str);
	if (!str_len) { return NULL; }

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

	if (str_len <= 0) { return NULL; }

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
	
	char *tidy = strip(input);
	if (!tidy) { return NULL; }

	char *chrpos = strchr(tidy, '\n');
	while (chrpos != NULL) {
		*chrpos = ' ';
		chrpos = strchr(chrpos+1, '\n');
	}

	dynamic_wlist_t *t = tokenize_wr_delim(tidy, " \t");
	char *r = dynamic_wlist_join_with_delim(t, " ");	
	// this should get rid of all duplicate whitespace (two or more consecutive)

	sa_free(tidy);
	return r;

}


