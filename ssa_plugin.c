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
#include <opensm/osm_log.h>
#include <ssa_database.h>
#include <ssa_plugin.h>
#include <ssa_comparison.h>

struct ssa_database *ssa_db = NULL;

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
struct ssa_db *init_ssa_db(struct ssa_events *ssa)
{
	osm_subn_t *p_subn = &ssa->p_osm->subn;
	struct ssa_db *p_ssa;
	char buffer[64];
	uint16_t lids = (uint16_t)
			cl_ptr_vector_get_size(&p_subn->port_lid_tbl);
	p_ssa = ssa_db_init(lids);
	if (!p_ssa) {
		sprintf(buffer, "init_ssa_db: ssa_db_init failed\n");
		fprintf_log(ssa->log_file, buffer);
	}

	return p_ssa;
}

/** =========================================================================
 */
static void *construct(osm_opensm_t *osm)
{
	char buffer[64];

	struct ssa_events *ssa = (struct ssa_events *) malloc(sizeof(*ssa));
	if (!ssa)
		return (NULL);

	sprintf(buffer, "%s/%s",
		osm->subn.opt.dump_files_dir, SSA_PLUGIN_OUTPUT_FILE);
	ssa->log_file = fopen(buffer, "a+");
	if (!(ssa->log_file)) {
		osm_log(&osm->log, OSM_LOG_ERROR,
			"SSA Plugin: Failed to open output file \"%s\"\n",
			buffer);
		free(ssa);
		return (NULL);
	}
	fprintf_log(ssa->log_file, "SSA Plugin started\n");

	ssa_db = ssa_database_init();
	if (!ssa_db) {
		fprintf_log(ssa->log_file, "SSA database init failed\n");
		osm_log(&osm->log, OSM_LOG_ERROR,
			"SSA Plugin: SSA database init failed\n");
		fclose(ssa->log_file);
		free(ssa);
		return (NULL);
	}

	ssa->osmlog = &osm->log;
	ssa->p_osm = osm;

	ssa_db->p_previous_db = init_ssa_db(ssa);
	ssa_db->p_current_db = init_ssa_db(ssa);
	ssa_db->p_dump_db = init_ssa_db(ssa);
	return ((void *)ssa);
}

/** =========================================================================
 */
static void destroy(void *_ssa)
{
	struct ssa_events *ssa = (struct ssa_events *) _ssa;

	fprintf_log(ssa->log_file, "SSA Plugin stopped\n");
	ssa_database_delete(ssa_db);
	fclose(ssa->log_file);
	free(ssa);
}

/** =========================================================================
 */
static void handle_trap_event(struct ssa_events *ssa, ib_mad_notice_attr_t *p_ntc)
{
	char buffer[128];

	if (ib_notice_is_generic(p_ntc)) {
		sprintf(buffer,
			"Generic trap type %d event %d from LID %u\n",
			ib_notice_get_type(p_ntc),
			cl_ntoh16(p_ntc->g_or_v.generic.trap_num),
			cl_ntoh16(p_ntc->issuer_lid));
	} else {
		sprintf(buffer,
			"Vendor trap type %d from LID %u\n",
			ib_notice_get_type(p_ntc),
			cl_ntoh16(p_ntc->issuer_lid));
	}
	fprintf_log(ssa->log_file, buffer);
}

/** =========================================================================
 */
static void report(void *_ssa, osm_epi_event_id_t event_id, void *event_data)
{
	struct ssa_events *ssa = (struct ssa_events *) _ssa;
	struct ssa_db_diff *p_ssa_db_diff;
	struct ep_lft_rec *p_lft_rec, *p_lft_rec_old;
	osm_switch_t *p_sw;
	uint16_t lid_ho;
	char buffer[48];

	switch (event_id) {
	case OSM_EVENT_ID_TRAP:
		handle_trap_event(ssa, (ib_mad_notice_attr_t *) event_data);
		break;
	case OSM_EVENT_ID_LFT_CHANGE:
		p_sw = (osm_switch_t *) event_data;
		if (p_sw) {
			lid_ho = cl_ntoh16(osm_node_get_base_lid(p_sw->p_node, 0));
			sprintf(buffer, "LFT change event received for LID %u\n",
				lid_ho);
			fprintf_log(ssa->log_file, buffer);
			p_lft_rec = ep_lft_rec_init(p_sw);
			if (p_lft_rec) {
				p_lft_rec_old = (struct ep_lft_rec *)
							cl_qmap_get(&ssa_db->p_dump_db->ep_lft_tbl,
								    (uint64_t) lid_ho);
				if (p_lft_rec_old != (struct ep_lft_rec *)
							cl_qmap_end(&ssa_db->p_dump_db->ep_lft_tbl))
					/* in case of existing record that was modified */
					cl_qmap_remove(&ssa_db->p_dump_db->ep_lft_tbl,
						       (uint64_t) lid_ho);
				cl_qmap_insert(&ssa_db->p_dump_db->ep_lft_tbl,
					       lid_ho, &p_lft_rec->map_item);
				ssa_db->p_dump_db->initialized = 1;
			}
		}
		break;
	case OSM_EVENT_ID_SUBNET_UP:
		/* For now, ignore SUBNET UP events when there is subnet init error */
		if (ssa->p_osm->subn.subnet_initialization_error)
			break;

		fprintf_log(ssa->log_file, "Subnet up event\n");

if (ssa_db->p_dump_db)
fprintf_log(ssa->log_file, "First removing existing SSA dump db\n");
		remove_dump_db(ssa, ssa_db->p_dump_db);
fprintf_log(ssa->log_file, "Now dumping OSM db\n");
		ssa_db->p_dump_db = dump_osm_db(ssa);
		/* For verification */
		validate_dump_db(ssa, ssa_db->p_dump_db);

		/* Updating SMDB versions */
		update_ssa_db(ssa, ssa_db);

		/* Getting SMDB changes from the last dump */
		p_ssa_db_diff =
			ssa_db_compare(ssa, ssa_db);
		if (p_ssa_db_diff) {
			sprintf(buffer, "SMDB was changed. Pushing the changes...\n");
			fprintf_log(ssa->log_file, buffer);
			/* TODO: Here the changes are pushed down through ditribution tree */
			ssa_db_diff_destroy(p_ssa_db_diff);
		}

		break;
	case OSM_EVENT_ID_STATE_CHANGE:
		sprintf(buffer, "SM state (%u: %s) change event\n",
			ssa->p_osm->subn.sm_state,
			sm_state_str(ssa->p_osm->subn.sm_state));
		fprintf_log(ssa->log_file, buffer);
		break;
	default:
		/* Ignoring all other events for now... */
		if (event_id >= OSM_EVENT_ID_MAX) {
			sprintf(buffer, "Unknown event (%d)\n", event_id);
			fprintf_log(ssa->log_file, buffer);
			osm_log(ssa->osmlog, OSM_LOG_ERROR,
				"Unknown event (%d) reported to SSA plugin\n",
				event_id);
		}
	}
	fflush(ssa->log_file);
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
