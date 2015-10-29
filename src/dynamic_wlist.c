#include "dynamic_wlist.h"
#include "string_allocator.h"
#include "string_manip.h"

int wlist_append(dynamic_wlist_t* wlist, const char* word) {

	if (!word) { return 0; }

	++wlist->num_items;
	if (wlist->num_items > wlist->capacity) {
		wlist->capacity *= 2;
		wlist->items = realloc(wlist->items, wlist->capacity*sizeof(char*));
		if (!wlist->items) { return 0; }
	}

	wlist->items[wlist->num_items-1] = sa_strdup(word);

//	fprintf(stderr, "appended string \"%s\"\n", wlist->items[wlist->num_items-1]);

	return 1;
}


char *wlist_join_with_delim(dynamic_wlist_t *wl, const char *delim) {
	size_t total_length = 0;

	for (long i = 0; i < wl->num_items; ++i) {
		char *iter = wl->items[i];
		total_length += strlen(iter);
	}

	size_t joint_size = total_length + wl->num_items + 2;
	char* joint = sa_alloc(joint_size);
	memset(joint, 0x0, joint_size);

	for (int i = 0; i < wl->num_items-1; ++i) {
		char *iter = wl->items[i];
		strcat(joint, iter);
		strcat(joint, delim);
	}
	strcat(joint, wl->items[wl->num_items-1]);
	joint[joint_size - 1] = '\0';

//	dynamic_wlist_print(wl);
//	fprintf(stderr, "joint: \"%s\"\n", joint);

	return joint; 

}

dynamic_wlist_t *wlist_create() {
	dynamic_wlist_t *wl = malloc(sizeof(dynamic_wlist_t));
	wl->num_items = 0;
	wl->capacity = 8;
	wl->items = malloc(wl->capacity*sizeof(char*)); 
	memset(wl->items, 0x0, wl->capacity*sizeof(char*));

	return wl;
}

void wlist_destroy(dynamic_wlist_t *wl) {
	for (int i = 0; i < wl->num_items; ++i) {
		sa_free(wl->items[i]);
	}
	free(wl->items);
	free(wl);
}

dynamic_wlist_t *wlist_tidy(dynamic_wlist_t *wl) {
	dynamic_wlist_t *t = wlist_create();
	for (int i = 0; i < wl->num_items; ++i) {
		wlist_append(t, tidy_string(wl->items[i]));
	}
	return t;
}

void wlist_print(dynamic_wlist_t *wl) {
	printf("wlist contents:\n");
	for (int i = 0; i < wl->num_items; ++i) {
		printf(" \"%s\"\n", wl->items[i]);
	}
	printf("------------------------\n");
}

dynamic_wlist_t *tokenize_wr_delim(const char* input, const char* delim) {
	dynamic_wlist_t *wl = wlist_create();
	char *exprbuf, *saveptr;

	char *buf = sa_strdup(input);
	//size_t input_len = strlen(input);

	for (exprbuf = strtok_r(buf, delim, &saveptr); exprbuf != NULL; exprbuf = strtok_r(NULL, delim, &saveptr)) {
		wlist_append(wl, exprbuf);
	}

	sa_free(buf);

	return wl;
}

dynamic_wlist_t *tokenize_wr_delim_exclude(const char* input, const char* delim, const char* begstr) {
	dynamic_wlist_t *wl = wlist_create();
	char *exprbuf, *saveptr;

	char *buf = sa_strdup(input);
	size_t begstr_len = strlen(begstr);

	for (exprbuf = strtok_r(buf, delim, &saveptr); exprbuf != NULL; exprbuf = strtok_r(NULL, delim, &saveptr)) {
		size_t len = strlen(exprbuf);
		if (len >= begstr_len) {
			if (strncmp(exprbuf, begstr, begstr_len) != 0)
				wlist_append(wl, exprbuf);
		}
	}

	sa_free(buf);

	return wl;
}



dynamic_wlist_t *tokenize_wr_delim_tidy(const char* input, const char* delim) {
	dynamic_wlist_t *wl = wlist_create();
	char *exprbuf, *saveptr;

	char *buf = sa_strdup(input);
	size_t input_len = strlen(input);
	fprintf(stderr, "input_len = %ld, delim = \"%s\"\n", input_len, delim);

	//for (exprbuf = strtok(buf, delim); exprbuf != NULL; exprbuf = strtok(NULL, delim)) {
	for (exprbuf = strtok_r(buf, delim, &saveptr); exprbuf != NULL; exprbuf = strtok_r(NULL, delim, &saveptr)) {
		wlist_append(wl, tidy_string(exprbuf));
	}

	fprintf(stderr, "[info]: tokenize_wr_delim_tidy: tokens gathered: %d\n", (int)wl->num_items);

	sa_free(buf);

	return wl;
	
	/*
	dynamic_wlist_t *wldirty = tokenize_wr_delim(input, delim);
	dynamic_wlist_t *r = dynamic_wlist_tidy(wldirty);

	dynamic_wlist_destroy(wldirty);  
	return r; */
}


