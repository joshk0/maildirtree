/* snprintf.h header for support snprintf() function.
 * (C) 2003 Joshua Kwan <joshk@triplehelix.org> */

#ifndef INCLUDED_snprintf_h

#ifndef HAVE_SNPRINTF
int snprintf (char *str, size_t count, const char *fmt, ...);
#endif

#endif
