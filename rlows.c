/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 29, 2009
 */

/******************************************************************************
 * Remote 'low' server.  Operates in exactly the same way as previous programs,
 * except we do not do our own command line parsing (the client will send the
 * flags and constants needed) or output (that is instead sent back to the
 * client).
 ******************************************************************************
 */

#include "rlows.h"
#define TEXT_SIZE 1024

void listener() /* borrowed from server1tcp.c */
{
	pthread_t agent;
	socklen_t len;
	int listening_fd, client_fd;
	struct sockaddr server, client;
	struct sockaddr_in *iptr;
	char text_buf[INET_ADDRSTRLEN];
	s_agent_info *a;
	no_sigpipe();
	listening_fd = openlistener(NULL, NULL, &server);
	if (listening_fd < 0)
		exit(EXIT_FAILURE);

	while(1)
	{
		iptr = (struct sockaddr_in *) &server;
		if (inet_ntop(iptr->sin_family, &iptr->sin_addr, text_buf, 
			INET_ADDRSTRLEN) == NULL )
		{
			perror("inet_ntop server");
			exit(EXIT_FAILURE);
		}

		printf("\nserver listening at %s port %d\n", text_buf, 
				ntohs(iptr->sin_port));
		len = sizeof(client);
		if ((client_fd = accept(listening_fd, &client, &len)) < 0)
		{
			perror("server accept");
			exit(EXIT_FAILURE);
		}
		a = malloc(sizeof(s_agent_info));
		a->client_fd = client_fd;
		a->client = &client;
		if (pthread_create(&agent, NULL, server_agent_setup, a) != 0)
		{
			fprintf(stderr, "Could not create thread to handle ");
			fprintf(stderr, "remote client.\n");
			free(a);
		}
		/*pthread_detach(&agent);*/
	}
	if (close(listening_fd) < 0)
	{
		perror("server close");
		exit(EXIT_FAILURE);
	}
}

/* ------------------------------------------------------------------
 * Tiny helper function to print that a file/directory read error
 * occurred, if appropriately enabled by the user flags.
 * Always returns 0.
 * ------------------------------------------------------------------
 */
int print_file_error(params *P, int myerrno, char *filename)
{
	char *msg, *errmsg;
	int len;
	if (!enabled(P, QUIET_FILENAME))
	{
		len = strlen(my_address) + strlen(filename) + 1;
		if (myerrno == 0)
		{
			/* specifically passed 0 = can't open filename */
			len += 24;
			if ((msg = malloc(len)) == NULL)
			{
				fprintf(stderr, "Malloc failed in p_f_e.\n");
				return 0;
			}
			snprintf(msg, len, "%s/%s could not be opened.\n",
				my_address, filename);
			free(msg);
			return 0;
		}
		else if ((errmsg = malloc(ERROR_MSG_LEN)) == NULL)
		{
			fprintf(stderr, "Malloc failed in print_file_error\n");
			return 0;
		}
		else
		{
			#if defined(__sun__)
			if ((errmsg = strerror(myerrno)) == NULL)
			#else
			if (strerror_r(myerrno, errmsg, ERROR_MSG_LEN) != 0)
			#endif
			{
				fprintf(stderr, "strerror_r failed.\n");
				return 0;
			}
		}
		len += strlen(errmsg) + 4;
		if ((msg = malloc(len)) != NULL)
		{
			snprintf(msg, len, "%s/%s: %s\n", 
				my_address, filename, errmsg);
			send_output(P, P->error_fd, msg);
			free(msg);
		}
		else
			send_output(P, P->error_fd, errmsg);	
		free(errmsg);
	}
	else
		inc_stat(P, t_errors, 1);
	return 0;
}

/* ------------------------------------------------------------------
 * Prints directory error information when a loop has been detected.
 * ------------------------------------------------------------------
 */
void print_loop(params *P, char *filename, char *path, char *abspath, 
		visited_dir *list, dev_t st_dev, ino_t st_ino)
{
	visited_dir *p;
	char *s;
	int len;
	len = 36 + (2 * strlen(path)) + (2 * strlen(abspath)) + 
		strlen(filename) + (3 * strlen(my_address));
	s = malloc(len);
	snprintf(s,len,"*** Loop detected in %s/%s/%s -> %s/%s\n%s/%s\n%s/%s\n",
			my_address, path, filename, my_address, abspath, 
			my_address, abspath, my_address, path);
	pthread_mutex_lock(&output_mutex);
	send_output(P, P->error_fd, s);
	pthread_mutex_lock(&stack_mutex);
	p = list;
	while ((st_dev != p->st_dev) || (st_ino != p->st_ino))
	{
		len = strlen(my_address) + strlen(p->path) + 3;
		s = malloc(len);
		snprintf(s, len, "%s/%s\n", my_address, p->path);
		send_output(P, P->error_fd, s);
		free(s);
		p = p->next;
	}
	pthread_mutex_unlock(&stack_mutex);
	pthread_mutex_unlock(&output_mutex);
}


