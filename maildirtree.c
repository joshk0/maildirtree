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

#include "maildirtree.h"

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

static void insert_tree (struct Directory *, char*, unsigned int, unsigned int);
static struct Directory * read_this_dir (DIR*, char*);
static void print_tree (struct Directory *, unsigned int);
static unsigned int count_messages (DIR *);

int main (int argc, char* argv[])
{
  DIR * maildir;
  struct Directory * root;
  
  if (argc >= 2) {
    if ((maildir = opendir(argv[1])) != NULL) {
      root = read_this_dir(maildir, argv[1]);
    
      closedir(maildir);
      
      printf("%s (%u/%u)\n", root->name, root->unread, root->read+root->unread);
      print_tree (root, -1);
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

static struct Directory * read_this_dir (DIR* d, char* rootpath)
{
  DIR *curdir, *newdir;
  struct dirent *entries;
  struct Directory *root = malloc(sizeof(struct Directory));  
  char *cur, *new;
  unsigned int count = 0;

  root->name = strdup(basename(rootpath));
      
  asprintf (&cur, "%s/cur", rootpath);
  asprintf (&new, "%s/new", rootpath);

  curdir = opendir(cur);
  newdir = opendir(new);

  root->read = (curdir != NULL) ? count_messages(curdir) : 0;
  root->unread = (newdir != NULL) ? count_messages(newdir) : 0;

  if (curdir == NULL || newdir == NULL)
    fprintf(stderr, "WARNING: %s does not look like a complete Maildir\n", rootpath);

  root->count = 0;
  root->subdirs = NULL;
  root->last = true;
  root->parent = NULL;
 
  closedir(curdir);
  closedir(newdir);

  count = 0;
  
  while ((entries = readdir(d)) != NULL)
  {
    if (*entries->d_name == '.' &&
        strcmp(entries->d_name, ".") &&
        strcmp(entries->d_name, ".."))
    {
      asprintf (&cur, "%s/%s/cur", rootpath, entries->d_name);
      asprintf (&new, "%s/%s/new", rootpath, entries->d_name);

      curdir = opendir(cur);
      newdir = opendir(new);
      
      insert_tree(root, (entries->d_name)+1,
              (curdir != NULL) ? count_messages(curdir) : 0,
	      (newdir != NULL) ? count_messages(newdir) : 0);

      if (curdir) closedir(curdir);
      if (newdir) closedir(newdir);
      
      free(cur);
      free(new);
    }
  }

  return root;
}

/* insert_tree()
 * 
 * precondition: dirs points to a listing of directories in a Maildir
 * as created by read_this_dir, and root is allocated already
 */
static void insert_tree (struct Directory * root, char* dirName, unsigned int read, unsigned int unread)
{
  struct Directory *i = root;
  char *test;	
  
  /* Loop on strtok until it is NULL. */
  test = strtok (dirName, ".");

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
      i->subdirs[i->count - 1]->parent = i;
      
      /* There was a 'last' one before this */
      if (i->count > 1)
	i->subdirs[i->count - 2]->last = false;

      i = i->subdirs[i->count - 1];
      i->name = strdup(test);
      i->count = 0;
      i->read = read;
      i->unread = unread;
      i->subdirs = NULL;
      i->last = true;
    }
  }
  while ((test = strtok (NULL, ".")) != NULL);
}

static void print_tree (struct Directory * start, unsigned int level)
{
  int j, k, l;
  struct Directory *it = start;
  
  for (l = level; l > 0; l--)
  {
    it->parent->kid = it;
    it = it->parent;
  }
  
  for (l = level; l > 0; l--)
  {
    assert (it != NULL);
      
    if (!it->last)
      putchar('|');
    else
      putchar(' ');
    
    it = it->kid;
    
    for (k = 0; k < INDENT_LEN; k++)
      putchar(' ');
  }
  
  if (start->parent != NULL) /* Not the start entry */
    printf("%c-- %s (%u/%u)\n", start->last ? '`' : '|', start->name, start->unread, start->read + start->unread);
  
  for (j = 0; j < start->count; j++)
    print_tree (start->subdirs[j], level + 1);
}

/* precondition: dir must have been opendir'd */
static unsigned int count_messages (DIR *dir)
{
  unsigned int r = 0;
  struct dirent * tmp;

  while ((tmp = readdir(dir)) != NULL)
  {
    if (strcmp(tmp->d_name, ".") &&
        strcmp(tmp->d_name, ".."))
    {
      r++;
    } 
  }

  return r;
}
