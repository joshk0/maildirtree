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
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>

static void insert_tree (struct Directory *, char*, unsigned int, unsigned int);
static void process (char*, char*);
static struct Directory * read_this_dir (DIR*, char*, int*, int*, int*);
static void print_tree (struct Directory *, unsigned int, bool);
static unsigned int count_messages (DIR *);
static void clean (struct Directory * root);
static inline void restore_stderr(void);

static char usage [] =
"Maildirtree 0.1 by Joshua Kwan <joshk@triplehelix.org>\n\
Syntax: maildirtree [opts] maildir [maildir...] \n\
Options:\n\
  -h --help\tDisplay this help message.\n\
  -s --summary\tOnly print total counts of read and unread messages\n\
  -n --nocolor\tDo not highlight folders that contain unread messages in white\n\
  -q --quiet\tDo not print warning messages at all. (Same as 2>/dev/null)";

int stderrfd;
bool summary = false, nocolor = false;

int main (int argc, char* argv[])
{
  int opt;
  char cd [PATH_MAX];
  struct option longopts [] = {
	  { "help"   , 0, 0, 'h' },
	  { "summary", 0, 0, 's' },
	  { "nocolor", 0, 0, 'n' },
	  { "quiet"  , 0, 0, 'q' },
	  { 0, 0, 0, 0 },
  };
	
  /* Save stderr unconditionally */
  stderrfd = dup(2);

  atexit (&restore_stderr);

  while ((opt = getopt_long (argc, argv, "hsnq", longopts, NULL)) != -1)
  {
    switch (opt)
    {
      case 'h':
        puts(usage);
	exit(0);

      case 's':
	summary = true;
	break;

      case 'n':
	nocolor = true;
	break;

      case 'q':
	dup2(open("/dev/null", O_WRONLY), 2);
	break;

      case '?':
	return 1;
    }
  }

  if (optind >= argc)
  {
    /* Make sure we get no false positive */
    errno = 0;
    
    getcwd (cd, PATH_MAX);
    
    if (errno != 0)
    {
      printf("maildirtree: could not get working directory: %s", strerror(errno));
      return 1;
    }
    
    process(".", basename(cd));
    
    return 0;
  }

  while (optind < argc) 
  {
    process(argv[optind++], 0);
    puts("");
  }
    
  return 0;
}

static void process (char* dir, char* fake)
{
  DIR * maildir;
  struct Directory * root = NULL;
  int p, folders_unread = 0, total_read = 0, total_unread = 0;
  
  if ((maildir = opendir(dir)) != NULL)
  {
    root = read_this_dir(maildir, dir, &folders_unread, &total_read, &total_unread);
    closedir(maildir);
            
    if (!summary)
    {
      p = COUNT_START - printf("%s ", fake ? fake : root->name);
      while (p > 0) { putchar(' '); p--; }
    
      printf ("%s(%u/%u)%s\n",
        (root->unread > 0 && !nocolor) ? "\033[1m" : "",
         root->unread, root->read + root->unread,
         (!nocolor) ? "\033[0m" : "");
	
      print_tree (root, -1, nocolor);
     
      if (total_unread > 0)
        printf ("\n%d message%c unread in %d folder%c, %d messages total.\n",
             total_unread, 
             (total_unread > 1) ? 's' : 0,
             folders_unread,
             (folders_unread > 1) ? 's' : 0,
             total_read + total_unread);
      else
        printf ("\n%d messages unread, %d messages total.\n",
             total_unread, total_read + total_unread);
    }
		
    else
    {
      if (total_unread > 0)
        printf("%s: %d message%c unread in %d folder%c, %d messages total.\n",
             dir, total_unread,
             (total_unread > 1) ? 's' : 0,
             folders_unread,
             (folders_unread > 1) ? 's' : 0,
             total_read + total_unread);
      else
        printf ("%s: %d messages unread, %d messages total.\n",
             dir, total_unread, total_read + total_unread);
    }
      
    clean(root);
    root = NULL;
    folders_unread = total_read = total_unread = 0;
  }
  else
  {
    printf ("maildirtree: %s: %s\n", dir, strerror(errno));
    exit (1);
  }
}

static struct Directory * read_this_dir (DIR* d, char* rootpath, int* fu, int* tr, int* tu)
{
  DIR *curdir, *newdir;
  struct dirent *entries;
  struct Directory *root = malloc(sizeof(struct Directory));  
  struct stat isdir;
  char *cur, *new, *stattmp;
  int count = 0, len;
  size_t rlen;
  unsigned int r, u;

  root->name = strdup(basename(rootpath));
  
  /* Used later, save a call to strlen */
  rlen = strlen(rootpath);

  cur = malloc(rlen + 5);
  new = malloc(rlen + 5);
  
  snprintf (cur, rlen + 5, "%s/cur", rootpath);
  snprintf (new, rlen + 5, "%s/new", rootpath);