/* ------------------------------------------------------------------
 * Prints end-of-word output for the program.  Output format depends on
 * what flags are set.  We unfortunately need to be kinda unwieldy because
 * we want to reduce the output to stdout to as few lines as possible.
 * So, we malloc a string to hold the entire line, then format that string
 * appropriately, using snprintf.  Then, that formatted line is output
 * exactly once.
 * flags:	Input/output flags.
 * filename:	The source filename.
 * p:		The page number.  Upon the closing of a file, this method
 *		is called with p=0 and l=wordcount for the -t option.
 * l:		The line number.  This value is equal to word count when
 *		-t option is used (and p=0).
 * ------------------------------------------------------------------
 */
void print_word(params *P, char *word, char *filename, int p, int l)
{
	char *line;
	int linelen;
	linelen = strlen(word) + strlen(filename) + strlen(my_address) + 33;
	if ((line = malloc(linelen)) == NULL)
	{
        fprintf(stderr, "Malloc failed in read_file.\n");
		return;
	}
	line[0] = '\0';
	if (p && !enabled(P, OUTPUT_TOTAL))
	{
		if (!enabled(P, OUTPUT_NOFILE) && (*filename != DO_STDIN))
		{
			if (!enabled(P, OUTPUT_NOPAGE))
			{
				snprintf(line, linelen, "%s\t%s/%s\t%d\t%d\n",
					word, my_address, filename, p, l);
			}
			else
			{
				snprintf(line, linelen, "%s\t%s/%s\t%d\n",
					word, my_address, filename, l);
			}
		}
		else
		{
			if (!enabled(P, OUTPUT_NOPAGE))
			{
				snprintf(line, linelen, "%s\t%d\t%d\n",
					word, p, l);
			}
			else
			{
				snprintf(line, linelen, "%s\t%d\n", word, l);
			}
		}
		send_output(P, P->output_fd, line);
	}
	else if (!p && enabled(P, OUTPUT_TOTAL) )
	{
		if (!enabled(P, OUTPUT_NOFILE) &&(*filename != DO_STDIN))
		{
			snprintf(line, linelen, "%d\t%s/%s\n", l, 
				my_address, filename);
		}
		else
		{
			snprintf(line, linelen, "%d\n", l);
		}
		send_output(P, P->output_fd, line);
	}

	free(line);
}

/* ------------------------------------------------------------------
 * Performs the local output of an output string to stdout.  This is a separate
 * function so that the client and server can have their own versions, and the
 * server can send the formatted string straight through the socket fd :)
 * ------------------------------------------------------------------
 */
void send_output(params *P, long output_fd, char *line)
{
	char *packet_type = P_TYPE_OUTPUT;
	int len = strlen(line) + 1;
	if (output_fd == P->error_fd)
		packet_type = P_TYPE_ERROR;
	if (send_packet(P->output_fd, packet_type, line, len) == -1)
		fprintf(stderr, "writing output failed. :(\n");
}

/* ------------------------------------------------------------------
 * The meat of the remote low server.  This is the work that each server agent
 * performs for its client.  Basically this accepts the parameter information,
 * then begins processing.  At the end, the collected statistics are sent back.
 * ------------------------------------------------------------------
 */
