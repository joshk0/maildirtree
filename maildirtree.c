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

#define _GNU_SOURCE /* strdup in C99 */

#include <stdlib.h>
#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

struct Directory {
  char * name;
  struct Directory ** subdirs;
  int count;
};

#if __STDC_VERSION__ < 199901L
typedef enum { false = 0, true } bool;
#endif

typedef enum { FIRST, MIDDLE, LAST } STATE;

static unsigned int indent_len = 3;

static struct Directory * hier_sort (char** dirs, char* dirName);
static char** read_this_dir (DIR* d);
static void print_tree (struct Directory * start, unsigned int level, STATE s);

int main (int argc, char* argv[])
{
  DIR * maildir;
  struct Directory * root;
  char ** dirs;
  
  if (argc >= 2) {
    if ((maildir = opendir(argv[1])) != NULL) {
      dirs = read_this_dir(maildir);

      closedir(maildir);

      root = hier_sort(dirs, basename(argv[1]));
      
      print_tree (root, -1, FIRST);
      
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
      /* remove the leading dot; not used later */
      result[count-1] = strdup ((entries->d_name)+1);
    }
  }

  /* End it */
  result = realloc (result, (++count)*sizeof(char**));
  result[count-1] = NULL;
  
  return result;
}

/* hier_sort()
 * 
 * precondition: dirs points to a listing of directories in a Maildir
 * as created by read_this_dir
 * 
 * side effects: dirs will be WASTED, don't use it again; deep copy it
 * if necessary?
 */
static struct Directory * hier_sort (char** dirs, char* dirName)
{
  /* Initialize an empty root entry. */
  struct Directory *root = malloc(sizeof(struct Directory)), *i = root;

  root->name = strdup(dirName);
  root->count = 0;
  root->subdirs = NULL;
  
  char *test;	
  /* Loop on strtok until it is NULL. */
  
  while (*dirs != NULL)
  {
    test = strtok (*dirs, ".");

    assert (test != NULL);
    do
    {
      bool found = false;
      /* Find it in the 'current' list, unless there are no subdirs */
      if (i->subdirs)
      {
        int x;
        for (x = 0; x < i->count; x++)
        {
          if (!strcmp(test, i->subdirs[x]->name)) /* FOUND IT */
          {
            i = i->subdirs[x];
            found = true;
            break;
          }
        }
        if (!found)
          goto create;
      }
      else /* This is our first */
      {
create:
        i->subdirs = realloc (i->subdirs, sizeof(struct Directory*) * (++(i->count)));
        i->subdirs[i->count - 1] = malloc (sizeof(struct Directory));
        i = i->subdirs[i->count - 1];
  
	i->name = strdup(test);
        i->count = 0;
        i->subdirs = NULL;
      }
    }
    while ((test = strtok (NULL, ".")) != NULL);
    
    dirs++;
    i = root; /* rewind all the way back */
  }

  return root;
}

static void print_tree (struct Directory * start, unsigned int level, STATE s)
{
  int j, l = level;

  if (s != FIRST)
  {
    while (l--)
    {
      int k;
      putchar('|');
      for (k = 0; k < indent_len; k++)
        putchar(' ');
    }
    
    printf("%c- %s\n", (s == LAST) ? '`' : '|', start->name);
  }
  else
    puts(start->name);
  
  for (j = 0; j < start->count; j++)
    print_tree (start->subdirs[j], level + 1,
	    (j == start->count - 1) ? LAST : MIDDLE);
}
