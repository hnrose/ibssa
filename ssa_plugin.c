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

static const char *port_state_str[] = {
	"No change",
	"Down",
	"Initialize",
	"Armed",
	"Active"
};

/** =========================================================================
 */
#define SSA_PLUGIN_OUTPUT_FILE "ssa_plugin.log"
struct ssa_events {
	FILE *log_file;
	osm_log_t *osmlog;
	osm_opensm_t *p_osm;
};

/** =========================================================================
 */
static void fprintf_log(FILE *log_file, const char *buffer)
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
static struct ssa_db *dump_osm_db(struct ssa_events *ssa)
{
	struct ssa_db *p_ssa;
	osm_subn_t *p_subn = &ssa->p_osm->subn;
	osm_node_t *p_node, *p_next_node;
	osm_port_t *p_port, *p_next_port;
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
	const osm_pkey_tbl_t *p_pkey_tbl;
	const ib_pkey_table_t *block;
	char slvl_buffer[128];
	char *header_line =    "#in out : 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15";
	char *separator_line = "#--------------------------------------------------------";
	ib_slvl_table_t *p_tbl;
	ib_net16_t pkey;
	uint16_t lids, block_index, pkey_idx, max_pkeys;
	uint8_t out_port, in_port, num_ports;
	uint8_t i, n;
#else
	uint16_t lids;
#endif
	struct ep_node_rec *p_node_rec;
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec;
	struct ep_port_rec *p_port_rec;
	char buffer[64];
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
	uint8_t is_fdr10_active;
#endif

	lids = (uint16_t) cl_ptr_vector_get_size(&p_subn->port_lid_tbl);
	sprintf(buffer, "dump_osm_db: %u LIDs\n", lids);
	fprintf_log(ssa->log_file, buffer);
	p_ssa = ssa_db_init(lids);		/* Always do this ??? */
	if (!p_ssa) {
		sprintf(buffer, "dump_osm_db: ssa_db_init failed\n");
		fprintf_log(ssa->log_file, buffer);
		goto _exit;
	}

	/* First, Fabric/SM related parameters */
	p_ssa->subnet_prefix = cl_ntoh64(p_subn->opt.subnet_prefix);
	p_ssa->sm_state = p_subn->sm_state;
	p_ssa->lmc = p_subn->opt.lmc;
	p_ssa->subnet_timeout = p_subn->opt.subnet_timeout;
	/* Determine fabric_mtu/rate by looking at switch external ports with peer switch external ports in LinkUp state !!! */
	p_ssa->fabric_mtu = IB_MTU_LEN_4096;
	p_ssa->fabric_rate = IB_PATH_RECORD_RATE_56_GBS;	/* assume 4x FDR for now */
	p_ssa->enable_quirks = (uint8_t) p_subn->opt.enable_quirks;
	p_ssa->allow_both_pkeys = (uint8_t) p_subn->opt.allow_both_pkeys;

	p_next_node = (osm_node_t *)cl_qmap_head(&p_subn->node_guid_tbl);
	while (p_next_node !=
	       (osm_node_t *)cl_qmap_end(&p_subn->node_guid_tbl)) {
		p_node = p_next_node;
		p_next_node = (osm_node_t *)cl_qmap_next(&p_node->map_item);
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
		sprintf(buffer, "Node GUID 0x%" PRIx64 " Type %d%s",
			cl_ntoh64(osm_node_get_node_guid(p_node)),
			osm_node_get_type(p_node),
			(osm_node_get_type(p_node) == IB_NODE_TYPE_SWITCH) ? " " : "\n");
		fprintf_log(ssa->log_file, buffer); 
		if (osm_node_get_type(p_node) == IB_NODE_TYPE_SWITCH)
			fprintf(ssa->log_file, "with %s Switch Port 0\n",
				ib_switch_info_is_enhanced_port0(&p_node->sw->switch_info) ? "Enhanced" : "Base");
#endif

		/* add to node table (ep_node_tbl) */
		p_node_rec = ep_node_rec_init(p_node);
		if (p_node_rec) {
			cl_qmap_insert(&p_ssa->ep_node_tbl,	/* dump_db ??? */
				       osm_node_get_node_guid(p_node),
				       &p_node_rec->map_item);
			/* handle error !!! */

		} else {
			sprintf(buffer, "Node rec memory allocation for "
				"Node GUID 0x%" PRIx64 " failed\n",
				cl_ntoh64(osm_node_get_node_guid(p_node)));
			fprintf_log(ssa->log_file, buffer);
		}
	}

