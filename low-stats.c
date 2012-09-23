/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 28, 2009
 */

/******************************************************************************
 * low-stats.c
 * This file implements most of the logic needed for statistics collection.
 * (Output of stats to a file is handled by rlowc.)
 ******************************************************************************
 */

#include "low-stats.h"

/* ------------------------------------------------------------------
 * Returns the current value for the requested statistic.
 * Since it is just accessing, no lock is needed on the information.
 * stat:	Enumerated nmemonic for a statistic (see STAT in low.h)
 * ------------------------------------------------------------------
 */
int read_stat(params *P, int stat)
{
	int s;
	if (enabled(P, COLLECT_STATS))
	{
		pthread_mutex_lock(&stats_mutex);
		s = (P->statistics[stat]);	
		pthread_mutex_unlock(&stats_mutex);
		return s;
	}
	return 0;
}

/* ------------------------------------------------------------------
 * Updates the value for the specified statistic.  A lock is needed
 * since the statistic information is global across all threads.
 * stat:	Nmemonic for the statistic to be updated
 * val:		The new value for stat to hold
 * ------------------------------------------------------------------
 */
void update_stat(params *P, int stat, int val)
{
	if (enabled(P, COLLECT_STATS))
	{
		pthread_mutex_lock(&stats_mutex);
		(P->statistics[stat]) = val;
		pthread_mutex_unlock(&stats_mutex);
	}
}

/* ------------------------------------------------------------------
 * Special statistic update function.  Since so many statistics are
 * accumulations, for convenience this function will just increment the
 * specified statistic instead of requiring an absolute value.
 * A lock is needed since the statistic information is global across
 * all threads.
 * stat:	Nmemonic for the statistic to be updated
 * amount:	Amount to increment the statistic by.  Usually 1.
 * ------------------------------------------------------------------
 */
void inc_stat(params *P, int stat, int amount)
{
	if (enabled(P, COLLECT_STATS))
	{
		pthread_mutex_lock(&stats_mutex);
		P->statistics[stat] += amount;
		pthread_mutex_unlock(&stats_mutex);
	}
}

/* ------------------------------------------------------------------
 * Does the dirty work of casting time info into four ints.
 * ------------------------------------------------------------------
 */
void set_times(int *stats, time_type *begin, time_type *end)
{
    double time;
    double avg = 0.0;
    int words = stats[t_words];
    time = split_difference(begin, end, &stats[t_tt1], &stats[t_tt2]);
    if (words > 0)
        avg = time / words;
    split_time_double(avg, &stats[t_at1], &stats[t_at2]);

}

/* ------------------------------------------------------------------
 * Adds the given time (in ints) to the current stats
 * ------------------------------------------------------------------
 */
void accumulate_times(int *stats, int s, int us)
{
    double time;
    double avg = 0.0;
    int words = stats[t_words];
    add_times(&stats[t_tt1], &stats[t_tt2], s, us);
    time = convert_time(stats[t_tt1], stats[t_tt2]);
    if (words > 0)
        avg = time / words;
    split_time_double(avg, &stats[t_at1], &stats[t_at2]);

}

/* ------------------------------------------------------------------
 * Returns the total processing time as a double
 * ------------------------------------------------------------------
 */
double get_total_time(int *stats)
{
    /*
    total =(double) ((stats[t_tt1] << 32) | stats[t_tt2]);
    */

    return convert_time(stats[t_tt1], stats[t_tt2]);
}
