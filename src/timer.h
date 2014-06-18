#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	struct timeval beg;
} sgen_timer_t;

sgen_timer_t timer_construct();

void timer_begin(sgen_timer_t* timer);
double timer_get_us(const sgen_timer_t* timer);
double timer_get_ms(const sgen_timer_t* timer);
double timer_get_s(const sgen_timer_t* timer);

char *timer_report(sgen_timer_t *timer);