	p_next_port = (osm_port_t *)cl_qmap_head(&p_subn->port_guid_tbl);
	while (p_next_port !=
	       (osm_port_t *)cl_qmap_end(&p_subn->port_guid_tbl)) {
		p_port = p_next_port;
		p_next_port = (osm_port_t *)cl_qmap_next(&p_port->map_item);
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
		sprintf(buffer, "Port GUID 0x%" PRIx64 " LID %u Port state %d (%s)\n",
			cl_ntoh64(osm_physp_get_port_guid(p_port->p_physp)),
			cl_ntoh16(osm_port_get_base_lid(p_port)),
			osm_physp_get_port_state(p_port->p_physp),
			(osm_physp_get_port_state(p_port->p_physp) < 5 ? port_state_str[osm_physp_get_port_state(p_port->p_physp)] : "???"));
		fprintf_log(ssa->log_file, buffer);
		is_fdr10_active =
		    p_port->p_physp->ext_port_info.link_speed_active & FDR10;
		sprintf(buffer, "FDR10 %s active\n",
			is_fdr10_active ? "" : "not");
		fprintf_log(ssa->log_file, buffer);
#endif

#ifdef SSA_PLUGIN_VERBOSE_LOGGING
		sprintf(slvl_buffer, "\t\t\tSLVL tables\n");
		fprintf_log(ssa->log_file, slvl_buffer);
		sprintf(slvl_buffer, "%s\n", header_line);
		fprintf_log(ssa->log_file, slvl_buffer);
		sprintf(slvl_buffer, "%s\n", separator_line);
		fprintf_log(ssa->log_file, slvl_buffer);

		out_port = p_port->p_physp->port_num;
		num_ports = p_port->p_physp->p_node->node_info.num_ports;
		if (osm_node_get_type(p_port->p_physp->p_node) == IB_NODE_TYPE_SWITCH) {
			/* no need to print SL2VL table for port that is down */
			/* TODO:: not sure if it is needed */
			/*if (!p_port->p_physp->p_remote_physp)
				continue; */

			for (in_port = 0; in_port <= num_ports; in_port++) {
				p_tbl = osm_physp_get_slvl_tbl(p_port->p_physp, in_port);
				for (i = 0, n = 0; i < 16; i++)
					n += sprintf(buffer + n, " %-2d",
						ib_slvl_table_get(p_tbl, i));
					sprintf(slvl_buffer, "%-3d %-3d :%s\n", in_port, out_port, buffer);
				fprintf_log(ssa->log_file, slvl_buffer);
			}
		} else {
			p_tbl = osm_physp_get_slvl_tbl(p_port->p_physp, 0);
			for (i = 0, n = 0; i < 16; i++)
				n += sprintf(buffer + n, " %-2d",
						ib_slvl_table_get(p_tbl, i));
				sprintf(slvl_buffer, "%-3d %-3d :%s\n", out_port, out_port, buffer);
			fprintf_log(ssa->log_file, slvl_buffer);
		}

		max_pkeys = cl_ntoh16(p_port->p_node->node_info.partition_cap);
		sprintf(buffer, "PartitionCap %u\n", max_pkeys);
		fprintf_log(ssa->log_file, buffer);
		p_pkey_tbl = osm_physp_get_pkey_tbl(p_port->p_physp);
		sprintf(buffer, "PKey Table %u used blocks\n",
			p_pkey_tbl->used_blocks);
		fprintf_log(ssa->log_file, buffer);
		for (block_index = 0; block_index < p_pkey_tbl->used_blocks;
		     block_index++) {
			block = osm_pkey_tbl_new_block_get(p_pkey_tbl,
							   block_index);
			if (!block)
				continue;
			for (pkey_idx = 0;
			     pkey_idx < IB_NUM_PKEY_ELEMENTS_IN_BLOCK;
			     pkey_idx++) {
				pkey = block->pkey_entry[pkey_idx];
				if (ib_pkey_is_invalid(pkey))
					continue;
				sprintf(buffer, "PKey 0x%04x at block %u index %u\n",
					cl_ntoh16(pkey), block_index, pkey_idx);
				fprintf_log(ssa->log_file, buffer);
			}
		}
#endif

		/* check for valid LID first */
		if ((cl_ntoh16(osm_port_get_base_lid(p_port)) < IB_LID_UCAST_START_HO) ||
		    (cl_ntoh16(osm_port_get_base_lid(p_port)) > IB_LID_UCAST_END_HO)) {
			sprintf(buffer, "Port GUID 0x%" PRIx64
				" has invalid LID %u\n",
				cl_ntoh64(osm_physp_get_port_guid(p_port->p_physp)),
				cl_ntoh16(osm_port_get_base_lid(p_port)));
			fprintf_log(ssa->log_file, buffer);
		}

		/* add to port tables (ep_guid_to_lid_tbl (qmap), ep_port_tbl (vector)) */
		p_guid_to_lid_rec = ep_guid_to_lid_rec_init(p_port);
		if (p_guid_to_lid_rec) {
			cl_qmap_insert(&p_ssa->ep_guid_to_lid_tbl,	/* dump_db ??? */
				       osm_physp_get_port_guid(p_port->p_physp),
				       &p_guid_to_lid_rec->map_item);
			/* handle error !!! */

		} else {
			sprintf(buffer, "Port GUID to LID rec memory allocation "
				" for Port GUID 0x%" PRIx64 " failed\n",
				cl_ntoh64(osm_physp_get_port_guid(p_port->p_physp)));
			fprintf_log(ssa->log_file, buffer);
		}

		p_port_rec = ep_port_rec_init(p_port);
		if (p_port_rec) {
			cl_ptr_vector_set(&p_ssa->ep_port_tbl,	/* dump_db ??? */
					  cl_ntoh16(osm_port_get_base_lid(p_port)),
					  p_port_rec);
			/* handle error !!! */

		} else {
			sprintf(buffer, "Port rec memory allocation for "
				"Port GUID 0x%" PRIx64 " failed\n",
				cl_ntoh64(osm_physp_get_port_guid(p_port->p_physp)));
			fprintf_log(ssa->log_file, buffer);
		}

	}

_exit:
	sprintf(buffer, "Exiting dump_osm_db\n");
	fprintf_log(ssa->log_file, buffer);

