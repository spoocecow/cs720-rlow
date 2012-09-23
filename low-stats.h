/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 28, 2009
 */

/******************************************************************************
 * This file declares (almost) everything to do with statistic collecting.
 * This is so I don't have any 1000+ line code files anymore :)
 ******************************************************************************
 */
#define _POSIX_C_SOURCE 200112L
#define _ISOC99_SOURCE
#define __EXTENSIONS__

#include <pthread.h>
#include "low-netbase.h"
#include "print_time.h"

#define COLLECT_STATS	 0x0040

/*int statistics[NUM_STATS+1];*/
pthread_mutex_t stats_mutex;

/* Handy enumerations for statistics */
enum STATS
{
	t_words, t_words_a, t_words_h, t_words_ah, t_words_small,
	t_words_long, t_infiles, t_dirs, t_dloops, t_dskips, t_ddepth,
	t_dotfiles, t_threadsmade, t_threadfails, t_threadskips,
	t_threadssim, t_errors, current_tcount, t_tt1, t_tt2, t_at1, t_at2
};

int read_stat(params*, int);
void update_stat(params*, int, int);
void inc_stat(params*, int, int);
void set_times(int*, time_type*, time_type*);
void accumulate_times(int*, int, int);
double get_total_time(int*);

extern int enabled(params*, unsigned int);
