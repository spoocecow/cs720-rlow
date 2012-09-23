/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 28. 2009
 */

/******************************************************************************
 * rlowc.c
 * Remote 'low' client.  Operates in much the same way as previous programs,
 * except for when remote names are detected in the list of input file names.
 * In this case, a connection is made to the specified remote 'low' server,
 * and the processing is performed server-side, with output and statistics
 * being sent back to the client as necessary.
 ******************************************************************************
 */

#include "rlowc.h"

/* ------------------------------------------------------------------
 * Writes statistics to specified file.
 * f:	File stream to write to.
 * ------------------------------------------------------------------
 */
void write_stats(params *P, FILE *f)
{
	int i;
	double avg = 0.0;
	double duration = get_total_time(P->statistics);
	if (P->statistics[t_words] > 0)
		avg = duration / P->statistics[t_words];
	for (i = 0; i < NUM_STATS; i++)
		fprintf(f,"%s: %d\n", statistic_labels[i], (P->statistics[i]));
	fprintf(f, "Total processing time: %10.6f\n", duration);
	fprintf(f, "Average processing time per word found: %10.6f\n", avg);
}


/* ------------------------------------------------------------------
 * Taken from Dr. Russell's getopttest.c demo.
 * Scans the string pointed to by optarg and tries to convert it to a number.
 * Returns 0 if successful (and stores the number in result),
 *	  -1 on any error (prints an error message and leaves result unchanged)
 * ------------------------------------------------------------------
 */
int scan_switch_number(int switch_char, int *result)
{
	int temp, retval;
	char *ptr;

	errno = 0;
	temp = strtol(optarg, &ptr, 10);
	if (errno != 0  ||  ptr == optarg  ||
	   strspn(ptr, WHITE_SPACE) != strlen(ptr)) {
		fprintf(stderr,"Illegal numeric value \"%s\" for switch -%c\n",
			optarg, switch_char);
		retval = -1;
	} else {
		*result = temp;
		retval = 0;
	}
	return retval;
}

/* ------------------------------------------------------------------
 * Prints the help menu.
 * ------------------------------------------------------------------
 */
void print_help(void)
{
	fprintf(stdout, "\nfile input options:\n");
	fprintf(stdout, "-A:\tprocess files found in directories ");
	fprintf(stdout, "which start with '.' (except . and ..)\n");
	fprintf(stdout, "-d [number]:\tlimit depth of directory recursion\n");
	fprintf(stdout, "-q:\tsilently ignore filename or access errors\n");
	fprintf(stdout, "-l:\tdo not follow sym. links found in directories\n");
	fprintf(stdout, "-T [number]:\tmaxmimum number of threads to use\n");

	fprintf(stdout, "\nfile parsing options:\n");
	fprintf(stdout, "-i:\tconvert to lowercase\n");
	fprintf(stdout, "-a:\tsupport apostrophes\n");
	fprintf(stdout, "-h:\tsupport hyphens\n");
	fprintf(stdout, "-m [number]:\tmax # of words to read in any 1 file\n");
	fprintf(stdout, "-L [number]:\tminimum word length\n");
	fprintf(stdout, "-U [number]:\tmaximum word length\n");
	
	fprintf(stdout, "\noutput options:\n");
	fprintf(stdout, "-t:\toutput total word count for each file only\n");
	fprintf(stdout, "\t\"total#\tfilename\"\n");
	fprintf(stdout, "-p:\toutput overall line number (if -t not enabled)");
	fprintf(stdout, "\n\t\"word\tfilename\tlinenumber\"\n");
	fprintf(stdout, "-f:\tdo not include filename in output\n");
	fprintf(stdout, "-S [filename]:\tlog program statistics to file\n");
}

/* ------------------------------------------------------------------
 * Sets the proper flags for input and output, as well as values for
 * certain options.
 * argc:	Amount of command line arguments (taken straight from main)
 * argv:	Actual command line arguments (again, straight from main)
 * max_words:	If -m option set, value is stored in here.  Global scope.
 * dir_depth:	If -d option set, value stored here.  Local scope.
 * min_length:	If -L option set, value stored here.  Global scope.
 * max_length:	If -U option set, value stored here.  Global scope.
 * threads_left:If -T option set, value stored here.  Global scope.
 * stats_file:	If -S option set, value stored here.  Global scope.
 * Returns:	An integer container with all necessary flags set.
 * ------------------------------------------------------------------
 */
