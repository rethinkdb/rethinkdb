#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([rethinkdb], [0.1], [aditya@rethinkdb.com], [rethinkdb], [http://www.rethinkdb.com])
AM_INIT_AUTOMAKE([1.10 no-define foreign])

AM_CONDITIONAL(CHK_DEBUG, [test "$DEBUG" = 1])
AM_CONDITIONAL(CHK_VALGRIND, [test "$VALGRIND" = 1])
# Checks for C++ compiler.
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h netinet/in.h stdint.h stdlib.h string.h strings.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero ftruncate memchr memmove memset pow socket strtoul strtoull])

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT
