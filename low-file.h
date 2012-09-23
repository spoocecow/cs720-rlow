/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 29, 2009
 */

/******************************************************************************
 * This file declares methods used to actually process the files we discover.
 ******************************************************************************
 */

#define _POSIX_C_SOURCE 200112L
#define _ISOC99_SOURCE
#define __EXTENSIONS__

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include "low-netbase.h"

#define WHITE_SPACE " \n\r\t\v\f"
#define NEW_PAGE '\f'
#define NEW_LINE '\n'
#define APOSTROPHE '\''
#define HYPHEN '-'

int is_word(params*, char, FILE*);
void outside_word(params*, char*, int, char*, int, int, int, int);
int read_file(params*, char*);

extern int enabled(params*, unsigned int);
extern void print_word(params*, char*, char*, int, int);
extern void send_output(params*, long, char*);
