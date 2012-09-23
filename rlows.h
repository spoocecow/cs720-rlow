/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 29, 2009
 */

/******************************************************************************
 * rlows.h
 * This file declares constants and methods which are ONLY needed by the server
 * (and NOT the client).  This basically means the server-side connection
 * handling, and sending information to the client instead of outputting it
 * ourselves.
 ******************************************************************************
 */

#define _POSIX_C_SOURCE 200112L
#define _ISOC99_SOURCE
#define __EXTENSIONS__

#include <stdio.h>
#include <netdb.h>
#include "low-base.h"
#include "low-netbase.h"
#include "low-file.h"
#include "low-stats.h"
#include "print_time.h"

/* Struct to store information that a server agent needs to know */
#define s_agent_info struct S_AGENT_INFO
s_agent_info
{
    int client_fd;
    struct sockaddr *client;
};

char *my_address;

void print_word(params*, char*, char*, int, int);
int print_file_error(params*, int, char*);
void print_loop(params*, char*, char*, char*, visited_dir*, dev_t, ino_t);
void send_output(params*, long, char*);
void listener(void);
void* server_agent_setup(void*);
void begin_processing(params*, int, char*);
void send_params(params*, int);
