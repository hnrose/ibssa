#--
# Copyright (c) 2012 Mellanox Technologies LTD. All rights reserved.
#
# This software is available to you under the OpenIB.org BSD license below:
#
#     Redistribution and use in source and binary forms, with or
#     without modification, are permitted provided that the following
#     conditions are met:
#
#      - Redistributions of source code must retain the above
#        copyright notice, this list of conditions and the following
#        disclaimer.
#
#      - Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials
#        provided with the distribution.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#--

dnl Process this file with autoconf to produce a configure script.

AC_INIT([ssa],[1.0])
AC_CONFIG_SRCDIR([])
AC_CONFIG_MACRO_DIR(config)

dnl Define an input config option to control osm
AC_ARG_WITH(osm,
[  --with-osm=<dir> define OSM library directory],
AC_MSG_NOTICE(Using OSM from: $with_osm),
with_osm="none")
OSM_PATH=/usr
if test "x$with_osm" != xnone; then
  OSM_PATH=$with_osm
fi
AC_SUBST(OSM_PATH)

dnl use local config dir for extras
AC_CONFIG_AUX_DIR(config)

dnl Defines the Language - we need gcc linker ...
#AC_LANG_CPLUSPLUS
AC_LANG_C

dnl we use auto cheader
AM_CONFIG_HEADER(config.h)

dnl Auto make
AM_INIT_AUTOMAKE

dnl Provides control over re-making of all auto files
dnl We also use it to define swig dependencies so end
dnl users do not see them.
AM_MAINTAINER_MODE

dnl Required for cases make defines a MAKE=make ??? Why
AC_PROG_MAKE_SET

dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_CC
AC_PROG_CXX

dnl We will use libtool for making ...
AC_PROG_LIBTOOL

dnl For making dist we need not check any lib dependency
dnl 
dnl AC_ARG_ENABLE(libcheck,
dnl [  --enable-libcheck    Enable Dependency Checking],
dnl [case "${enableval}" in
dnl   yes) libcheck=true ;;
dnl   no)  libcheck=false ;;
dnl   *) AC_MSG_ERROR(bad value ${enableval} for --enable-libcheck) ;; esac],[libcheck=true])

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/types.h stdio.h stdlib.h string.h pthread.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

dnl Checks for library functions.
AC_CHECK_FUNCS([strchr strstr strtol strtoull regcomp regexec])

AC_CONFIG_FILES([ssa.spec])

dnl Create the following Makefiles
AC_OUTPUT(Makefile)

