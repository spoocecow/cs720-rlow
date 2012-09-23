/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * December 06, 2009
 */

/**********************************************************
 * This class contains common networking information needed by
 * both the client and server.
 **********************************************************
 */
#ifndef LOW_NETBASE_H
#define LOW_NETBASE_H


#define _POSIX_C_SOURCE 200112L
#define _ISOC99_SOURCE
#define __EXTENSIONS__

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include "tcpblockio.h"
#include "no_sigpipe.h"

#define HEADER_SIZE 4
#define P_TYPE_SIZE 1
#define BUFFER_INCREM 256
#define P_TYPE_PARAMS "P"
#define P_TYPE_OUTPUT "O"
#define P_TYPE_ERROR  "E"
#define P_TYPE_STATS  "S"
#define NUM_STATS 17
#define ADDRESS_LENGTH 22
#define PARAM_SIZE 112

#define params struct MWNPARAMS
params
{
    unsigned int flags;
    int max_words;
    int max_dir_depth;
    int max_word_length;
    int min_word_length;
    int threads_left;
    int statistics[NUM_STATS+5]; /* stats, +curr_threads, ++time, ++avgtime */
    long output_fd;
    long error_fd;
};
#endif

pthread_mutex_t packet_xfer_mutex;

int hton_params(params*, params*);
int ntoh_params(params*, params*);

/*void print_params(params*);*/

int send_packet(long fd, char *packet_type, char *msg, int msg_len);
int recv_packet(long fd, char *recv_msg_type, char **recv_buf, int *msg_buf_len);