	return p_ssa;
}

/** =========================================================================
 */
static void validate_dump_db(struct ssa_events *ssa, struct ssa_db *p_dump_db)
{
	struct ep_node_rec *p_node, *p_next_node;
	struct ep_guid_to_lid_rec *p_port, *p_next_port;
	struct ep_port_rec *p_port_rec;
	const ib_pkey_table_t *block;
	uint16_t lid, block_index, pkey_idx;
	ib_net16_t pkey;
	char buffer[64];

	if (!p_dump_db)
		return;

	sprintf(buffer, "validate_dump_db\n");
	fprintf_log(ssa->log_file, buffer);

	/* First, most Fabric/SM related parameters */
	sprintf(buffer, "Subnet prefix 0x%" PRIx64 "\n", p_dump_db->subnet_prefix);
	fprintf_log(ssa->log_file, buffer);
	sprintf(buffer, "LMC %u Subnet timeout %u Quirks %sabled MTU %d Rate %d Both Pkeys %sabled\n",
		p_dump_db->lmc, p_dump_db->subnet_timeout,
		p_dump_db->enable_quirks ? "en" : "dis",
		p_dump_db->fabric_mtu, p_dump_db->fabric_rate,
		p_dump_db->allow_both_pkeys ? "en" : "dis");
	fprintf_log(ssa->log_file, buffer);

	p_next_node = (struct ep_node_rec *)cl_qmap_head(&p_dump_db->ep_node_tbl);
	while (p_next_node !=
	       (struct ep_node_rec *)cl_qmap_end(&p_dump_db->ep_node_tbl)) {
		p_node = p_next_node;
		p_next_node = (struct ep_node_rec *)cl_qmap_next(&p_node->map_item);
		sprintf(buffer, "Node GUID 0x%" PRIx64 " Type %d%s",
			cl_ntoh64(p_node->node_info.node_guid),
			p_node->node_info.node_type,
			(p_node->node_info.node_type == IB_NODE_TYPE_SWITCH) ? " " : "\n");
		fprintf_log(ssa->log_file, buffer);
		if (p_node->node_info.node_type == IB_NODE_TYPE_SWITCH)
			fprintf(ssa->log_file, "with %s Switch Port 0\n",
				p_node->is_enhanced_sp0 ? "Enhanced" : "Base");
	}

	p_next_port = (struct ep_guid_to_lid_rec *)cl_qmap_head(&p_dump_db->ep_guid_to_lid_tbl);
	while (p_next_port !=
	       (struct ep_guid_to_lid_rec *)cl_qmap_end(&p_dump_db->ep_guid_to_lid_tbl)) {
		p_port = p_next_port;
		p_next_port = (struct ep_guid_to_lid_rec *)cl_qmap_next(&p_port->map_item);
		sprintf(buffer, "Port GUID 0x%" PRIx64 " LID %u LMC %u is_switch %d\n",
			cl_ntoh64(cl_map_key((cl_map_iterator_t) p_port)),
			p_port->lid, p_port->lmc, p_port->is_switch);
		fprintf_log(ssa->log_file, buffer);
	}

	for (lid = 1;
	     lid < (uint16_t) cl_ptr_vector_get_size(&p_dump_db->ep_port_tbl);
	     lid++) {		/* increment LID by LMC ??? */
		p_port_rec = (struct ep_port_rec *) cl_ptr_vector_get(&p_dump_db->ep_port_tbl, lid);
		if (p_port_rec) {
			sprintf(buffer, "Port LID %u LMC %u Port state %d (%s)\n",
				cl_ntoh16(p_port_rec->port_info.base_lid),
				ib_port_info_get_lmc(&p_port_rec->port_info),
				ib_port_info_get_port_state(&p_port_rec->port_info),
				(ib_port_info_get_port_state(&p_port_rec->port_info) < 5 ? port_state_str[ib_port_info_get_port_state(&p_port_rec->port_info)] : "???"));
			fprintf_log(ssa->log_file, buffer);
			sprintf(buffer, "FDR10 %s active\n",
				p_port_rec->is_fdr10_active ? "" : "not");
			fprintf_log(ssa->log_file, buffer);

			/* TODO: add SLVL tables dump */

			sprintf(buffer, "PartitionCap %u\n",
				p_port_rec->ep_pkey_rec.max_pkeys);
			fprintf_log(ssa->log_file, buffer);
			sprintf(buffer, "PKey Table %u used blocks\n",
				p_port_rec->ep_pkey_rec.used_blocks);
			fprintf_log(ssa->log_file, buffer);

			for (block_index = 0;
			     block_index < p_port_rec->ep_pkey_rec.used_blocks;
			     block_index++) {
				block = &p_port_rec->ep_pkey_rec.pkey_tbl[block_index];
				for (pkey_idx = 0;
				     pkey_idx < IB_NUM_PKEY_ELEMENTS_IN_BLOCK;
				     pkey_idx++) {
					pkey = block->pkey_entry[pkey_idx];
					if (ib_pkey_is_invalid(pkey))
						continue;
					sprintf(buffer,
						"PKey 0x%04x at block %u index %u\n",
						cl_ntoh16(pkey), block_index,
						pkey_idx);
					fprintf_log(ssa->log_file, buffer);
				}
			}
		}
	}

	sprintf(buffer, "Exiting validate_dump_db\n");
	fprintf_log(ssa->log_file, buffer);
}

