/* Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 28, 2009
 */

/**
 * low.c
 * Generates a list of words from a set of input files or directories.
 * A new thread is created for each directory being processed.
 */
#define _POSIX_C_SOURCE 200112L
#define _ISOC99_SOURCE
#define __EXTENSIONS__

/* ---- Includes ---- */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include "low-netbase.h"

/* ---- Flags ---- */
#define CASE_INSENSITIVE 0x8000
#define APOSTROPHES	 0x4000
#define HYPHENS		 0x2000
#define READ_DOT_FILES	 0x0800
#define QUIET_FILENAME	 0x0400
#define NO_SYMLINKS	 0x0200
#define THREAD_LIMIT	 0x0020
#define OUTPUT_NOFILE	 0x0002
#define OUTPUT_TOTAL	 0x0004
#define OUTPUT_NOPAGE	 0x0008

/* ---- Useful constants ---- */
#define DO_STDIN '-'
#define THIS_DIR "."
#define PARENT_DIR ".."
#define ERROR_MSG_LEN   79

/* --- Global variables.  -------------------------------------------
 * These amount to a few bytes, _never_ change once they've
 * been set up by the option flags, and clutter up my parameter lists!
 * Not to mention, threads_left and the stats HAVE to be global.
 * Such is my justification for making these global...  Hope that's OK. 
 * ------------------------------------------------------------------
 */
size_t path_size;
pthread_mutex_t output_mutex;
pthread_mutex_t stack_mutex;
pthread_mutex_t chdir_mutex;
pthread_mutex_t child_list_mutex;

/* ------------------------------------------------------------------
 * Linked list stuff.  We maintain a list of currently open 
 * directories during a traversal so we can detect symlink loops.
 * ------------------------------------------------------------------
 */
#define child struct CHILD
child
{
    pthread_t pid;
    child *next;
};

#define visited_dir struct VISITED_DIR
visited_dir
{
    dev_t st_dev;
    ino_t st_ino;
    child *children;
	char *path;
	visited_dir *next;
};
/* ------------------------------------------------------------------
 * Directory info is packaged into a struct so we can pass it as a
 * single parameter to pthread_create.
 * ------------------------------------------------------------------
 */
#define dir_info struct DIR_INFO
dir_info
{
    visited_dir *list;
    params *parameters;
    char *path;
    int dir_d;
};

/* ---- Forward declarations ---- */
visited_dir* add_visited(visited_dir*, char*, dev_t, ino_t);
void free_visited(visited_dir*);
int have_visited(visited_dir*, dev_t, ino_t);
dir_info* create_dir_info(params*, visited_dir*, char*, int);
void free_dir_info(dir_info*);
int enabled(params*, unsigned int);
int process_entry(params*, visited_dir*, char*, char*, int, int);
void* thread_dir_setup(void*);
void handle_directory(params*, visited_dir*, char*, dev_t, ino_t, char*, int);
void add_child(visited_dir*, pthread_t);
void wait_for_children(visited_dir*);
int explore_dir(params*, visited_dir*, char*, int);

/* lib functions that gcc complains about for some reason... */
char *realpath(const char*, char*);
int lstat(const char*, struct stat*);

extern int read_file(params*, char*);

extern void send_output(params*, long, char*);
extern int print_file_error(params*, int, char*);
extern void print_loop(params*, char*, char*, char*, visited_dir*, dev_t, ino_t);
