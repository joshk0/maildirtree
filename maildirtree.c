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

#include "config.h"

#include "maildirtree.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Special headers that aren't always available. */

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

static void insert_tree (struct Directory *, char*, unsigned int, unsigned int);
static void process (char*, char*);
static struct Directory * read_this_dir (DIR*, char*, int*, int*, int*);
static void print_tree (struct Directory *, int);
static unsigned int count_messages (DIR *);
static void clean (struct Directory * root);
static inline void restore_stderr(void);

static char usage [] =
"Maildirtree " PACKAGE_VERSION " by Joshua Kwan <joshk@triplehelix.org>\n\
Syntax: maildirtree [opts] maildir [maildir...] \n\
Options:\n"
#ifdef HAVE_GETOPT_LONG
"  -h, --help\tDisplay this help message.\n\
  -s, --summary\tOnly print total counts of read and unread messages\n\
  -n, --nocolor\tDo not highlight folders that contain unread messages in white\n\
  -q, --quiet\tDo not print warning messages at all. (Same as 2>/dev/null)";
#else
"  -h\tDisplay this help message.\n\
  -s\tOnly print total counts of read and unread messages\n\
  -n\tDo not highlight folders that contain unread messages in white\n\
  -q\tDo not print warning messages at all. (Same as 2>/dev/null)";
#endif

int stderrfd;
bool summary = false, nocolor = false;

