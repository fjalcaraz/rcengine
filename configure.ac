AC_PREREQ([2.69])
AC_INIT([rcengine], [1.0], [fjalcaraz@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC(g++)
AC_PROG_YACC(yacc, bison)
AM_PROG_AR(ar)

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memmove memset strdup strerror strtol])

LT_INIT
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_FILES([ Makefile src/Makefile tests/tester/Makefile ])
AC_OUTPUT
