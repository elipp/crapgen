#include <sys/time.h>

#include "timer.h"

sgen_timer_t timer_construct() {
	sgen_timer_t r;
	memset(&r, 0, sizeof(r));
	gettimeofday(&r.beg, NULL);	
	return r;
}

void timer_begin(sgen_timer_t *timer) {
	gettimeofday(&timer->beg, NULL);
}
	
double timer_get_us(const sgen_timer_t* timer) {
	struct timeval end;
	memset(&end, 0, sizeof(end));
	gettimeofday(&end, NULL);
	return (end.tv_usec + end.tv_sec*1000000) - (timer->beg.tv_usec + timer->beg.tv_sec*1000000);
}

double timer_get_s(const sgen_timer_t* timer) {
	return timer_get_us(timer)/1000000.0;
}
	
double timer_get_ms(const sgen_timer_t* timer) {
	return timer_get_us(timer)/1000.0;
}
	
char *timer_report(sgen_timer_t *timer) {
	double t_us = timer_get_us(timer);

#define TIMER_REPORT_STR_MAX 128
	char *buf = malloc(TIMER_REPORT_STR_MAX);

	if (t_us > 100000.0) {
		sprintf(buf, "%.2f s", t_us/1000000.0);
	} else if (t_us > 100.0)  {
		sprintf(buf, "%.2f ms", t_us/1000.0);
	} else { 
		sprintf(buf, "%.2f us", t_us);
	}

	return buf;
}
