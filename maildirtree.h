/* maildirtree.h: see maildirtree.c for full copyright. 
 * (C) 2003 by Joshua Kwan. */

#ifndef INCLUDED_maildirtree_h
#define INCLUDED_maildirtree_h

#if !defined(__cplusplus) && __STDC_VERSION__ < 199901L
typedef enum { false = 0, true } bool;
#endif

struct Directory
{
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
