/* print_time.h - header file for
 *		  program to print out time differences in microseconds
 *
 * modified by Mark Niemeyer 12.10.09 :)
 */

#ifndef PRINT_TIME_H
#define PRINT_TIME_H

#include <unistd.h>
#include <stdio.h>

#if defined(_POSIX_TIMERS) && _POSIX_TIMERS != -1

	/* use the newer Posix high-resolution time facilities */
	/* must be linked on all machines with -lrt */

	#include <time.h>

	#define PER_SECOND	1000000000
	#define time_type	struct timespec
	#define fraction_field	tv_nsec
	#define get_time(where)	clock_gettime(CLOCK_REALTIME, where)

#else

	/* use the older (BSD) high-resolution time facilities */

	#include <sys/time.h>

	#define PER_SECOND	1000000
	#define time_type	struct timeval
	#define fraction_field	tv_usec
	#define get_time(where)	gettimeofday(where, NULL)

#endif

/* performs the actual arithmetic to determine the difference between times */
double get_difference(time_type *before, time_type *after);

/* returns the time difference in two int containers, for s and us */
double split_difference(time_type*, time_type*, int*, int*);

/* converts a split time stored in 2 int containers to a double */
double convert_time(int, int);

void add_times(int*, int*, int, int);

void split_time_double(double, int*, int*);
#endif