  curdir = opendir(cur);
  newdir = opendir(new);

  root->read   = (curdir != NULL) ? count_messages(curdir) : 0;
  root->unread = (newdir != NULL) ? count_messages(newdir) : 0;

  *tr += root->read;
  *tu += root->unread;
  
  if (root->unread > 0) (*fu)++;

  if (!curdir || !newdir) /* Are we SURE this is a Maildir? */
    fprintf(stderr, "WARNING: %s does not look like a complete Maildir\n", rootpath);

  root->count   = 0;
  root->subdirs = NULL;
  root->last    = true;
  root->parent  = NULL;
 
  closedir (curdir);
  closedir (newdir);

  free (cur);
  free (new);

  count = 0;
  
  while ((entries = readdir(d)) != NULL)
  {
    len = rlen + strlen(entries->d_name) + 6;

    stattmp = malloc (len - 4);
    snprintf (stattmp, len - 4, "%s/%s", rootpath, entries->d_name);
    stat (stattmp, &isdir);
    free (stattmp);
    
    if (S_ISDIR(isdir.st_mode) &&
	*entries->d_name == '.' &&
        strcmp(entries->d_name, ".") &&
        strcmp(entries->d_name, ".."))
    {
      cur = malloc(len);
      new = malloc(len);
      
      snprintf (cur, len, "%s/%s/cur", rootpath, entries->d_name);
      snprintf (new, len, "%s/%s/new", rootpath, entries->d_name);

      curdir = opendir(cur);
      newdir = opendir(new);

      if (!curdir || !newdir)
      {
        fprintf(stderr, "WARNING: %s is missing cur or new; ignoring!\n", entries->d_name);
	if (curdir) closedir(curdir);
	if (newdir) closedir(newdir);

	free(cur);
	free(new);

	continue;
      }
      
      *tr += (r = (curdir != NULL) ? count_messages(curdir) : 0);
      *tu += (u = (newdir != NULL) ? count_messages(newdir) : 0);
      
      if (u > 0) (*fu)++;
      
      insert_tree(root, (entries->d_name) + 1, r, u);

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
  bool found;
  struct Directory *i = root;
  char *test;	
  
  /* Loop on strtok until it is NULL. */
  if ((test = strtok (dirName, ".")) == NULL)
  {
    fprintf(stderr, "WARNING: skipping bad subfolder name '%s'\n", dirName);
    return;
  }

  do
  {
    found = false;

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
      if (!found) /* Additional recursion impossible, must create */
        goto create;
    }
    else /* This is our first */
    {
create:
      i->count++;
      i->subdirs = realloc (i->subdirs, sizeof(struct Directory*) * i->count);
      i->subdirs[i->count - 1] = malloc (sizeof(struct Directory));
      i->subdirs[i->count - 1]->parent = i;
      
      /* There was a 'last' one before this */
      if (i->count > 1)
	i->subdirs[i->count - 2]->last = false;

      i = i->subdirs[i->count - 1];
      i->name = strdup(test);
      i->count = 0;
      
      /* This will be set for real later if this actually exists,
       * but prevents junk if this is not the case (i.e. .Foo.Bar where
       * .Foo is non-existent. */
      
      i->read = 0;
      i->unread = 0;
      
      i->subdirs = NULL;
      i->last = true;
    }
  }
  while ((test = strtok (NULL, ".")) != NULL);

  /* This is only valid on the innermost node */
  i->read = read;
  i->unread = unread;
}

static void print_tree (struct Directory * start, unsigned int level, bool nc)
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
    
    for (j = 0; j < INDENT_LEN; j++)
      putchar(' ');
  }
  
  if (start->parent != NULL) /* Not the start entry */
  {
    k = COUNT_START - (level * (INDENT_LEN + 1)) -
	    printf("%c-- %s ", start->last ? '`' : '|', start->name);
    
    while (k > 0)
    {
      putchar(' ');
      k--;
    }
    
    printf ("%s(%u/%u)%s\n", 
	(start->unread > 0 && !nc) ? "\033[1m" : "",
	start->unread, start->read + start->unread,
	(!nc) ? "\033[0m" : "");
  }
  
  for (j = 0; j < start->count; j++)
    print_tree (start->subdirs[j], level + 1, nc);
}

/* precondition: dir must have been opendir'd */
static unsigned int count_messages (DIR *dir)
{
  unsigned int r = 0;
  struct dirent * tmp;

  while ((tmp = readdir(dir)) != NULL)
  {
    if (*tmp->d_name != '.') /* assuming that dotfiles != messages */
      r++;
  }

  return r;
}

static void clean (struct Directory * root)
{
  root->count--;

  while (root->count >= 0)
  {
    clean(root->subdirs[root->count]);
    root->count--;
  }

  free(root->subdirs);
  free(root->name);
  free(root);
}

static inline void restore_stderr(void)
{
  dup2(stderrfd, 2);
}
