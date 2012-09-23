/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * December 08, 2009
 */

/**********************************************************
 * This class contains some helpful functions for networking that both
 * client and server can use.
 **********************************************************
 */

#define POSIX_C_SOURCE 200112L
#define _ISOC99_SOURCE
#define __EXTENSIONS__

#include "low-netbase.h"

/*
void print_params(params *P)
{
	int i;
	char *labels[NUM_STATS] = {"Total words found", 
	"Total words with apostrophes", "Total words with hyphens",
	"Total words with apostrophes and hyphens", "Total words too small",
	"Total words too long", "Total input files processed",
	"Total directories processed", "Total directory loops avoided",
	"Total directory descents pruned", "Maximum directory depth achieved",
	"Total dot names ignored", "Total threads created", 
	"Total thread creation failures", "Total thread creations pruned",
	"Maximum simultaneous threads achieved", "Total ignored errors"};

	if (P == NULL)
	{
		fprintf(stderr, "[print_params]:  passed a null pointer!!\n");
		return;
	}
	fprintf(stderr, "Parameters:\n-----------\n");
	fprintf(stderr, "\tflags = %x\n", P->flags);
	fprintf(stderr, "\tmax_words = %x\n", P->max_words);
	fprintf(stderr, "\tmax_dir_depth = %x\n", P->max_dir_depth);
	fprintf(stderr, "\tmax_word_length = %x\n", P->max_word_length);
	fprintf(stderr, "\tmin_word_length = %x\n", P->min_word_length);
	fprintf(stderr, "\tthreads_left = %x\n", P->threads_left);
	fprintf(stderr, "\toutput_fd = %x\n", P->output_fd);
	fprintf(stderr, "\terror_fd = %x\n", P->error_fd);
	fprintf(stderr, "Statistics:\n-----------\n");
	for (i = 0; i < NUM_STATS; i++)
	{
		fprintf(stderr, "\t%s: %x\n", labels[i], P->statistics[i]);
	}
	fprintf(stderr, "\n");
}
*/

/* ------------------------------------------------------------------
 * Converts a parameters struct from the host format to network format.
 * ------------------------------------------------------------------
 */
int hton_params(params *P, params *Q)
{
	int i;
	int *pptr;
	int *qptr;
	int max;
	if (!P || !Q)
		return 0;
	i = 0;
	max = PARAM_SIZE / sizeof(int);
	pptr = (int*) &P[0];
	qptr = (int*) &Q[0];
	for (i = 0; i < max; i+= 1)
	{
		qptr[i] = htonl(pptr[i]);
	}
	return 1;
}

/* ------------------------------------------------------------------
 * Converts a parameters struct from network format to the host format.
 * ------------------------------------------------------------------
 */
int ntoh_params(params *P, params *Q)
{
	int i;
	int *pptr;
	int *qptr;
	int max;
	if (!P || !Q)
		return 0;
	i = 0;
	max = PARAM_SIZE / sizeof(int);
	pptr = (int*) &P[0];
	qptr = (int*) &Q[0];
	for (i = 0; i < max; i+= 1)
	{
		qptr[i] = ntohl(pptr[i]);
	}
	return 1;
}


/* ------------------------------------------------------------------
 * Transmits a packet with the specified type and message to the recipient at
 * the other end of the socket denoted by fd.  No error checking is done for
 * validity of the parameters.
 * fd:		File descriptor of socket to send packet to
 * packet_type:	Single character used to describe whether the packet contains:
 *		['P'] Setup parameters (initial client->server packet)
 *		['O'] Formatted standard output (regular server->client pkt.)
 *		['E'] Formatted error output (error server->client pkt.)
 *		['S'] Statistic information (optional final server->client pkt)
 * msg:		Data to send.
 * msg_len:	Length of buffer msg.
 *
 * Returns:	Number of bytes from msg actually sent on success.
 *		-1 on error.
 * ------------------------------------------------------------------
 */
int send_packet(long fd, char *packet_type, char *msg, int msg_len)
{
	int net_len;
	int sent;
	net_len = htonl(msg_len);
	pthread_mutex_lock(&packet_xfer_mutex);
	if (writeblock(fd, (char*) &net_len, HEADER_SIZE) != HEADER_SIZE)
	{
		pthread_mutex_unlock(&packet_xfer_mutex);
		return -1;
	}
	if (writeblock(fd, packet_type, P_TYPE_SIZE) != P_TYPE_SIZE)
	{
		pthread_mutex_unlock(&packet_xfer_mutex);
		return -1;
	}
	if ((sent = writeblock(fd, msg, msg_len)) != msg_len)
	{
		pthread_mutex_unlock(&packet_xfer_mutex);
		return -1;
	}
	pthread_mutex_unlock(&packet_xfer_mutex);
	return sent;
}

/* ------------------------------------------------------------------
 * Receives a packet from the socket specified by fd.
 * fd:			File descriptor of socket to receive packet from
 * recv_msg_type:	Buffer to store single character denoting the packet's
 *			type.  Assumed to already be allocated.
 * recv_msg_buf:	Buffer to store packet's data.  Does not need to be
 *			already allocated.  Will be allocated to at least 
 *			the length of the received message + 1 
 *			and null-terminated at function completion.
 * msg_buf_len:		Length of buffer recv_msg_buf.  Is modified when
 *			realloc()ing the buffer is necessary.
 *
 * Returns:		Length of received message on success.
 *			-1 on error. *
 * ------------------------------------------------------------------
 */
int recv_packet(long fd, char *recv_msg_type, char **recv_msg_buf, 
		int *msg_buf_len)
{
	int recv;
	int net_len;
	char *tmp_buf;
	pthread_mutex_lock(&packet_xfer_mutex);
	if (readblock(fd, (char*) &net_len, HEADER_SIZE) != HEADER_SIZE)
	{
		pthread_mutex_unlock(&packet_xfer_mutex);
		return -1;
	}
	if (readblock(fd, recv_msg_type, P_TYPE_SIZE) != P_TYPE_SIZE)
	{
		pthread_mutex_unlock(&packet_xfer_mutex);
		return -1;
	}
	recv = ntohl(net_len);
	if (recv > *msg_buf_len)
	{
		if ((tmp_buf = realloc(*recv_msg_buf, recv+BUFFER_INCREM))
				== NULL)
		{
			fprintf(stderr, "Realloc in recv_packet failed.\n");
			pthread_mutex_unlock(&packet_xfer_mutex);
			return -1;
		}
		*recv_msg_buf = tmp_buf;
		*msg_buf_len = recv+BUFFER_INCREM;
	}
	if (readblock(fd, *recv_msg_buf, recv) != recv)
	{
		pthread_mutex_unlock(&packet_xfer_mutex);
		return -1;
	}
	pthread_mutex_unlock(&packet_xfer_mutex);
	return recv;

}
/* vi: set autoindent tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
