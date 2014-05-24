#include "dynamic_wlist.h"
#include "string_allocator.h"
#include "string_manip.h"

char *dynamic_wlist_join_with_delim(dynamic_wlist_t *wl, const char *delim) {
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

dynamic_wlist_t *dynamic_wlist_create() {
	dynamic_wlist_t *wl = malloc(sizeof(dynamic_wlist_t));
	wl->num_items = 0;
	wl->capacity = 8;
	wl->items = malloc(wl->capacity*sizeof(char*)); 
	memset(wl->items, 0x0, wl->capacity*sizeof(char*));

	return wl;
}

void dynamic_wlist_destroy(dynamic_wlist_t *wl) {
	for (int i = 0; i < wl->num_items; ++i) {
		sa_free(wl->items[i]);
	}
	free(wl->items);
	free(wl);
}

dynamic_wlist_t *dynamic_wlist_tidy(dynamic_wlist_t *wl) {
	dynamic_wlist_t *t = dynamic_wlist_create();
	for (int i = 0; i < wl->num_items; ++i) {
		dynamic_wlist_append(t, tidy_string(wl->items[i]));
	}
	return t;
}

void dynamic_wlist_print(dynamic_wlist_t *wl) {
	printf("wlist contents:\n");
	for (int i = 0; i < wl->num_items; ++i) {
		printf(" \"%s\"\n", wl->items[i]);
	}
	printf("------------------------\n");
}

int dynamic_wlist_append(dynamic_wlist_t* wlist, const char* word) {

	if (!word) return 0; 

	++wlist->num_items;
	if (wlist->num_items > wlist->capacity) {
		wlist->capacity *= 2;
		wlist->items = realloc(wlist->items, wlist->capacity*sizeof(char*));
		if (!wlist->items) { return 0; }
	}

	wlist->items[wlist->num_items-1] = sa_strdup(word);

	return 1;
}

dynamic_wlist_t *tokenize_wr_delim(const char* input, const char* delim) {
	dynamic_wlist_t *wl = dynamic_wlist_create();
	char *exprbuf; //, *saveptr;

	char *buf = sa_strdup(input);

	for (exprbuf = strtok(buf, delim); exprbuf != NULL; exprbuf = strtok(NULL, delim)) {
		dynamic_wlist_append(wl, exprbuf);
	}

	sa_free(buf);

	return wl;
}

dynamic_wlist_t *tokenize_wr_delim_tidy(const char* input, const char* delim) {
	dynamic_wlist_t *wl = dynamic_wlist_create();
	char *exprbuf; //, *saveptr;

	char *buf = sa_strdup(input);

	for (exprbuf = strtok(buf, delim); exprbuf != NULL; exprbuf = strtok(NULL, delim)) {
		dynamic_wlist_append(wl, tidy_string(exprbuf));
	}

	sa_free(buf);

	return wl;
}


