#--
# Copyright (c) 2011-2012 Mellanox Technologies LTD. All rights reserved.
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
#
#--

# Makefile.am -- Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I config

SUBDIRS = . 

#INCLUDES = -I. -I$(OSM_PATH)/include/infiniband
#INCLUDES = -I. -I$(OSM_PATH)/local/include/infiniband
#INCLUDES = -I. -I$(OSM_PATH)/include/infiniband -I$(OSM_PATH)/include
INCLUDES = -I. -I$(OSM_PATH)/local/include/infiniband -I$(OSM_PATH)/local/include

AM_CFLAGS = -Wall -Werror -fPIC -g
#AM_CFLAGS = -Wall -Werror -fPIC -Wextra $(DBGFLAGS)
#AM_CFLAGS = -Wall -Werror -fPIC -g -DSSA_PLUGIN_VERBOSE_LOGGING

LDADD =

ssa_api_version=$(shell \grep LIBVERSION= $(srcdir)/libssa.ver | sed 's/LIBVERSION=//')
libssa_la_LDFLAGS = -version-info $(ssa_api_version) -export-dynamic
libssa_la_SOURCES = $(srcdir)/ssa_database.c $(srcdir)/ssa_plugin.c \
		    $(srcdir)/ssa_comparison.c $(srcdir)/ssa_extract.c \
		    $(srcdir)/ssa_smdb.c
#libssa_la_LIBADD =
lib_LTLIBRARIES = libssa.la

#libssa_la_CFLAGS = 
#libssa_la_LDFLAGS = -static
#libssa_la_SOURCES = $(topsrcdir)/src/ssa_database.c \
#		     $(topsrcdir)/src/ssa_plugin.c \
#		     $(topsrcdir)/src/ssa_comparison.c
