AC_INIT([maildirtree], [0.1])
AC_CONFIG_HEADER(config.h)

AC_PROG_INSTALL

AC_PROG_CC
AC_C_INLINE

if test "$ac_cv_c_compiler_gnu" = yes; then
  CFLAGS="-Wall -W $CFLAGS"
fi

AC_CHECK_FUNCS([getopt_long snprintf])
AC_CHECK_HEADERS([getopt.h libgen.h])

AC_ARG_WITH(indent-len,
	[AC_HELP_STRING([--with-indent-len], [Aesthetic: chars to offset successive levels by (default 3)])],
	[cf_indent_len=$withval], [cf_indent_len=3])

AC_DEFINE_UNQUOTED([INDENT_LEN], $cf_indent_len, [Amount of chars successive levels are offset])

AC_ARG_WITH(count-start,
	[AC_HELP_STRING([--with-count-start], [Aesthetic: chars to offset message counts by (default 40)])],
	[cf_count_start=$withval], [cf_count_start=40])

AC_DEFINE_UNQUOTED([COUNT_START], $cf_count_start, [Amount of chars to offset message count text])

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
