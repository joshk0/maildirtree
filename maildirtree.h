/* maildirtree.h: see maildirtree.c for full copyright. */

#ifndef INCLUDED_maildirtree_h
#define INCLUDED_maildirtree_h

#define _GNU_SOURCE /* strdup in C99 */
#define INDENT_LEN 3

struct Directory {
  char * name;
  struct Directory ** subdirs;
  int count;
  int messages;
};

#if __STDC_VERSION__ < 199901L
typedef enum { false = 0, true } bool;
#endif

typedef enum { FIRST, MIDDLE, LAST } STATE;

#endif /* !INCLUDED_maildirtree_h */
