#ifndef STRING_ALLOCATOR_H
#define STRING_ALLOCATOR_H
// a simple string pool

void *sa_alloc(size_t size);
void sa_free(void *mem);	// nop free ^^
void sa_clearbuf();
char *sa_strdup(const char* str);

#endif
