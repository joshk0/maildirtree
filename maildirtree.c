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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

struct Directory {
  char* name = NULL;
  struct Directory ** subdirs = NULL;
  int last_subdir_index = 0;
};

#if __STDC_VERSION__ < 199901L
typedef enum { false = 0, true } bool;
#endif

static const char indent [] = "   ";
static unsigned int indentlen = 3;

static bool find_subdirs (struct Directory * subdir, char* token);
static struct Directory * hier_sort (char** dirs);
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
      
      print_tree (root, 0);
      
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
      /* remove the leading dot; not used later */
      result[count-1] = strdup ((entries->d_name)+1);
    }
  }

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
static struct Directory * hier_sort (char** dirs)
{
  struct Directory *dir = malloc(sizeof(struct Directory));
  struct Directory *sub = NULL;
  dir->subdirs = NULL;
  
  char *tokens;

  while (*dirs != NULL)
  {
    tokens = strtok(*dirs, "."); /* This must not be null due to read_this_dir
				  * filtering */
    if (dir->subdirs != NULL && (sub = find_subdirs(dir->subdirs, tokens)) != NULL) // XXX O(n)
    {
      
    }
    else /* create the subdir */
    {
      dir->subdirs[++last_subdir_index] = malloc(sizeof(struct Directory));
      dir->subdirs[last_subdir_index]->name = tokens;
    }
    /* DURR */
  }
  
  return dir;
}

static struct Directory * find_subdirs (struct Directory * subdir, char* token)
{
  int i;
  assert(subdir != NULL);

  for (i = 0; i < num_subdirs; i++)
  {
    assert(subdir[i] != NULL);
    if (!strcmp(subdir[i]->name, token))
      return subdir[i];
  }
  return NULL;
}

static void print_tree (struct Directory * start, unsigned int level)
{
  char* in = (level == 0) ? "" : malloc ((level * indentlen) + 1);
  unsigned char bar;
  unsigned int i;
  
  for (i = 0; i <= level; i++)
    strcat (in, indent);
  
  printf ("%s%c %s\n", in, start->name);
  
  while (start->subdirs != NULL) {
    bar = ((start->subdirs)+1 == NULL) ? '`' : '|';
    printf("%s%c %s\n", in, bar, (*(start->subdirs))->name);
    print_tree (*(start->subdirs++), level + 1);
  }
}
