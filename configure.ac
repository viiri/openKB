#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.56])
AC_INIT(openkb, 0.0.1, https://sourceforge.net/p/openkb/tickets/)

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([SDL], [SDL_Init])
AC_CHECK_LIB([SDL_image], [IMG_Load])

# Checks for header files.
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
#AC_TYPE_UINT16_T
#AC_TYPE_UINT32_T
#AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([isascii mkdir strcasecmp strlcpy strlcat])

#AC_CONFIG_FILES([Makefile
#                 src/tools/Makefile
#                 ])
#AC_CONFIG_SUBDIRS([vendor/scale2x-2.4])
AC_OUTPUT
