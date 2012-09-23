/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 28, 2009
 */

/*
 * rlowc.h
 * This file declares constants and methods which are ONLY needed by the client
 * (and NOT the server).  Which basically means the command-line processing
 * and writing the statistics to a file at the end.
 */

#define _POSIX_C_SOURCE 200112L
#define _ISOC99_SOURCE
#define __EXTENSIONS__

#include <stdio.h>
#include <string.h>
#include "low-base.h"
#include "low-netbase.h"
#include "low-file.h"
#include "low-stats.h"
#include "print_time.h"

#define OPTIONS ":iahtpfm:Ad:qlL:T:U:S:"
#define OPTION_ERROR	 0x1111
#define DIGITS "0123456789"

char *stat_file;
int active_agent_threads;
pthread_mutex_t agent_mutex;
pthread_cond_t agents_done;

void write_stats(params*, FILE*);
int scan_switch_number(int, int*);
void print_help(void);
int process_options(int, char**, int*, int*, int*, int*, int*);
void print_word(params*, char*, char*, int, int);
int print_file_error(params*,int, char*);
void print_loop(params*, char*, char*, char*, visited_dir*, dev_t, ino_t);
void send_output(params*, long, char*);
int is_remote_name(char*, char**, char**);
void create_connection(params*, char*, int, char*);
void* client_agent_setup(void*);
void merge_stats(int*, int*);

/* Labels for statistics.  Having this in array form makes printing
 * super easy! */
static const char *statistic_labels[NUM_STATS] = {"Total words found", 
	"Total words with apostrophes", "Total words with hyphens",
	"Total words with apostrophes and hyphens", "Total words too small",
	"Total words too long", "Total input files processed",
	"Total directories processed", "Total directory loops avoided",
	"Total directory descents pruned", "Maximum directory depth achieved",
	"Total dot names ignored", "Total threads created", 
	"Total thread creation failures", "Total thread creations pruned",
	"Maximum simultaneous threads achieved", "Total ignored errors"};

/* Struct to store information that a client agent needs to know */
#define c_agent_info struct C_AGENT_INFO
c_agent_info
{
	char *node;
	char *port;
	char *path;
	params *parameters;
};
