/*
 * Copyright (c) 2011-2013 Mellanox Technologies LTD. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef _SSA_PLUGIN_H_
#define _SSA_PLUGIN_H_

#include <iba/ib_types.h>
#include <opensm/osm_opensm.h>
#include <ssa_database.h>

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else                           /* !__cplusplus */
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif                          /* __cplusplus */

BEGIN_C_DECLS

#define SSA_PLUGIN_OUTPUT_FILE "ssa_plugin.log"
struct ssa_events {
	osm_log_t *osmlog;
	osm_opensm_t *p_osm;
};

/* TODO: remove after migration with SSA framework */
enum {
	SSA_LOG_DEFAULT		= 1 << 0,
	SSA_LOG_VERBOSE		= 1 << 1,
	SSA_LOG_CTRL		= 1 << 2,
	SSA_LOG_DB		= 1 << 3,
	SSA_LOG_COMM		= 1 << 4,
	SSA_LOG_ALL		= 0xFFFFFFFF,
};

int  ssa_open_log(char *log_file, osm_opensm_t *osm);
void ssa_close_log(void);
void ssa_write_log(int level, const char *format, ...);
#define ssa_log(level, format, ...) \
	ssa_write_log(level, "%s: "format, __func__, ## __VA_ARGS__)

void fprintf_log(FILE *log_file, const char *buffer);
END_C_DECLS
#endif				/* _SSA_PLUGIN_H_ */
