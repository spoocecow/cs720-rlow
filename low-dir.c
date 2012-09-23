/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 28, 2009
 */

/******************************************************************************
 * low-dir.c
 * This file implements directory traversal and all of that fun stuff.
 * Some thread logic is bunched up in here too since directory traversal
 * and thread creation are closely linked in this program.
 ******************************************************************************
 */

#include "low-dir.h"

/* ------------------------------------------------------------------
 * Adds a new directory to a list of visited directories. New directory
 * is added as the head of the list.  Pointer to new head is returned
 * (unless malloc error, then NULL is returned.)
 * ------------------------------------------------------------------
 */
visited_dir* add_visited(visited_dir *list_head, char *path, dev_t dt, ino_t it)
{
	visited_dir *v;
	if ( (v = malloc(sizeof(visited_dir))) == NULL)
	{
		fprintf(stderr, "Could not malloc %ldb.\n", 
			(long) sizeof(visited_dir));
		return NULL;
	}
	if ( (v->path = malloc(strlen(path)+1)) == NULL)
	{
		fprintf(stderr, "Could not malloc %ldb.\n",
			(long) strlen(path)+1);
		return NULL;
	}
	v->st_dev = dt;
	v->st_ino = it;
	v->children = 0;
	v->next = list_head;
	strcpy(v->path, path);
	return v;
}

/* ------------------------------------------------------------------
 * Frees a visited_dir object.
 * ------------------------------------------------------------------
 */
void free_visited(visited_dir *v)
{
	pthread_mutex_lock(&stack_mutex);
	free(v->path);
	free(v);
	pthread_mutex_unlock(&stack_mutex);
}

/* ------------------------------------------------------------------
 * Finds whether we have already visited a given directory in a list.
 * list:	Linked list of visited DIR objects
 * newdir:	Directory we're checking for
 * Returns 1 if newdir is in list
 * Returns 0 otherwise
 * ------------------------------------------------------------------
 */
int have_visited(visited_dir *list, dev_t st_dev, ino_t st_ino)
{
	visited_dir *p;
	pthread_mutex_lock(&stack_mutex);
	p = list;
	while (p)
	{
		if ((st_dev == p->st_dev) && (st_ino == p->st_ino))
		{
			pthread_mutex_unlock(&stack_mutex);
			return 1;
		}
		p = p->next;
	}
	pthread_mutex_unlock(&stack_mutex);
	return 0;
}

/* ------------------------------------------------------------------
 * Prints directory error information when a loop has been detected.
 * ------------------------------------------------------------------
 */
void print_loop(visited_dir *list, dev_t st_dev, ino_t st_ino)
{
	visited_dir *p;
	pthread_mutex_lock(&stack_mutex);
	p = list;
	while ((st_dev != p->st_dev) || (st_ino != p->st_ino))
	{
		fprintf(stderr, "%s\n", p->path);
		p = p->next;
	}
	pthread_mutex_unlock(&stack_mutex);
}

/* ------------------------------------------------------------------
 * Creates a new dir_info entry based on the current information
 * available in process_entry.
 * ------------------------------------------------------------------
 */
dir_info* create_dir_info(visited_dir *list, char *path, char *parent, int dir_d)
{
	dir_info *d;
	if ((d = malloc(sizeof(dir_info))) == NULL)
	{
		fprintf(stderr, "Could not malloc %lub.\n", sizeof(dir_info));
		return 0;
	}
	d->list = list;
	d->path = path;
	d->parent = parent;
	d->dir_d = dir_d;
	return d;
}

/* ------------------------------------------------------------------
 * Frees the memory used by a dir_info object.  For now it's just
 * got pointers so there's not much to free, but that might change
 * later.  Hopefully I remember to reflect the change in these comments.
 * ------------------------------------------------------------------
 */
void free_dir_info(dir_info *d)
{
	free(d);
}

/* ------------------------------------------------------------------
 * Given a string to a file or path, first determines the full 'real' path.
 *
 * If the filename is a symlink, we resolve the link and call this function
 * again, with the from_link flag set.  This is used in deciding which
 * parameter (filename or fullpath) to use when calling lstat, and in
 * determining whether it is necessary to check for loops in the directory
 * hierarchy.
 *
 * If the filename is actually a directory, (and is not already open),
 * explore_dir is called to take care of its entries.  If the directory is
 * found within the visited_dir list, then we have found a loop, and the
 * directory is not explored.
 *
 * Otherwise, it's a regular ol' file and we just call read_file on it.
 *
 * list:	Linked list of visited directories.  Loops are detected
 *		by comparing absolute pathnames.
 * filename:	Name of entity being examined (directory or file).
 * fullpath:	Buffer to hold the full pathname of a file.
 * dir_d:	Maxmimum allowed depth of directory recursion to perform.
 * from_link:	Flag denoting whether this function has been called from
 *		a symbolic link or not.
 * ------------------------------------------------------------------
 */
