/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 28, 2009
 */

/******************************************************************************
 * This file declares constants, structs, and methods related to 
 * directory traversal.
 ******************************************************************************
 */

#define _POSIX_C_SOURCE 200112L
#define _ISOC99_SOURCE
#define __EXTENSIONS__

#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>


#define READ_DOT_FILES	 0x0800
#define THIS_DIR "."
#define PARENT_DIR ".."

pthread_mutex_t stack_mutex;
pthread_mutex_t chdir_mutex;

/* ------------------------------------------------------------------
 * Linked list stuff.  We maintain a list of currently open 
 * directories during a traversal so we can detect symlink loops.
 * ------------------------------------------------------------------
 */
#define child struct CHILD
child
{
    int pid;
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
    char *path;
    char *parent;
    int dir_d;
};

visited_dir* add_visited(visited_dir*, char*, dev_t, ino_t);
void free_visited(visited_dir*);
int have_visited(visited_dir*, dev_t, ino_t);
void print_loop(visited_dir*, dev_t, ino_t);
dir_info* create_dir_info(visited_dir*, char *path, char *parent, int dir_d);
void free_dir_info(dir_info*);
void handle_directory(visited_dir*, char*, dev_t, ino_t, char*, int);
int explore_dir(visited_dir*, char*, char*, int);
int process_entry(visited_dir*, char*, char*, int, int);