void* server_agent_setup(void *info)
{
	int i, len, size;
	long client_fd;
	struct sockaddr_in *iptr;
	char text_buf[TEXT_SIZE];
	char *pathname;
	char *ptype;
	params *netP;
	params *P;
	s_agent_info *a = (s_agent_info*) info;
	client_fd = a->client_fd;
	iptr = (struct sockaddr_in *) a->client;
	if (inet_ntop(iptr->sin_family, &iptr->sin_addr, text_buf, TEXT_SIZE)
			== NULL)
	{
		perror("inet_ntop client");
		exit(EXIT_FAILURE);
	}
	P = malloc(sizeof(params));
	netP = malloc(sizeof(params));
	ptype = malloc(2);
	pathname = malloc(1);
	fprintf(stderr, "server connected to client at %s port %d\n", 
		text_buf, ntohs(iptr->sin_port));
	len = PARAM_SIZE;
	if ((size = recv_packet(client_fd, ptype, (char**) &netP, &len)) == -1)
	{
		fprintf(stderr, "Uh oh, no params received.\n");
	}
	if (!strcmp(ptype, P_TYPE_PARAMS))
	{
		fprintf(stderr, "Unexpected packet type received.\n");
	}
	if ((ntoh_params(netP,P)) == 0)
	{
		fprintf(stderr, "Failed to convert parameters. Exiting.\n");
		return 0;
	}

	/* receive server address string */
	if ((size = recv_packet(client_fd, ptype, &my_address, &len)) == -1)
	{
		fprintf(stderr, "Didn't get server address string. :(\n");
		
	}
	if (!strcmp(ptype, P_TYPE_PARAMS))
	{
		fprintf(stderr, "Unexpected packet type received.\n");
	}
	/* receive pathname */
	len = 1;
	if ((size = recv_packet(client_fd, ptype, &pathname, &len)) == -1)
	{
		fprintf(stderr, "Didn't get path to read. :(\n");
		return 0;
		
	}
	if (!strcmp(ptype, P_TYPE_PARAMS))
	{
		fprintf(stderr, "Unexpected packet type received.\n");
	}
	pathname[size] = '\0';


	P->output_fd = client_fd;
	P->error_fd = -1;
	
	for (i = 0; i < NUM_STATS; i++)
		P->statistics[i] = 0;
	begin_processing(P, client_fd, pathname);

	if (enabled(P, COLLECT_STATS))
	{
		if ((hton_params(P, netP)) == 0)
		{
			fprintf(stderr, "Stats conversion failed.\n");
		}
		len = PARAM_SIZE + 1;
		if (send_packet(client_fd, P_TYPE_STATS, 
			(char*) netP, len) != len)
		{
			fprintf(stderr, "Sending statistics failed.\n");
		}

		/*free(netP);*/
	}
	free(netP);
	free(P);

	if (close(a->client_fd) < 0)
	{
		perror("server close connection to client");
		exit(EXIT_FAILURE);
	}
	printf("server done with client at %s port %d.\n", text_buf, 
			ntohs(iptr->sin_port));
	return 0;
}

void begin_processing(params *P, int client_fd, char *pathname)
{
	int dir_depth; /* locally-scoped option values */
	int i;
	char *in, *cwd;
	time_type t_begin, t_end;
	visited_dir *home;
	/* following is taken from Dr. Russell's getchdir.c demo */
	#if defined(PATH_MAX)
		char fullpath[PATH_MAX];
		path_size = PATH_MAX;
	#else
		char *fullpath;
		if ((path_size = pathconf(".", PCPATH_MAX)) == -1)
		{
			perror("current working directory");
			exit(EXIT_FAILURE);
		}
		if ((fullpath = malloc(path_size)) == NULL)
		{
			fprintf(stderr, "No space for %ld bytes\n",
					(long int) path_size);
			exit(EXIT_FAILURE);
		}
	#endif
	/* now, fullpath will be the absolute pathname buffer for filename
	 * output for the rest of the program */
	
	if (getcwd(fullpath, path_size) == 0)
	{
		fprintf(stderr, "Could not get current working directory.\n");
		exit(EXIT_FAILURE);
	}
	
	if ((cwd = malloc(path_size)) == NULL)
	{
		fprintf(stderr, "Could not malloc %ldb.\n", (long) path_size);
		exit(EXIT_FAILURE);
	}
	
	/* savin' the current working directory */
	strncpy(cwd,fullpath, path_size);
	home = add_visited(0, cwd, -1, -1);

	in = malloc(1);
	*in = DO_STDIN;
	i = 0;
	dir_depth = P->max_dir_depth;
	pthread_mutex_init(&stats_mutex, NULL);
	pthread_mutex_init(&stack_mutex, NULL);
	pthread_mutex_init(&output_mutex, NULL);
	pthread_mutex_init(&chdir_mutex, NULL);
	pthread_mutex_init(&child_list_mutex, NULL);
	get_time(&t_begin);
	process_entry(P, home, pathname, fullpath, dir_depth, 0);
	wait_for_children(home);
	get_time(&t_end);
	set_times(P->statistics, &t_begin, &t_end);
	chdir(cwd);
}

int main(int argc, char *argv[])
{
	pthread_mutex_init(&packet_xfer_mutex, NULL);
	my_address = malloc(ADDRESS_LENGTH);
	listener();
	return EXIT_SUCCESS;
}
/* vi: set autoindent tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
