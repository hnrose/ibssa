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

#include <pthread.h>

#include <complib/cl_qmap.h>
#include <complib/cl_passivelock.h>
#include <opensm/osm_config.h>
#include <opensm/osm_version.h>
#include <opensm/osm_opensm.h>
#include <opensm/osm_event_plugin.h>
#include <opensm/osm_log.h>
#include <ssa_database.h>
#include <ssa_plugin.h>
#include <ssa_comparison.h>
#include <ssa_extract.h>
#include <stdarg.h>

struct ssa_database *ssa_db = NULL;
static FILE *flog;
uint8_t first_time_subnet_up = 1;

static const char *month_str[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

const char *port_state_str[] = {
	"No change",
	"Down",
	"Initialize",
	"Armed",
	"Active"
};

/** =========================================================================
 */
void fprintf_log(FILE *log_file, const char *buffer)
{
	pid_t pid = 0;
	time_t tim;
	struct tm result;
	uint64_t time_usecs;
	uint32_t usecs;

	time_usecs = cl_get_time_stamp();
	tim = time_usecs / 1000000;
	usecs = time_usecs % 1000000;
	localtime_r(&tim, &result);
	pid = pthread_self();
	fprintf(log_file,
		"%s %02d %02d:%02d:%02d %06d [%04X] -> %s",
		(result.tm_mon < 12 ? month_str[result.tm_mon] : "???"),
		result.tm_mday, result.tm_hour, result.tm_min,
		result.tm_sec, usecs, pid, buffer);
}

/** =========================================================================
 */
int  ssa_open_log(char *log_file, osm_opensm_t *osm)
{
	char buffer[64];

	sprintf(buffer, "%s/%s",
		osm->subn.opt.dump_files_dir, log_file);
	flog = fopen(buffer, "w");
	if (!(flog)) {
		osm_log(&osm->log, OSM_LOG_ERROR,
			"SSA Plugin: Failed to open output file \"%s\"\n",
			buffer);
		return -1;
	}
	return 0;
}

/** =========================================================================
 */
void ssa_close_log(void)
{
	fclose(flog);
}

/** =========================================================================
 */
void ssa_write_log(int level, const char *format, ...)
{
	va_list args;
	char buffer[256];

	va_start(args, format);
	vsprintf(buffer, format, args);
	fprintf_log(flog, buffer);
	fflush(flog);
	va_end(args);
}

/** =========================================================================
 */
static const char *sm_state_str(int state)
{
	switch (state) {
	case IB_SMINFO_STATE_DISCOVERING:
		return "Discovering";
	case IB_SMINFO_STATE_STANDBY:
		return "Standby";
	case IB_SMINFO_STATE_NOTACTIVE:
		return "Not Active";
	case IB_SMINFO_STATE_MASTER:
		return "Master";
	}
	return "UNKNOWN";
}

/** =========================================================================
 */
static void *construct(osm_opensm_t *osm)
{
	struct ssa_events *ssa = (struct ssa_events *) malloc(sizeof(*ssa));
	if (!ssa)
		return (NULL);

	if (ssa_open_log(SSA_PLUGIN_OUTPUT_FILE, osm) < 0) {
		free(ssa);
		return (NULL);
	}

	ssa_log(SSA_LOG_DEFAULT | SSA_LOG_VERBOSE, "Scalable SA Core - OpenSM Plugin\n");
	ssa_log(SSA_LOG_DEFAULT | SSA_LOG_VERBOSE, "SSA Plugin started\n");

	ssa_db = ssa_database_init();
	if (!ssa_db) {
		ssa_log(SSA_LOG_ALL, "SSA database init failed\n");
		osm_log(&osm->log, OSM_LOG_ERROR,
			"SSA Plugin: SSA database init failed\n");
		ssa_close_log();
		free(ssa);
		return (NULL);
	}

	ssa->osmlog = &osm->log;
	ssa->p_osm = osm;

	ssa_db->p_previous_db = ssa_db_init();
	if (!ssa_db->p_previous_db) {
		ssa_log(SSA_LOG_ALL, "ssa_db_init failed (previous SMDB)\n");
		return (NULL);
	}
	ssa_db->p_current_db = ssa_db_init();
	if (!ssa_db->p_current_db) {
		ssa_log(SSA_LOG_ALL, "ssa_db_init failed (current SMDB)\n");
		ssa_db_delete(ssa_db->p_previous_db);
		return (NULL);
	}
	ssa_db->p_dump_db = ssa_db_init();
	if (!ssa_db->p_dump_db) {
		ssa_log(SSA_LOG_ALL, "ssa_db_init failed (dump SMDB)\n");
		ssa_db_delete(ssa_db->p_previous_db);
		ssa_db_delete(ssa_db->p_current_db);
		return (NULL);
	}
	return ((void *)ssa);
}

/** =========================================================================
 */
static void destroy(void *_ssa)
{
	struct ssa_events *ssa = (struct ssa_events *) _ssa;

	ssa_log(SSA_LOG_DEFAULT | SSA_LOG_VERBOSE, "SSA Plugin stopped\n");
	ssa_database_delete(ssa_db);
	free(ssa);
	ssa_log(SSA_LOG_VERBOSE, "that's all folks!\n");
	ssa_close_log();
}

/** =========================================================================
 */
static void handle_trap_event(struct ssa_events *ssa, ib_mad_notice_attr_t *p_ntc)
{
	if (ib_notice_is_generic(p_ntc)) {
		ssa_log(SSA_LOG_DEFAULT | SSA_LOG_VERBOSE,
			"Generic trap type %d event %d from LID %u\n",
			ib_notice_get_type(p_ntc),
			cl_ntoh16(p_ntc->g_or_v.generic.trap_num),
			cl_ntoh16(p_ntc->issuer_lid));
	} else {
		ssa_log(SSA_LOG_DEFAULT | SSA_LOG_VERBOSE,
			"Vendor trap type %d from LID %u\n",
			ib_notice_get_type(p_ntc),
			cl_ntoh16(p_ntc->issuer_lid));
	}
}

/** =========================================================================
 */
static void report(void *_ssa, osm_epi_event_id_t event_id, void *event_data)
{
	struct ssa_events *ssa = (struct ssa_events *) _ssa;
	struct ssa_db_diff *p_ssa_db_diff;
	struct ep_lft_block_rec *p_lft_block, *p_lft_block_old;
	struct ep_lft_top_tbl_rec *p_lft_top_tbl_rec;
	struct ep_map_rec *p_map_rec, *p_map_rec_tmp;
	osm_epi_lft_change_event_t *p_lft_change;
	uint64_t key;
	uint64_t rec_num;
	uint16_t lid, block_num;

	switch (event_id) {
	case OSM_EVENT_ID_TRAP:
		handle_trap_event(ssa, (ib_mad_notice_attr_t *) event_data);
		break;
	case OSM_EVENT_ID_LFT_CHANGE:
		p_lft_change = (osm_epi_lft_change_event_t *) event_data;
		if (p_lft_change && p_lft_change->p_sw) {
			lid = cl_ntoh16(osm_node_get_base_lid(p_lft_change->p_sw->p_node, 0));
			if (p_lft_change->flags == LFT_CHANGED_BLOCK) {
				block_num = p_lft_change->block_num;
				key = ep_lft_block_rec_gen_key(lid, block_num);
				ssa_log(SSA_LOG_VERBOSE, "LFT change block event received "
							 "for LID %u Block %u\n",
							 lid, block_num);
				p_lft_block = ep_lft_block_rec_init(p_lft_change->p_sw,
								    lid, block_num);
				if (p_lft_block) {
					p_lft_block_old = (struct ep_lft_block_rec *)
								cl_qmap_insert(&ssa_db->p_lft_db->ep_dump_lft_block_tbl,
									       key, &p_lft_block->map_item);
					if (p_lft_block_old != p_lft_block) {
						/* in case of existing record with the same key */
						memcpy(p_lft_block_old->block, p_lft_block->block, IB_SMP_DATA_SIZE);
						ep_lft_block_rec_delete(p_lft_block);
					}
				}
			} else if (p_lft_change->flags == LFT_CHANGED_LFT_TOP) {
				p_lft_top_tbl_rec = (struct ep_lft_top_tbl_rec *) malloc(sizeof(*p_lft_top_tbl_rec));
				if (!p_lft_top_tbl_rec) {
						/* TODO: add memory allocation failure handling */
				}
				rec_num = cl_qmap_count(&ssa_db->p_lft_db->ep_dump_lft_top_tbl);
				if (rec_num % SSA_TABLE_BLOCK_SIZE == 0) {
					ssa_db->p_lft_db->p_dump_lft_top_tbl = (struct ep_lft_top_tbl_rec *)
							realloc(&ssa_db->p_lft_db->p_dump_lft_top_tbl[0],
								(rec_num / SSA_TABLE_BLOCK_SIZE + 1) *
								SSA_TABLE_BLOCK_SIZE *
								sizeof(*ssa_db->p_lft_db->p_dump_lft_top_tbl));
				}

				ssa_log(SSA_LOG_VERBOSE, "LFT change top event received "
							 "for LID %u New Top %u\n",
							 lid, p_lft_change->lft_top);

				ep_lft_top_tbl_rec_init(lid, p_lft_change->lft_top, p_lft_top_tbl_rec);
				key = (uint64_t) lid;
				p_map_rec = ep_map_rec_init(rec_num);
				p_map_rec_tmp = (struct ep_map_rec *)
					cl_qmap_insert(&ssa_db->p_lft_db->ep_dump_lft_top_tbl,
						       key, &p_map_rec->map_item);

				if (p_map_rec != p_map_rec_tmp) {
					/* in case of a record with the same key already exist */
					rec_num = p_map_rec_tmp->offset;
					free(p_map_rec);
				}

				memcpy(&ssa_db->p_lft_db->p_dump_lft_top_tbl[rec_num],
				       p_lft_top_tbl_rec, sizeof(*p_lft_top_tbl_rec));
				free(p_lft_top_tbl_rec);
			} else {
				ssa_log(SSA_LOG_ALL, "Unknown LFT change event (%d)\n", p_lft_change->flags);
				osm_log(ssa->osmlog, OSM_LOG_ERROR,
					"Unknown LFT change event (%d) reported to SSA plugin\n",
					p_lft_change->flags);
			}
		}
		break;
	case OSM_EVENT_ID_SUBNET_UP:
		/* For now, ignore SUBNET UP events when there is subnet init error */
		if (ssa->p_osm->subn.subnet_initialization_error)
			break;

		ssa_log(SSA_LOG_VERBOSE, "Subnet up event\n");

		ssa_db->p_dump_db = ssa_db_extract(ssa);
		/* For verification */
		ssa_db_validate(ssa, ssa_db->p_dump_db);
		ssa_db_validate_lft(ssa);

		/* Updating SMDB versions */
		ssa_db_update(ssa, ssa_db);

		/* Getting SMDB changes from the last dump */
		p_ssa_db_diff =
			ssa_db_compare(ssa, ssa_db);
		if (p_ssa_db_diff) {
			ssa_log(SSA_LOG_VERBOSE, "SMDB was changed. Pushing the changes...\n");
			/* TODO: Here the changes are pushed down through ditribution tree */
			ssa_db_diff_destroy(p_ssa_db_diff);
		}
		first_time_subnet_up = 0;
		break;
	case OSM_EVENT_ID_STATE_CHANGE:
		ssa_log(SSA_LOG_DEFAULT | SSA_LOG_VERBOSE,
			"SM state (%u: %s) change event\n",
			ssa->p_osm->subn.sm_state,
			sm_state_str(ssa->p_osm->subn.sm_state));
		break;
	default:
		/* Ignoring all other events for now... */
		if (event_id >= OSM_EVENT_ID_MAX) {
			ssa_log(SSA_LOG_ALL, "Unknown event (%d)\n", event_id);
			osm_log(ssa->osmlog, OSM_LOG_ERROR,
				"Unknown event (%d) reported to SSA plugin\n",
				event_id);
		}
	}
}

/** =========================================================================
 * Define the object symbol for loading
 */

#if OSM_EVENT_PLUGIN_INTERFACE_VER != 2
#error OpenSM plugin interface version mismatch
#endif

osm_event_plugin_t osm_event_plugin = {
      osm_version:OSM_VERSION,
      create:construct,
      delete:destroy,
      report:report
};