int process_options(int argc, char *argv[], int *max_words, int *max_dir_depth,
		int *min_word_length, int *max_word_length, int *threads_left)
{
	unsigned int flags = 0;
	int c;
	while ((c = getopt(argc, argv, OPTIONS)) != -1)
	{
		switch (c) {
		case 'i': /* enable case insensitivity */
			flags |= CASE_INSENSITIVE;
			break;
		case 'a': /* enable apostrophes */
			flags |= APOSTROPHES;
			break;
		case 'h': /* enable hyphens */
			flags |= HYPHENS;
			break;
		case 'm': /* max # of words to read in 1 file */
			if (scan_switch_number(c, max_words) == -1)
				flags |= OPTION_ERROR;
			if (*max_words < 1)
			{
				flags |= OPTION_ERROR;
				fprintf(stderr, "Max words must be > 0.\n");
			}
			break;
		case 't': /* output= 'total#ofwords	filename" */
			flags |= OUTPUT_TOTAL;
			flags &= ~(OUTPUT_NOPAGE);
			break;
		case 'p': /* output= 'word	filename	totalline# */
			if ( (flags & OUTPUT_TOTAL) != OUTPUT_TOTAL )
				flags |= OUTPUT_NOPAGE;
			break;
		case 'f': /* default output= 'word	page#	line# 
				  -t output= 'total#ofwords
				  -p output= 'word	totalline# */
			flags |= OUTPUT_NOFILE;
			break;
		case 'A': /* Files found in a dir starting with . are read */
			flags |= READ_DOT_FILES;
			break;
		case 'd': /* set depth of directory recursion */
			if (scan_switch_number(c, max_dir_depth) == -1)
				flags |= OPTION_ERROR;
			if (*max_dir_depth < 0)
			{
				flags |= OPTION_ERROR;
				fprintf(stderr, "Directory recursion depth ");
				fprintf(stderr, "must be non-negative.\n");
			}
			break;
		case 'q': /* ignore filename errors */
			flags |= QUIET_FILENAME;
			break;
		case 'l': /* ignore symlinks */
			flags |= NO_SYMLINKS;
			break;
		case 'L': /* set lower bound of acceptable word length */
			if (scan_switch_number(c, min_word_length) == -1)
				flags |= OPTION_ERROR;
			if ((*min_word_length < 1) || 
				(*max_word_length && 
					(*min_word_length > *max_word_length)))
			{
				flags |= OPTION_ERROR;
				fprintf(stderr, "Minimum word length must be ");
				fprintf(stderr, "<= %d.\n", *max_word_length);
			}
			break;
		case 'U': /* set upper bound of acceptable word length */
			if (scan_switch_number(c, max_word_length) == -1)
				flags |= OPTION_ERROR;
			if (*max_word_length < *min_word_length)
			{
				flags |= OPTION_ERROR;
				fprintf(stderr, "Maximum word length must be ");
				fprintf(stderr, ">= %d.\n", *min_word_length);
			}
			break;

		case 'T': /* set limit on concurrent active threads */
			if (scan_switch_number(c, threads_left) == -1)
				flags |= OPTION_ERROR;
			if (*threads_left < 0)
			{
				flags |= OPTION_ERROR;
				fprintf(stderr, "Thread limit must be >0.\n");
			}
			flags |= THREAD_LIMIT;
			break;
		case 'S': /* collect statistics */
			flags |= COLLECT_STATS;
			stat_file = optarg;
			break;
		case ':':
			fprintf(stderr, "Missing parameter for -%c\n", optopt);
			flags |= OPTION_ERROR;
			break;
		case '?':
			fprintf(stderr, "Illegal switch '%c'\n", optopt);
			flags |= OPTION_ERROR;
			break;
		}
	}
	return flags;
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
	linelen = strlen(word) + strlen(filename) + 27;
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
				snprintf(line, linelen, "%s\t%s\t%d\t%d\n",
					word, filename, p, l);
			}
			else
			{
				snprintf(line, linelen, "%s\t%s\t%d\n",
					word, filename, l);
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
			snprintf(line, linelen, "%d\t%s\n", l, filename);
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
		if (myerrno == 0)
		{
			/* specifically passed 0 = can't open filename */
			len = strlen(filename) + 24;
			if ((msg = malloc(len)) == NULL)
			{
				fprintf(stderr, "Malloc failed in p_f_e.\n");
				return 0;
			}
			snprintf(msg,len,"%s could not be opened.\n",filename);
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
			if (strerror_r(myerrno, errmsg, ERROR_MSG_LEN) !=0)
			#endif
			{
				fprintf(stderr, "strerror_r failed.\n");
				return 0;
			}
		}
		len = strlen(errmsg) + strlen(filename) + 4;
		if ((msg = malloc(len)) != NULL)
		{
			snprintf(msg, len, "%s: %s\n", filename, errmsg);
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
	len = 31+ (2 * strlen(path)) + (2 * strlen(abspath)) + strlen(filename);
	s = malloc(len);
	snprintf(s, len, "*** Loop detected in %s/%s -> %s\n%s\n%s\n",
		path, filename, abspath, abspath, path);
	pthread_mutex_lock(&output_mutex);
	send_output(P, P->error_fd, s);
	pthread_mutex_lock(&stack_mutex);
	p = list;
	while ((st_dev != p->st_dev) || (st_ino != p->st_ino))
	{
		len = strlen(p->path) + 2;
		s = malloc(len);
		snprintf(s, len, "%s\n", p->path);
		send_output(P, P->error_fd, s);
		free(s);
		p = p->next;
	}
	pthread_mutex_unlock(&stack_mutex);
	pthread_mutex_unlock(&output_mutex);
}


/* ------------------------------------------------------------------
 * Performs the local output of an output string to stdout.  This is a separate
 * function so that the client and server can have their own versions, and the
 * server can send the formatted string straight through the socket fd :)
 * ------------------------------------------------------------------
 */
void send_output(params *P, long output_fd, char *line)
{
	fflush((FILE*) output_fd);
	fprintf((FILE*) output_fd, "%s", line);
}

/* ------------------------------------------------------------------
 * A small function to determine whether a name entered in the argument list
 * describes a remote node or a local file.  Performs syntax checking if
 * it appears to describe a remote name.  A valid remote name has the following
 * format:
 *		[node]:[service]/[name]
 *		 str	  int	 str
 * If the name is syntactically correct, the string is modified in-place
 * and pointers are returned through the node and path parameters such that
 * node points to a null-terminated remote node name, and
 * path points to a null-terminated remote path name
 *		 
 * name:	string being checked
 * Returns:	0 if local name
 *		service # if valid remote name
 *		-1 if invalid remote name
 * ------------------------------------------------------------------
 */
int is_remote_name(char *name, char **node, char **path)
{
	char *colon;
	char *slash;
	char *tmp;
	int service;
	service = -1;
	if ((colon = strchr(name, ':')) == NULL)
		return 0;
	if ((slash = strchr(name, '/')) == NULL)
		return -1;
	*colon = '\0';
	if (strlen(name) < 1) /* node must not be empty */
	{
		*colon = ':';
		return -1;
	}
	*colon = ':';
	*slash = '\0';
	colon++;
	if (strlen(colon) < 1) /* service must not be empty */
	{
		*slash = '/';
		colon--;
		return -1;
	}
	/* service must be a valid decimal integer */
	errno = 0;
	service = strtol(colon, &tmp, 10);
	if (errno != 0  ||  tmp == colon  || 
		strspn(tmp, DIGITS) != strlen(tmp))
	{
		*slash = '/';
		colon--;
		return -1;
	}

	colon--;
	*slash = '/';
	if (strlen(slash+1) < 1) /* name must not be empty */
		return -1;
	
	/* name is syntactically OK */
	/* "blank out" the colon and slash so the separate segments can be
	 * parsed as individual strings */
	*colon = '\0';
	*slash = '\0';
	slash++;
	*node = name;
	*path = slash;
	return service;
}

/* ------------------------------------------------------------------
 * Method to help create an agent for handling input from the remote server.
 * rnode:	string representation of server address
 * rport:	int representation of server port
 * rpath:	string representation of entity for server to read
 * ------------------------------------------------------------------
 */
void create_connection(params *P, char *rnode, int rport, char *rpath)
{
	pthread_t agent;
	c_agent_info *a;
	char *port;
	no_sigpipe();
	port = malloc(6); /* port cannot exceed 5 digits */
	sprintf(port, "%d", rport);
	a = malloc(sizeof(c_agent_info));
	a->node = rnode;
	a->port = port;
	a->path = rpath;
	a->parameters = P;
	pthread_mutex_lock(&agent_mutex);
	active_agent_threads++;
	if (pthread_create(&agent, NULL, client_agent_setup, a) != 0)
	{
		fprintf(stderr, "Could not create thread to handle remote ");
		fprintf(stderr, "path %s:%s/%s.\n", rnode, port, rpath);
		free(port);
		free(a);
	}
	pthread_mutex_unlock(&agent_mutex);
}

/* ------------------------------------------------------------------
 * The work function for a client agent thread connecting to a server.
 * Creates the connection, sends the necessary parameters, then receives
 * output until the termination of the connection.
 * ------------------------------------------------------------------
 */
void* client_agent_setup(void *info)
{
	int server_fd;
	int n, len, buflen;
	struct sockaddr server, client;
	char *line;
	char ip_address[ADDRESS_LENGTH];
	char server_address_str[ADDRESS_LENGTH];
	char *p_type;
	params *P;
	params *netP;
	c_agent_info *a = (c_agent_info*) info;
	server_fd = openclient(a->port, a->node, &server, &client);
	if (server_fd >= 0)
	{
		P = a->parameters;
		if (getnameinfo(&server, sizeof(server), ip_address, 
			ADDRESS_LENGTH, NULL, 0, NI_NUMERICHOST))
		{
			fprintf(stderr, "Could not get addr. of %s\n", a->node);
		}
		else
		{
			snprintf(server_address_str, ADDRESS_LENGTH, "%s:%s",
				ip_address, a->port);
			fprintf(stderr,"Connected to %s\n", server_address_str);
		}

		/* send path & parameters */
		netP = malloc(PARAM_SIZE);
		if ((hton_params(P, netP)) == 0)
		{
			fprintf(stderr, "Param conversion failed.  Dying.\n");
			free(P);
			free(a->port);
			free(a);
			pthread_mutex_lock(&agent_mutex);
			active_agent_threads--;
			if (active_agent_threads == 0)
				pthread_cond_signal(&agents_done);
			pthread_mutex_unlock(&agent_mutex);
			return 0;
		}
		len = PARAM_SIZE;
		if ((n = send_packet(server_fd, P_TYPE_PARAMS, 
			(char*) netP, len)) != len)
		{
			fprintf(stderr, "Sending parameters failed.\n");
		}
		if (send_packet(server_fd, P_TYPE_PARAMS, 
			server_address_str, ADDRESS_LENGTH) != ADDRESS_LENGTH)
		{
			fprintf(stderr, "Sending server string failed.\n");
		}
		len = strlen(a->path);
		if (send_packet(server_fd, P_TYPE_PARAMS, a->path, len) != len)
		{
			fprintf(stderr, "Sending pathname failed.  Dying.\n");
			free(P);
			free(a->port);
			free(a);
			pthread_mutex_lock(&agent_mutex);
			active_agent_threads--;
			if (active_agent_threads == 0)
				pthread_cond_signal(&agents_done);
			pthread_mutex_unlock(&agent_mutex);
			return 0;
		}


		len = 1;
		buflen = 1;
		line = malloc(len+1);
		p_type = malloc(len+1);
		p_type[1] = '\0';
		/* receive output! */
		while ((len = recv_packet(server_fd,p_type, &line,&buflen)) > 0)
		{
			line[len] = '\0';
			if (buflen != len)
				len = buflen;
			if (strcmp(p_type, P_TYPE_OUTPUT))
				send_output(P, (long) stdout, line);
			else if (strcmp(p_type, P_TYPE_ERROR))
				send_output(P, (long) stderr, line);
			else if (strcmp(p_type, P_TYPE_STATS))
			{
				/* stats are the last thing we receive */
				break;
			}
		}

		if (enabled(P, COLLECT_STATS))
		{
			if (ntoh_params((params*)line, netP) == 0)
			{
				fprintf(stderr, "Stats conversion failed.\n");
			}
			else
			{
				merge_stats(P->statistics, netP->statistics);
			}
		}
		close(server_fd);
	}
	free(a->port);
	free(a);
	pthread_mutex_lock(&agent_mutex);
	active_agent_threads--;
	if (active_agent_threads == 0)
		pthread_cond_signal(&agents_done);
	pthread_mutex_unlock(&agent_mutex);
	return 0;
}

/* ------------------------------------------------------------------
 * Having received statistics from the server, the client merges the received
 * stats back into its local copy.
 * ------------------------------------------------------------------
 */
void merge_stats(int *local_stats, int *received_stats)
{
	int i;
	pthread_mutex_lock(&stats_mutex);
	for (i = 0; i < NUM_STATS; i++)
	{
		if (i == t_ddepth || i == t_threadssim)
		{
			local_stats[i] = 
				local_stats[i] > received_stats[i] 
					? local_stats[i] 
					: received_stats[i];
		}
		else
			local_stats[i] += received_stats[i];
	}
	accumulate_times(local_stats, 
		received_stats[t_tt1], received_stats[t_tt2]);
	pthread_mutex_unlock(&stats_mutex);
}

/* ------------------------------------------------------------------
 * Main function.  Reads in options and list of filenames, then prints
 * information as requested.
 * ------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
	int dir_depth; /* locally-scoped option values */
	int max_words, max_dir_depth, max_word_length, min_word_length;
	int threads_left;
	int i;
	time_type t_begin, t_end;
	char *in, *cwd;
	double my_time;
	int net_port;
	char *net_node, *net_path;
	visited_dir *home;
	params *P;
	FILE *stats;
	int flags;
	struct stat stats_file_info;
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
	flags = 0;
	max_words = 0;
	max_dir_depth = -1;
	min_word_length = 1;
	max_word_length = 0;
	threads_left = 0;
	net_node = 0;
	net_path = 0;
	active_agent_threads = 0;
	pthread_mutex_init(&packet_xfer_mutex, NULL);
	pthread_mutex_init(&stats_mutex, NULL);
	pthread_mutex_init(&stack_mutex, NULL);
	pthread_mutex_init(&output_mutex, NULL);
	pthread_mutex_init(&chdir_mutex, NULL);
	pthread_mutex_init(&child_list_mutex, NULL);
	pthread_mutex_init(&agent_mutex, NULL);
	pthread_cond_init(&agents_done, NULL);
	
	stats = 0;



	/* Create global option flags, 
	 * store values for globally scoped option variables */
	flags = process_options(argc, argv, &max_words, &max_dir_depth,
		&min_word_length, &max_word_length, &threads_left);

	if ((flags & OPTION_ERROR) == OPTION_ERROR) /* Did errors occur? */
	{
		fprintf(stderr, "Error detected while parsing options.\n");
		print_help();
		return EXIT_FAILURE;
	}
	dir_depth = max_dir_depth;


	P = malloc(sizeof(params));
	(P->flags) = flags;
	(P->max_words) = max_words;
	(P->max_dir_depth) = max_dir_depth;
	(P->min_word_length) = min_word_length;
	(P->max_word_length) = max_word_length;
	(P->threads_left) = threads_left;
	(P->output_fd) = (long) stdout;
	(P->error_fd) = (long) stderr;

	if (enabled(P, COLLECT_STATS))
	{
		if (lstat(stat_file, &stats_file_info) < 0)
		{
			perror(stat_file);
			return EXIT_FAILURE;
		}
		if ( !S_ISREG(stats_file_info.st_mode))
		{
			fprintf(stderr, "%s is not a regular file.\n",
					stat_file);
			return EXIT_FAILURE;
		}
	}



	get_time(&t_begin);
	if (optind < argc)
	{
		/* arguments remain after option parsing -- files/directories */
		for (i = optind; i < argc; i++)
		{
			/* for special filename '-', bypass the whole realpath
			 * business */
			if (*(argv[i]) == DO_STDIN)
			{
				read_file(P, in);
			}
			else
			{
				if ((net_port = is_remote_name(
					argv[i], &net_node, &net_path)) == 0)
				{
					process_entry(P, home, argv[i],
						fullpath, dir_depth, 0);
					wait_for_children(home);
					chdir(cwd);
					home = add_visited(0,cwd,-1,-1);
				}
				else if (net_port > 0)
				{
					create_connection(P, net_node, 
						net_port, net_path);
				}
				else
				{
					fprintf(stderr, "%s does ", argv[i]);
					fprintf(stderr, "not name a valid ");
					fprintf(stderr, "remote name.\n");
				}
			}
		}
	}
	else
	{
		/* no other arguments -- read from stdin */
		read_file(P, in);
	}
	get_time(&t_end);
	my_time = get_difference(&t_begin, &t_end);
	pthread_mutex_lock(&stats_mutex);
	set_times(P->statistics, &t_begin, &t_end);
	pthread_mutex_unlock(&stats_mutex);
	pthread_mutex_lock(&agent_mutex);
	while (active_agent_threads > 0)
		pthread_cond_wait(&agents_done, &agent_mutex);
	pthread_mutex_unlock(&agent_mutex);
	if (enabled(P, COLLECT_STATS))
	{
		stats = fopen(stat_file, "w");
		write_stats(P, stats);
		fclose(stats);
	}
	
	/* making valgrind happy :) */
	free_visited(home);
	free(P);
	free(cwd);
	free(in);
	return EXIT_SUCCESS;
}

/* vi: set autoindent tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