/** =========================================================================
 */
static void remove_dump_db(struct ssa_events *ssa, struct ssa_db *p_dump_db)
{
	struct ep_port_rec *p_port_rec;
	struct ep_guid_to_lid_rec *p_port, *p_next_port;
	struct ep_node_rec *p_node, *p_next_node;
	uint16_t lid;
	char buffer[64];

	if (!p_dump_db)
		return;

	sprintf(buffer, "remove_dump_db\n");
	fprintf_log(ssa->log_file, buffer);

	for (lid = 1;
	     lid < (uint16_t) cl_ptr_vector_get_size(&p_dump_db->ep_port_tbl);
	     lid++) {		/* increment LID by LMC ??? */
		p_port_rec = (struct ep_port_rec *) cl_ptr_vector_get(&p_dump_db->ep_port_tbl, lid);
		ep_port_rec_delete(p_port_rec);
		cl_ptr_vector_set(&p_dump_db->ep_port_tbl, lid, NULL);	/* overkill ??? */
	}

	p_next_port = (struct ep_guid_to_lid_rec *)cl_qmap_head(&p_dump_db->ep_guid_to_lid_tbl);
	while (p_next_port !=
	       (struct ep_guid_to_lid_rec *)cl_qmap_end(&p_dump_db->ep_guid_to_lid_tbl)) {
		p_port = p_next_port;
		p_next_port = (struct ep_guid_to_lid_rec *)cl_qmap_next(&p_port->map_item);
		cl_qmap_remove_item(&p_dump_db->ep_guid_to_lid_tbl,
				    &p_port->map_item);
		ep_guid_to_lid_rec_delete(p_port);
	}

	p_next_node = (struct ep_node_rec *)cl_qmap_head(&p_dump_db->ep_node_tbl);
	while (p_next_node !=
	       (struct ep_node_rec *)cl_qmap_end(&p_dump_db->ep_node_tbl)) {
		p_node = p_next_node;
		p_next_node = (struct ep_node_rec *)cl_qmap_next(&p_node->map_item);
		cl_qmap_remove_item(&p_dump_db->ep_node_tbl,
				    &p_node->map_item);
		ep_node_rec_delete(p_node);
	}

	sprintf(buffer, "Exiting remove_dump_db\n");
	fprintf_log(ssa->log_file, buffer);
}

/** =========================================================================
 */
static void report(void *_ssa, osm_epi_event_id_t event_id, void *event_data)
{
	struct ssa_events *ssa = (struct ssa_events *) _ssa;
	char buffer[48];

	switch (event_id) {
	case OSM_EVENT_ID_TRAP:
		handle_trap_event(ssa, (ib_mad_notice_attr_t *) event_data);
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
