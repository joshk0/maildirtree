/* maildirtree: print out a hierarchy of Courier-style Maildirs in
 * tree format.
 * 
 * (C) 2003 by Joshua Kwan
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

struct Directory {
	char* name;
	struct Directory ** subdirs;
	char** contents;
}

static char indent [] = "   ";
static unsigned int indentlen = 3;

static Directory * hier_sort (char** dirs);
static char** read_this_dir (DIR* d);
static void print_tree (struct Directory * start, unsigned int level);

int main (int argc, char* argv[])
{
	DIR * maildir;
	struct Directory * root;
	char ** dirs;
	
	if (argc >= 2) {
		if ((maildir = opendir(argv[1])) != NULL) {
			dirs = read_this_dir(maildir);

			/* debug */
			while (dirs != NULL && *dirs != NULL) {
				puts (*dirs);
				dirs++;
			}
			
			closedir(maildir);

			root = hier_sort(dirs);
			
			print_tree (root);
			
		}
		else {
			printf ("%s: %s: %s\n", argv[0], argv[1],
					strerror(errno));
			exit (1);
		}
	}
	else {
		printf ("%s: must have a directory argument\n", argv[0]);
		exit (1);
	}

	return 0;
}

static char** read_this_dir (DIR* d)
{
	struct dirent *entries;
	char ** result = NULL;
	int count = 0;
	
	while ((entries = readdir(d)) != NULL) {
		if (*entries->d_name == '.' &&
		    strcmp(entries->d_name, ".") &&
		    strcmp(entries->d_name, ".."))
		{
			result = realloc (result, (++count)*sizeof(char**));
			result[count-1] = malloc (entries->d_reclen + 1);
			result[count-1] = strdup (entries->d_name);
		}
	}

	return result;
}

static Directory * hier_sort (char** dirs)
{
	/* XXX */
	return NULL;
}

static void print_tree (struct Directory * start, unsigned int level)
{
	char* in = malloc ((level * indentlen) + 3);
	unsigned char bar;
	unsigned int i;
	
	for (i = 0; i <= level; i++)
		strcat (in, indent);
	
	while (start->subdirs != NULL) {
		bar = ((start->subdirs)+1 == NULL) ? '`' : '|';
		printf("%s%c %s\n", indent, bar, *(start->subdirs)->name);
		print_tree (start->subdirs++, level + 1);
	}
	
	while (start->contents != NULL) {
		bar = ((start->contents)+1 == NULL) ? '`' : '|';
		printf ("%s%c %s\n", indent, bar, *(start->contents++));
	}
}
