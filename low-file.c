/*
 * Mark Niemeyer
 * CS 720
 * Assignment 4
 * November 29, 2009
 */

/******************************************************************************
 * low-file.c
 * This file implements all of the actual file processing which is done to
 * create a 'list of words.'  Since the file processing is largely independent
 * of the directory traversal and thread creation code, this can be cleanly
 * separated from that code.  Conceptually, we can create any number of
 * file processing programs just by editing this file and linking it into the
 * pre-existing directory traversal/multithreading code.  Hooray!
 ******************************************************************************
 */

#include "low-file.h"
#include "low-stats.h"
#include "low-base.h"

/* ------------------------------------------------------------------
 * Determines whether the character pointed to by c is part of a word.
 * If the character is alphanumeric or an underscore, it is always
 * part of a word.  If the character is an apostrophe or hyphen, it
 * must be allowed by the option flags, and must be surrounded ONLY
 * by letters.  To determine this, a pointer to the current filestream
 * is passed so we can check the surrounding characters using fseek.
 * All other characters are not part of a word.
 * c:		The character being examined.
 * f:		The stream currently being read from.
 * flags:	The usual set of option flags.
 * Returns:	1 if character
 *		0 if not character.
 * ------------------------------------------------------------------
 */
int is_word(params *P, char c, FILE *f)
{
	char prev, next;
	if (isalnum((int) c) || c == '_')
		return 1;
	else if ( (c == APOSTROPHE && enabled(P, APOSTROPHES)) ||
		  (c == HYPHEN && enabled(P, HYPHENS)) )
	{
		if (fseek(f, -2, SEEK_CUR) != -1)
			prev = getc(f);
		else /* error in seeking back -- beginning of file? */
			return 0;

		if (fseek(f,1, SEEK_CUR) != -1)
			next = getc(f);
		else /* error in peeking forward -- end of file? */
			return 0;

		/* reset file position index -- this shouldn't ever fail... */
		if (fseek(f,-1,SEEK_CUR) == -1)
			return 0;
		if (c == APOSTROPHE)
			return isalpha((int)prev) && isalpha((int)next);
		else
			return isalnum((int)prev) && isalnum((int)next);
	}
	return 0;
}


/* ------------------------------------------------------------------
 * Actions to perform when we have read in a character that is not
 * part of a word.  here because read_file was just ghastly.
 * word:	word that just ended
 * wlen:	length of said word
 * filename:	file word was read from
 * words:	number of words found in file
 * a:		'apostrophe found' flag
 * h:		'hyphen found' flag
 * page:	page number word was on
 * line:	line number word was on
 * ------------------------------------------------------------------
 */
void outside_word(params *P, char *word, int wlen, char *filename, 
	int a, int h, int page, int line)
{
	word[wlen] = '\0';
	if (	(wlen >= (P->min_word_length)) && 
			(!(P->max_word_length) || (wlen <= (P->max_word_length))))
	{
		print_word(P, word, filename, page, line);
		if (a && !h)
			inc_stat(P, t_words_a, 1);
		if (h && !a)
			inc_stat(P, t_words_h, 1);
		if (a && h)
			inc_stat(P, t_words_ah, 1);
	}
	else if (wlen && wlen < (P->min_word_length))
		inc_stat(P, t_words_small, 1);
	else if ((P->max_word_length) && (wlen > (P->max_word_length)))
	{
		inc_stat(P, t_words_long, 1);
	}
}

/* ------------------------------------------------------------------
 * Main file reading function.  Opens file (returning error if detected)
 * and reads data one character at a time.  Each character is evaluated
 * to be part of a word or not by the is_word function and printed as
 * appropriate.  When the end of a word is detected, the appropriate
 * filename/page/line information is printed.
 * At the end of input, print_word is called one last time for the -t option.
 * filename:	filename of file to be opened.
 * ------------------------------------------------------------------
 */
int read_file(params *P, char *filename)
{
	int line, page, words, c, word_length, bufsize, new_size;
	int apostrophes, hyphens;
	FILE *f;
	char *current_word;
	char *tmp_word;
	if ((P->max_word_length) > 0)
		bufsize = (P->max_word_length) + 1;
	else
		bufsize = BUFFER_INCREM;
	if ((current_word = malloc(bufsize+1)) == NULL)
	{
        fprintf(stderr, "Malloc failed in read_file.\n");
		return -1;
	}
	current_word[bufsize] = '\0';
	if ((*filename) == DO_STDIN)
		f = stdin;
	else 
	{
		f = fopen(filename, "r");
		if (!f)
		{
			print_file_error(P, 0, filename);
			return -1;
		}
	}
	
	line = 1;
	page = 1;
	words = 0;
	word_length = 0;
	apostrophes = 0;
	hyphens = 0;

	while ( ((c = fgetc(f)) != EOF) && /* while input exists... */
		(!((P->max_words)) || (words < (P->max_words))) ) /* ..and word count OK */
	{
		if (is_word(P, c, f)) /* char is part of a word */
		{
			if ((P->max_word_length) && (word_length > (P->max_word_length)))
				continue;
			if (enabled(P, CASE_INSENSITIVE))
				c = tolower(c);
			current_word[word_length] = c;
			word_length += 1;
			if (c == APOSTROPHE)
				apostrophes = 1;
			else if (c == HYPHEN)
				hyphens = 1;
			if (word_length > bufsize)
			{
				new_size = bufsize + BUFFER_INCREM;
				tmp_word = (char *) realloc(current_word, new_size);
				if (tmp_word == NULL)
				{
					perror("realloc");
					return -1;
				}
				current_word = tmp_word;
				bufsize = new_size;
			}
		}
		else /* we found something which delimits a word */
		{
			outside_word(P, current_word, word_length, filename,
				apostrophes, hyphens, page, line);
			words += 1;
			word_length = 0;
			apostrophes = 0;
			hyphens = 0;

			/* and then some checks for new page/line */
			if (c == NEW_PAGE)
			{
				page += 1;
				if (!enabled(P, OUTPUT_NOPAGE))
					line = 1;
			}
			else if (c == NEW_LINE)
				line += 1;
		}
	};
	outside_word(P, current_word, word_length, filename, 
		apostrophes, hyphens, page, line);
	words += 1;

	/* One last print statement at end of input, in case -t is enabled */
	print_word(P, current_word, filename, 0, words);
	free(current_word);
	inc_stat(P, t_words, words);

	/* Rewind input if stdin (so redirected input works...) */
	if (f == stdin)
		rewind(f);
	else
		fclose(f);
	return 0;
}