int process_entry(visited_dir *list, char *filename,
		  char *path, int dir_d, int from_link)
{
	struct stat statbuf;
	dev_t dev;
	ino_t ino;
	char *absolute_filename;
	if ((absolute_filename = malloc(path_size)) == NULL)
	{
		fprintf(stderr, "Could not malloc %ldb.\n", (long) path_size);
		return -1;
	}
	pthread_mutex_lock(&chdir_mutex);
	chdir(path);
	if (lstat(filename, &statbuf) < 0)
	{
		pthread_mutex_unlock(&chdir_mutex);
		return print_file_error(filename);
	}

	if (realpath(filename, absolute_filename) == 0)
	{
		pthread_mutex_unlock(&chdir_mutex);
		return print_file_error(filename);
	}
	if ( S_ISLNK(statbuf.st_mode))
	{ /* symbolic link: could be either file or dir, so resolve & process */
		if (!enabled(NO_SYMLINKS))
		{
			if (lstat(absolute_filename, &statbuf) < 0)
			{
				pthread_mutex_unlock(&chdir_mutex);
				return print_file_error(absolute_filename);
			}
		}
		else
			return 0;
	}
	pthread_mutex_unlock(&chdir_mutex);
	dev = statbuf.st_dev;
	ino = statbuf.st_ino;
	if (S_ISREG(statbuf.st_mode) || S_ISCHR(statbuf.st_mode))
	{ /* regular, readable file */
		read_file(absolute_filename);
		inc_stat(t_infiles, 1);

	}
	else if (!S_ISDIR(statbuf.st_mode))
	{ /* if not link, file, or dir, error (if appropriate) */
		return print_file_error(absolute_filename);
	}
		
	else if (S_ISDIR(statbuf.st_mode) && dir_d != 0)
	{ /* directory, and dir_depth allows us to explore it */
		if(have_visited(list, dev, ino))
		{ /* if loop found, print it, but don't explore */
			inc_stat(t_dloops, 1);
			if (!enabled(QUIET_FILENAME))
			{
				pthread_mutex_lock(&output_mutex);
				fprintf(stderr, "*** Loop detected ");
				fprintf(stderr, "in %s/%s -> %s\n",
					path, filename, absolute_filename);
				fprintf(stderr, "%s\n%s\n", 
					absolute_filename, path);
				print_loop(list, dev, ino);
				pthread_mutex_unlock(&output_mutex);
			}
		}
		else
		{ /* add a list node, explore the dir, then free the node */
			handle_directory(list, absolute_filename, 
				dev, ino, path, (dir_d - 1));
		}
	}
	else if (S_ISDIR(statbuf.st_mode) && dir_d == 0)
	{
		inc_stat(t_dskips, 1);
	}
	return 0;
}

/* ------------------------------------------------------------------
 * Special method for a directory to clean up after itself if it has
 * spawned any threads.
 * ------------------------------------------------------------------
 */
void handle_directory(visited_dir *list, char *nextpath, dev_t dev, ino_t ino, char *prevpath, int dir_d)
{
	pthread_t new_thread;
	dir_info *new_dir_info;
	visited_dir *parent_dir;
	parent_dir = list;
	inc_stat(t_dirs, 1);

	pthread_mutex_lock(&stack_mutex);
	list = add_visited(list, prevpath, dev, ino);
	pthread_mutex_unlock(&stack_mutex);

	pthread_mutex_lock(&stats_mutex);
	if (!enabled(THREAD_LIMIT) || threads_left > 0)
	{
		pthread_mutex_unlock(&stats_mutex);
		new_dir_info = create_dir_info(list, nextpath, prevpath, dir_d);
		if (pthread_create(&new_thread, NULL, 
			thread_dir_setup, new_dir_info) == 0)
		{
			add_child(parent_dir, new_thread);
		}
		else
		{
			inc_stat(t_threadfails, 1);
			explore_dir(list, nextpath, prevpath, dir_d);
			free_dir_info(new_dir_info);
		}
	}
	else
	{
		pthread_mutex_unlock(&stats_mutex);
		inc_stat(t_threadskips, 1);
		explore_dir(list, nextpath, prevpath, dir_d);
	}
}

/* ------------------------------------------------------------------
 * Opens and iterates through a given directory, operating on allowed
 * files found within it.  . and .. are skipped, as are files beginning
 * with . unless it's been enabled in the flags.  All other entries,
 * both files and directories, are thrown to process_entry to determine
 * whether they're a file/directory/link, and to read/explore them if
 * appropriate.  If threads are being used, they wait for their children
 * threads to terminate after they themselves are done with their work.
 *
 * flags:	The usual I/O option flags.
 * list_head:	A list of already visited directories.  Always will have
 *		the working directory low was called from as the tail,
 *		and will always have the current directory being explored
 *		as the head (so the src = list_head->next->path assignment
 *		is always safe).
 * fullpath:	The realpath of the current directory being explored.
 * dir_depth:	Passed to process_entry.
 * ------------------------------------------------------------------
 */
int explore_dir(visited_dir *list_head, char *nextpath, char *prevpath, int dir_depth)
{
	DIR *d;
	struct dirent *entry;
	
	if ( (d = opendir(nextpath)) == 0)
	{
		return print_file_error(nextpath);
	}
	
	if ((max_dir_depth - dir_depth) > read_stat(t_ddepth))
		update_stat(t_ddepth, (max_dir_depth - dir_depth));

	while ( (entry = readdir(d)) != NULL)
	{
		/* don't process '.' or '..' in a directory! */
		if ( (strcmp(entry->d_name, THIS_DIR) == 0) 
			|| (strcmp(entry->d_name, PARENT_DIR) == 0))
			continue;

		/* do all files but ones beginning with a dot,
		 * unless we've enabled it! */
		if (entry->d_name[0] == '.')
		{
			if (enabled(READ_DOT_FILES))
			{
				process_entry(list_head, entry->d_name,
					nextpath, dir_depth, 0);
			}
			else
				inc_stat(t_dotfiles, 1);
		}
		else
			process_entry(list_head, entry->d_name, nextpath,
				dir_depth, 0);
	}
	closedir(d);
	wait_for_children(list_head);
	free(nextpath);
	return 0;	
}
/* vi: set autoindent tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
