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
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

struct ssa_database *ssa_db = NULL;
static FILE *flog;
uint8_t first_time_subnet_up = 1;

pthread_t	work_thread;
int		fd[2];

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
	int rc;

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

	rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (rc) {
		ssa_log(SSA_LOG_ALL, "ERROR %d: error creating socketpair \n", rc);
		ssa_database_delete(ssa_db);
		return NULL;
	}

	rc = pthread_create(&work_thread, NULL, ssa_db_run, (void *) ssa);
	if (rc) {
		ssa_log(SSA_LOG_ALL, "ERROR %d: error creating work thread \n", rc);
		ssa_database_delete(ssa_db);
		close(fd[0]);
		close(fd[1]);
		return NULL;
	}

	return ((void *)ssa);
}

/** =========================================================================
 */
static void destroy(void *_ssa)
{
	struct ssa_events *ssa = (struct ssa_events *) _ssa;
	struct ssa_db_ctrl_msg msg;

	msg.len = sizeof(msg);
	msg.type = SSA_DB_EXIT;
	write(fd[0], (char *) &msg, sizeof(msg));
	pthread_join(work_thread, NULL);

	close(fd[0]);
	close(fd[1]);

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
			ntohs(p_ntc->g_or_v.generic.trap_num),
			ntohs(p_ntc->issuer_lid));
	} else {
		ssa_log(SSA_LOG_DEFAULT | SSA_LOG_VERBOSE,
			"Vendor trap type %d from LID %u\n",
			ib_notice_get_type(p_ntc),
			ntohs(p_ntc->issuer_lid));
	}
}

/** =========================================================================
 */
static void report(void *_ssa, osm_epi_event_id_t event_id, void *event_data)
{
	struct ssa_events *ssa = (struct ssa_events *) _ssa;
	osm_epi_lft_change_event_t *p_lft_change;
	osm_epi_ucast_routing_flags_t *p_ucast_routing_flag;
	struct ssa_db_lft_change_rec *p_lft_change_rec;
	struct ssa_db_ctrl_msg msg;
	size_t size;

	switch (event_id) {
	case OSM_EVENT_ID_TRAP:
		handle_trap_event(ssa, (ib_mad_notice_attr_t *) event_data);
		break;
	case OSM_EVENT_ID_LFT_CHANGE:
		p_lft_change = (osm_epi_lft_change_event_t *) event_data;
		if (p_lft_change && p_lft_change->p_sw) {
			ssa_log(SSA_LOG_VERBOSE, "LFT change event for SW 0x%" PRIx64"\n",
				ntohll(osm_node_get_node_guid(p_lft_change->p_sw->p_node)));

			size = sizeof(*p_lft_change_rec);
			if (p_lft_change->flags == LFT_CHANGED_BLOCK)
				size += sizeof(uint8_t) * IB_SMP_DATA_SIZE;

			p_lft_change_rec = (struct ssa_db_lft_change_rec *) malloc(size);
			if (!p_lft_change_rec) {
				/* TODO: handle failure in memory allocation */
			}

			memcpy(&p_lft_change_rec->lft_change, p_lft_change,
			       sizeof(p_lft_change_rec->lft_change));
			p_lft_change_rec->lid = osm_node_get_base_lid(p_lft_change->p_sw->p_node, 0);

			if (p_lft_change->flags == LFT_CHANGED_BLOCK)
				memcpy(p_lft_change_rec->block, p_lft_change->p_sw->lft +
				       p_lft_change->block_num * IB_SMP_DATA_SIZE,
				       IB_SMP_DATA_SIZE);

			pthread_mutex_lock(&ssa_db->lft_rec_list_lock);
			cl_qlist_insert_tail(&ssa_db->lft_rec_list, &p_lft_change_rec->list_item);
			pthread_mutex_unlock(&ssa_db->lft_rec_list_lock);

			msg.len = sizeof(msg);
			msg.type = SSA_DB_LFT_CHANGE;
			write(fd[0], (char *) &msg, sizeof(msg));
		}
		break;
	case OSM_EVENT_ID_UCAST_ROUTING_DONE:
		p_ucast_routing_flag = (osm_epi_ucast_routing_flags_t *) event_data;
		if (p_ucast_routing_flag &&
		    *p_ucast_routing_flag == UCAST_ROUTING_REROUTE) {
			/* We get here in case of subnet re-routing not followed by SUBNET_UP */
			/* TODO: notify the distribution thread and push the LFT changes */
		}
		break;
	case OSM_EVENT_ID_SUBNET_UP:
		/* For now, ignore SUBNET UP events when there is subnet init error */
		if (ssa->p_osm->subn.subnet_initialization_error)
			break;

		ssa_log(SSA_LOG_VERBOSE, "Subnet up event\n");

		msg.len = sizeof(msg);
		msg.type = SSA_DB_START_EXTRACT;
		write(fd[0], (char *) &msg, sizeof(msg));

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
      .osm_version = OSM_VERSION,
      .create = construct,
      .delete = destroy,
      .report = report
};
