/* maildirtree.h: see maildirtree.c for full copyright. */

#ifndef INCLUDED_maildirtree_h
#define INCLUDED_maildirtree_h

#define _GNU_SOURCE /* strdup in C99 */
#define INDENT_LEN 3

#if __STDC_VERSION__ < 199901L
typedef enum { false = 0, true } bool;
#endif

struct Directory {
  char * name;
  struct Directory ** subdirs;
  int count;
  unsigned int unread;
  unsigned int read;
  struct Directory * parent;
  struct Directory * kid;
  bool last;
};

#endif /* !INCLUDED_maildirtree_h */