int main (int argc, char* argv[])
{
  int opt;
  char cd [PATH_MAX];
#ifdef HAVE_GETOPT_LONG
  struct option longopts [] = {
          { "help"   , 0, 0, 'h' },
          { "summary", 0, 0, 's' },
          { "nocolor", 0, 0, 'n' },
          { "quiet"  , 0, 0, 'q' },
          { 0, 0, 0, 0 },
  };
#endif
  
  /* If stdout != tty, disable colors */
  if (!isatty(1))
    nocolor = true;

  /* Save stderr unconditionally */
  stderrfd = dup(2);

  atexit (&restore_stderr);

#ifdef HAVE_GETOPT_LONG
  while ((opt = getopt_long (argc, argv, "hsnq", longopts, NULL)) != -1)
#else
  while ((opt = getopt (argc, argv, "hsnq")) != -1)
#endif
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
        puts(usage);
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
      printf("maildirtree: can't get working directory: %s", strerror(errno));
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
      /* First we print the root entry manually... */
	    
      /* indentation of the unread message count, and printf basically returns
       * strlen(fake) + 1 (or strlen(root->name) if that's the case) */
      p = COUNT_START - printf("%s ", fake ? fake : root->name);
      while (p > 0) { putchar(' '); p--; }
    
      /* Unread message count */
      printf ("%s(%u/%u)%s\n",
         (root->unread > 0 && !nocolor) ? "\033[1m" : "",
         root->unread, root->read + root->unread,
         (!nocolor) ? "\033[0m" : "");
      
      /* Print the rest of the children */
      print_tree (root, -1);
     
      if (total_unread > 0)
      {
        printf ("\n%d message%c unread in %d folder%c, %d messages total.\n",
             total_unread, 
             (total_unread > 1) ? 's' : 0,
             folders_unread,
             (folders_unread > 1) ? 's' : 0,
             total_read + total_unread);
      }
      else
      {
        printf ("\n%d messages unread, %d messages total.\n",
             total_unread, total_read + total_unread);
      }
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

/* read_this_dir: reads a directory, and recurses into all folders below
 * 'd'.
 *
 * FIXME: Should be called many times. Many many times. Increase this
 * recursion and make it real clean.
 */

static struct Directory * read_this_dir (DIR* d, char* rootpath, int* fu, int* tr, int* tu)
{
  DIR *curdir, *newdir;
  struct dirent *entries;
  struct Directory *root = (struct Directory *)malloc(sizeof(struct Directory));
  struct stat isdir;
  char *cur_path, *new_path, *stattmp;
  int count = 0, len;
  size_t rlen;
  unsigned int r, u;

  root->name = strdup(basename(rootpath));
  
  /* Used later, save a call to strlen */
  rlen = strlen(rootpath);

  cur_path = (char *)malloc(rlen + 5);
  new_path = (char *)malloc(rlen + 5);
  
  snprintf (cur_path, rlen + 5, "%s/cur", rootpath);
  snprintf (new_path, rlen + 5, "%s/new", rootpath);

  curdir = opendir(cur_path);
  newdir = opendir(new_path);

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

  free (cur_path);
  free (new_path);

  count = 0;
  
  while ((entries = readdir(d)) != NULL)
  {
    len = rlen + strlen(entries->d_name) + 6;

    stattmp = (char*) malloc(len - 4);
    snprintf (stattmp, len - 4, "%s/%s", rootpath, entries->d_name);
    stat (stattmp, &isdir);
    free (stattmp);
    
    if (S_ISDIR(isdir.st_mode) &&
        strcmp(entries->d_name, ".") &&
        strcmp(entries->d_name, "..") &&
	strcmp(entries->d_name, "cur") &&
	strcmp(entries->d_name, "new") &&
	strcmp(entries->d_name, "tmp"))
    {
      cur_path = (char*)malloc(len);
      new_path = (char*)malloc(len);
      
      snprintf (cur_path, len, "%s/%s/cur", rootpath, entries->d_name);
      snprintf (new_path, len, "%s/%s/new", rootpath, entries->d_name);

      curdir = opendir(cur_path);
      newdir = opendir(new_path);

      if (!curdir || !newdir)
      {
        fprintf(stderr, "WARNING: %s is missing cur or new; ignoring!\n", entries->d_name);
        if (curdir) closedir(curdir);
        if (newdir) closedir(newdir);

        free(cur_path);
        free(new_path);

        continue;
      }
     
      /* Assign to r and u the unread message counts for *THIS* folder, 
       * add it to the totals. */
      *tr += (r = (curdir != NULL) ? count_messages(curdir) : 0);
      *tu += (u = (newdir != NULL) ? count_messages(newdir) : 0);
      
      if (u > 0) (*fu)++;
      
      insert_tree(root, entries->d_name, r, u);

      if (curdir) closedir(curdir);
      if (newdir) closedir(newdir);
      
      free(cur_path);
      free(new_path);
    }
  }

  return root;
}

/* insert_tree()
 * 
 * precondition: dirs points to a listing of directories in a Maildir
 * as created by read_this_dir, and root is allocated already
 *
 * FIXME: detection for .Foo.Bar where .Foo does not exist; set a flag
 * to not print message count which is 0/0
 */
static void insert_tree (struct Directory * root, char* dirName, unsigned int read, unsigned int unread)
{
  bool found;
  struct Directory *i = root;
  char *test;        

  /* Ignore the first null token of dirName if it's leading by a dot. */
  if (*dirName == '.')
    dirName++;
  
  /* Loop on strtok until it is NULL. */
  test = strtok (dirName, ".");

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
      i->subdirs = (struct Directory **) realloc (i->subdirs, sizeof(struct Directory*) * i->count);
      i->subdirs[i->count - 1] = (struct Directory *)malloc (sizeof(struct Directory));
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

static void print_tree (struct Directory * start, int level)
{
  int j, k, l;
  struct Directory *it = start;
  
  /* This is a little messy but works. Basically we need to look from the
   * root down to see whether we should print additional pipes for those
   * levels, not from the bottom up as our data structure mandates. 
   *
   * EXPECT KID TO GET CLOBBERED! DO NOT RELY ON IT FOR MULTIPLE
   * CALLS OF THIS FUNCTION! CAVEAT EMPTOR! ACHTUNG! ATTENTION!
   */
  
  for (l = level; l > 0; l--)
  {
    it->parent->kid = it;
    it = it->parent;
  }
  
  for (l = level; l > 0; l--)
  {
    assert (it != NULL);
    putchar ( !it->last ? '|' : ' ' );

    /* Move down into our temporary top-down hierarchy */
    it = it->kid;
    
    for (j = 0; j < INDENT_LEN; j++)
      putchar(' ');
  }
  
  if (start->parent != NULL) /* Not the start entry */
  {
    /* We've already printed INDENT_LEN + 1 number of spaces,
     * the tree 'graphic' + the name; offset the COUNT_START by this
     * to align correctly. */
	  
    k = COUNT_START - (level * (INDENT_LEN + 1)) -
            printf("%c-- %s ", start->last ? '`' : '|', start->name);
    
    /* Actually print the spaces. */
    while (k > 0)
    {
      putchar(' ');
      k--;
    }
    
    /* Unread/total message count */
    printf ("%s(%u/%u)%s\n", 
        (start->unread > 0 && !nocolor) ? "\033[1m" : "",
        start->unread, start->read + start->unread,
        (!nocolor) ? "\033[0m" : "");
  }
  
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

  assert (root->count == -1);
  
  free(root->subdirs);
  free(root->name);
  free(root);
}

static inline void restore_stderr(void)
{
  dup2(stderrfd, 2);
}
