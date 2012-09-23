/* print_time.c - function to print out time differences in microseconds */
/* modified by Mark Niemeyer 12.10.09  :) */

#define _POSIX_C_SOURCE 200112L
#define _ISOC99_SOURCE
#define __EXTENSIONS__

#include <stdio.h>

#include "print_time.h"

/* performs the actual arithmetic to determine the difference between times */
double get_difference(time_type *start, time_type *done)
{
	int secs, fraction;
	secs = done->tv_sec;
	fraction = done->fraction_field;
	if (start != NULL) {
		secs -= start->tv_sec;
		fraction -= start->fraction_field;
		if (fraction < 0) {
			secs--;
			fraction += PER_SECOND;
		}
	}
	return ((double)secs) + ((double)fraction)/((double)PER_SECOND);
}

/* returns the time difference as a double (for reference)
 * and adds the difference to the passed two int containers */
double split_difference(time_type *start, time_type *done, int *s, int *us)
{
	int secs, fraction;
	secs = done->tv_sec;
	fraction = done->fraction_field;
	if (start != NULL) {
		secs -= start->tv_sec;
		fraction -= start->fraction_field;
		if (fraction < 0) {
			secs--;
			fraction += PER_SECOND;
		}
	}
        add_times(s, us, secs, fraction);
        return convert_time(*s, *us);
}

double convert_time(int secs, int fraction)
{
	return ((double)secs) + ((double)fraction)/((double)PER_SECOND);
}

/* adds the destination and source seconds and microseconds, storing
 * them in destinations */
void add_times(int *ds, int *dus, int ss, int sus)
{
    *ds += ss;
    *dus += sus;
    if (*dus > PER_SECOND)
    {
        *ds += 1;
        *dus -= PER_SECOND;
    }
}

void split_time_double(double time, int *s, int *us)
{
    *s = (int) time;
    *us = (int) (time - ((double) *s)) * PER_SECOND;
}

/* vi: set autoindent tabstop=8 shiftwidth=8 : */
