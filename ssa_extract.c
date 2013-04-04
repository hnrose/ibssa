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

#include <opensm/osm_opensm.h>
#include <ssa_database.h>
#include <ssa_plugin.h>
#include <ssa_comparison.h>

extern char *port_state_str[];
extern struct ssa_db *init_ssa_db(struct ssa_events *ssa);

/** =========================================================================
 */
struct ssa_db *ssa_db_extract(struct ssa_events *ssa)
{
	struct ssa_db *p_ssa;
	osm_subn_t *p_subn = &ssa->p_osm->subn;
	osm_node_t *p_node, *p_next_node;
	osm_physp_t *p_physp;
	osm_port_t *p_port, *p_next_port;
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
	const osm_pkey_tbl_t *p_pkey_tbl;
	const ib_pkey_table_t *block;
	char buffer[64];
	char *header_line =    "#in out : 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15";
	char *separator_line = "#--------------------------------------------------------";
	ib_slvl_table_t *p_tbl;
	ib_net16_t pkey;
	uint16_t lids, block_index, pkey_idx, max_pkeys;
	uint8_t out_port, in_port, num_ports;
	uint8_t n;
#else
	uint16_t lids;
#endif
	struct ep_node_rec *p_node_rec;
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec;
	struct ep_port_rec *p_port_rec;
	struct ep_lft_rec *p_lft_cur, *p_lft_next, *p_lft_rec;
	struct ep_link_rec *p_link_rec;
	uint64_t link_rec_key;
	uint8_t i;
	static uint8_t is_first_dump = 1;
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
	uint8_t is_fdr10_active;
#endif

	lids = (uint16_t) cl_ptr_vector_get_size(&p_subn->port_lid_tbl);
	ssa_log(SSA_LOG_VERBOSE, "[ %u LIDs\n", lids);

	p_ssa = ssa_db->p_dump_db;
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
		if (osm_node_get_type(p_node) == IB_NODE_TYPE_SWITCH)
			sprintf(buffer, " with %s Switch Port 0\n",
				ib_switch_info_is_enhanced_port0(&p_node->sw->switch_info) ? "Enhanced" : "Base");
		else
			sprintf(buffer, "\n");
		ssa_log(SSA_LOG_VERBOSE, "Node GUID 0x%" PRIx64 " Type %d%s",
			cl_ntoh64(osm_node_get_node_guid(p_node)),
			osm_node_get_type(p_node),
			buffer);
#endif

		/* add to node table (ep_node_tbl) */
		p_node_rec = ep_node_rec_init(p_node);
		if (p_node_rec) {
			cl_qmap_insert(&p_ssa->ep_node_tbl,	/* dump_db ??? */
				       osm_node_get_node_guid(p_node),
				       &p_node_rec->map_item);
			/* handle error !!! */

		} else {
			ssa_log(SSA_LOG_VERBOSE, "Node rec memory allocation for "
				"Node GUID 0x%" PRIx64 " failed\n",
				cl_ntoh64(osm_node_get_node_guid(p_node)));
		}

		/* 		Adding LFT tables
		 * When the first SMDB dump is performed, all LFTs
		 * are added automatically, further dumps or changes
		 * will be done only on OSM_EVENT_ID_LFT_CHANGE
		 */
		if (osm_node_get_type(p_node) == IB_NODE_TYPE_SWITCH) {
			if (is_first_dump) {
				p_lft_rec = (struct ep_lft_rec *)
						cl_qmap_get(&p_ssa->ep_lft_tbl,
							    cl_ntoh16(osm_node_get_base_lid(p_node, 0)));
				if (p_lft_rec != (struct ep_lft_rec *)
							cl_qmap_end(&p_ssa->ep_lft_tbl)) {
					cl_qmap_remove(&p_ssa->ep_lft_tbl,
						       cl_qmap_key(&p_lft_rec->map_item));
					ep_lft_rec_delete(p_lft_rec);
				}
				p_lft_rec = ep_lft_rec_init(p_node->sw);
				if (p_lft_rec) {
					cl_qmap_insert(&p_ssa->ep_lft_tbl,
						       cl_ntoh16(osm_node_get_base_lid(p_node, 0)),
						       &p_lft_rec->map_item);
					/* handle error !!! */
				}
			}
		}
	}
	is_first_dump = 0;

	p_next_port = (osm_port_t *)cl_qmap_head(&p_subn->port_guid_tbl);
	while (p_next_port !=
	       (osm_port_t *)cl_qmap_end(&p_subn->port_guid_tbl)) {
		p_port = p_next_port;
		p_next_port = (osm_port_t *)cl_qmap_next(&p_port->map_item);
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
		ssa_log(SSA_LOG_VERBOSE, "Port GUID 0x%" PRIx64 " LID %u Port state %d (%s)\n",
			cl_ntoh64(osm_physp_get_port_guid(p_port->p_physp)),
			cl_ntoh16(osm_port_get_base_lid(p_port)),
			osm_physp_get_port_state(p_port->p_physp),
			(osm_physp_get_port_state(p_port->p_physp) < 5 ? port_state_str[osm_physp_get_port_state(p_port->p_physp)] : "???"));
		is_fdr10_active =
		    p_port->p_physp->ext_port_info.link_speed_active & FDR10;
		ssa_log(SSA_LOG_VERBOSE, "FDR10 %s active\n",
			is_fdr10_active ? "" : "not");
#endif

#ifdef SSA_PLUGIN_VERBOSE_LOGGING
		ssa_log(SSA_LOG_VERBOSE, "\t\t\tSLVL tables\n");
		ssa_log(SSA_LOG_VERBOSE, "%s\n", header_line);
		ssa_log(SSA_LOG_VERBOSE, "%s\n", separator_line);

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
					ssa_log(SSA_LOG_VERBOSE, "%-3d %-3d :%s\n", in_port, out_port, buffer);
			}
		} else {
			p_tbl = osm_physp_get_slvl_tbl(p_port->p_physp, 0);
			for (i = 0, n = 0; i < 16; i++)
				n += sprintf(buffer + n, " %-2d",
						ib_slvl_table_get(p_tbl, i));
				ssa_log(SSA_LOG_VERBOSE, "%-3d %-3d :%s\n", out_port, out_port, buffer);
		}

		max_pkeys = cl_ntoh16(p_port->p_node->node_info.partition_cap);
		ssa_log(SSA_LOG_VERBOSE, "PartitionCap %u\n", max_pkeys);
		p_pkey_tbl = osm_physp_get_pkey_tbl(p_port->p_physp);
		ssa_log(SSA_LOG_VERBOSE, "PKey Table %u used blocks\n",
			p_pkey_tbl->used_blocks);
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
				ssa_log(SSA_LOG_VERBOSE, "PKey 0x%04x at block %u index %u\n",
					cl_ntoh16(pkey), block_index, pkey_idx);
			}
		}
#endif

		/* check for valid LID first */
		if ((cl_ntoh16(osm_port_get_base_lid(p_port)) < IB_LID_UCAST_START_HO) ||
		    (cl_ntoh16(osm_port_get_base_lid(p_port)) > IB_LID_UCAST_END_HO)) {
			ssa_log(SSA_LOG_VERBOSE, "Port GUID 0x%" PRIx64
				" has invalid LID %u\n",
				cl_ntoh64(osm_physp_get_port_guid(p_port->p_physp)),
				cl_ntoh16(osm_port_get_base_lid(p_port)));
		}

		/* add to port tables (ep_guid_to_lid_tbl (qmap), ep_port_tbl (vector)) */
		p_guid_to_lid_rec = ep_guid_to_lid_rec_init(p_port);
		if (p_guid_to_lid_rec) {
			cl_qmap_insert(&p_ssa->ep_guid_to_lid_tbl,	/* dump_db ??? */
				       osm_physp_get_port_guid(p_port->p_physp),
				       &p_guid_to_lid_rec->map_item);
			/* handle error !!! */

		} else {
			ssa_log(SSA_LOG_VERBOSE, "Port GUID to LID rec memory allocation "
				" for Port GUID 0x%" PRIx64 " failed\n",
				cl_ntoh64(osm_physp_get_port_guid(p_port->p_physp)));
		}

		p_port_rec = ep_port_rec_init(p_port);
		if (p_port_rec) {
			cl_ptr_vector_set(&p_ssa->ep_port_tbl,	/* dump_db ??? */
					  cl_ntoh16(osm_port_get_base_lid(p_port)),
					  p_port_rec);
			/* handle error !!! */

		} else {
			ssa_log(SSA_LOG_VERBOSE, "Port rec memory allocation for "
				"Port GUID 0x%" PRIx64 " failed\n",
				cl_ntoh64(osm_physp_get_port_guid(p_port->p_physp)));
		}

		/* TODO:: adding all physical port objects to the port map - by LID and port_num or GUID ??? */

		/* add all physical ports to link table (ep_link_tbl) */
		/* TODO:: add log info ??? */
		p_node = p_port->p_physp->p_node;
		if (osm_node_get_type(p_node) ==
					IB_NODE_TYPE_SWITCH) {
			for (i = 0; i < p_node->physp_tbl_size; i++) {
				p_physp = osm_node_get_physp_ptr(p_node, i);
				if (!p_physp)
					continue;

				p_link_rec = ep_link_rec_init(p_physp);
				if (p_link_rec) {
					link_rec_key = ep_link_rec_gen_key(
							cl_ntoh16(osm_node_get_base_lid(p_node, 0)),
							osm_physp_get_port_num(p_physp));
					cl_qmap_insert(&p_ssa->ep_link_tbl,
						       link_rec_key,
						       &p_link_rec->map_item);
				}
			}
		} else {
			p_physp = p_port->p_physp;
			p_link_rec = ep_link_rec_init(p_physp);
			if (p_link_rec) {
				link_rec_key = ep_link_rec_gen_key(
						cl_ntoh16(osm_physp_get_base_lid(p_physp)),
						osm_physp_get_port_num(p_physp));
				cl_qmap_insert(&p_ssa->ep_link_tbl,
					       link_rec_key,
					       &p_link_rec->map_item);
			}
		}
	}

	/* remove all LFT records for switches that doesn't exist */
	/* HACK: for removed switch whose LID was immediately
	 * assigned to another port, its LFT record won't be deleted
	 */
	p_lft_next = (struct ep_lft_rec *) cl_qmap_head(&p_ssa->ep_lft_tbl);
	while (p_lft_next !=
	       (struct ep_lft_rec *) cl_qmap_end(&p_ssa->ep_lft_tbl)) {
		p_lft_cur = p_lft_next;
		p_lft_next = (struct ep_lft_rec *) cl_qmap_next(&p_lft_cur->map_item);
		p_port_rec = (struct ep_port_rec *)
				cl_ptr_vector_get(&p_ssa->ep_port_tbl,
					(uint16_t) cl_qmap_key(&p_lft_cur->map_item));
		if (!p_port_rec) {
			cl_qmap_remove_item(&p_ssa->ep_lft_tbl,
					    &p_lft_cur->map_item);
			ep_lft_rec_delete(p_lft_cur);
		}
	}

	p_ssa->initialized = 1;

	ssa_log(SSA_LOG_VERBOSE, "]\n");

	return p_ssa;
}

/** =========================================================================
 */
void ssa_db_validate(struct ssa_events *ssa, struct ssa_db *p_ssa_db)
{
	struct ep_node_rec *p_node, *p_next_node;
	struct ep_guid_to_lid_rec *p_port, *p_next_port;
	struct ep_port_rec *p_port_rec;
	struct ep_lft_rec *p_lft, *p_next_lft;
	struct ep_link_rec *p_link, *p_next_link;
	const ib_pkey_table_t *block;
	char buffer[64];
	uint16_t lid, block_index, pkey_idx;
	ib_net16_t pkey;

	if (!p_ssa_db || !p_ssa_db->initialized)
		return;

	ssa_log(SSA_LOG_VERBOSE, "[\n");

	/* First, most Fabric/SM related parameters */
	ssa_log(SSA_LOG_VERBOSE, "Subnet prefix 0x%" PRIx64 "\n", p_ssa_db->subnet_prefix);
	ssa_log(SSA_LOG_VERBOSE, "LMC %u Subnet timeout %u Quirks %sabled MTU %d Rate %d Both Pkeys %sabled\n",
		p_ssa_db->lmc, p_ssa_db->subnet_timeout,
		p_ssa_db->enable_quirks ? "en" : "dis",
		p_ssa_db->fabric_mtu, p_ssa_db->fabric_rate,
		p_ssa_db->allow_both_pkeys ? "en" : "dis");

	p_next_node = (struct ep_node_rec *)cl_qmap_head(&p_ssa_db->ep_node_tbl);
	while (p_next_node !=
	       (struct ep_node_rec *)cl_qmap_end(&p_ssa_db->ep_node_tbl)) {
		p_node = p_next_node;
		p_next_node = (struct ep_node_rec *)cl_qmap_next(&p_node->map_item);
		if (p_node->node_info.node_type == IB_NODE_TYPE_SWITCH)
			sprintf(buffer, " with %s Switch Port 0\n", p_node->is_enhanced_sp0 ? "Enhanced" : "Base");
		else
			sprintf(buffer, "\n");
		ssa_log(SSA_LOG_VERBOSE, "Node GUID 0x%" PRIx64 " Type %d%s",
			cl_ntoh64(p_node->node_info.node_guid),
			p_node->node_info.node_type,
			buffer);
	}

	p_next_port = (struct ep_guid_to_lid_rec *)cl_qmap_head(&p_ssa_db->ep_guid_to_lid_tbl);
	while (p_next_port !=
	       (struct ep_guid_to_lid_rec *)cl_qmap_end(&p_ssa_db->ep_guid_to_lid_tbl)) {
		p_port = p_next_port;
		p_next_port = (struct ep_guid_to_lid_rec *)cl_qmap_next(&p_port->map_item);
		ssa_log(SSA_LOG_VERBOSE, "Port GUID 0x%" PRIx64 " LID %u LMC %u is_switch %d\n",
			cl_ntoh64(cl_map_key((cl_map_iterator_t) p_port)),
			p_port->lid, p_port->lmc, p_port->is_switch);
	}

	for (lid = 1;
	     lid < (uint16_t) cl_ptr_vector_get_size(&p_ssa_db->ep_port_tbl);
	     lid++) {		/* increment LID by LMC ??? */
		p_port_rec = (struct ep_port_rec *) cl_ptr_vector_get(&p_ssa_db->ep_port_tbl, lid);
		if (p_port_rec) {
			ssa_log(SSA_LOG_VERBOSE, "Port LID %u LMC %u Port state %d (%s)\n",
				cl_ntoh16(p_port_rec->port_info.base_lid),
				ib_port_info_get_lmc(&p_port_rec->port_info),
				ib_port_info_get_port_state(&p_port_rec->port_info),
				(ib_port_info_get_port_state(&p_port_rec->port_info) < 5 ? port_state_str[ib_port_info_get_port_state(&p_port_rec->port_info)] : "???"));
			ssa_log(SSA_LOG_VERBOSE, "FDR10 %s active\n",
				p_port_rec->is_fdr10_active ? "" : "not");

			/* TODO: add SLVL tables dump */

			ssa_log(SSA_LOG_VERBOSE, "PartitionCap %u\n",
				p_port_rec->ep_pkey_rec.max_pkeys);
			ssa_log(SSA_LOG_VERBOSE, "PKey Table %u used blocks\n",
				p_port_rec->ep_pkey_rec.used_blocks);

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
					ssa_log(SSA_LOG_VERBOSE,
						"PKey 0x%04x at block %u index %u\n",
						cl_ntoh16(pkey), block_index,
						pkey_idx);
				}
			}
		}
	}

	p_next_lft = (struct ep_lft_rec *)cl_qmap_head(&p_ssa_db->ep_lft_tbl);
	while (p_next_lft !=
	       (struct ep_lft_rec *)cl_qmap_end(&p_ssa_db->ep_lft_tbl)) {
		p_lft = p_next_lft;
		p_next_lft = (struct ep_lft_rec *)cl_qmap_next(&p_lft->map_item);
		ssa_log(SSA_LOG_VERBOSE, "LFT Record: LID %u lft size %u maximum LID reachable %u\n",
			(uint16_t) cl_qmap_key(&p_lft->map_item), p_lft->lft_size, p_lft->max_lid_ho);
	}

	p_next_link = (struct ep_link_rec *)cl_qmap_head(&p_ssa_db->ep_link_tbl);
	while (p_next_link !=
	       (struct ep_link_rec *)cl_qmap_end(&p_ssa_db->ep_link_tbl)) {
		p_link = p_next_link;
		p_next_link = (struct ep_link_rec *)cl_qmap_next(&p_link->map_item);
		ssa_log(SSA_LOG_VERBOSE, "Link Record: from LID %u port %u to LID %u port %u\n",
			cl_ntoh16(p_link->link_rec.from_lid), p_link->link_rec.from_port_num,
			cl_ntoh16(p_link->link_rec.to_lid), p_link->link_rec.to_port_num);
	}

	ssa_log(SSA_LOG_VERBOSE, "]\n");
}

/** =========================================================================
 */
void ssa_db_remove(struct ssa_events *ssa, struct ssa_db *p_ssa_db)
{
	struct ep_port_rec *p_port_rec;
	struct ep_guid_to_lid_rec *p_port, *p_next_port;
	struct ep_node_rec *p_node, *p_next_node;
	struct ep_link_rec *p_link, *p_next_link;
	uint16_t lid;

	if (!p_ssa_db || !p_ssa_db->initialized)
		return;

	ssa_log(SSA_LOG_VERBOSE, "[\n");

	for (lid = 1;
	     lid < (uint16_t) cl_ptr_vector_get_size(&p_ssa_db->ep_port_tbl);
	     lid++) {		/* increment LID by LMC ??? */
		p_port_rec = (struct ep_port_rec *) cl_ptr_vector_get(&p_ssa_db->ep_port_tbl, lid);
		ep_port_rec_delete(p_port_rec);
		cl_ptr_vector_set(&p_ssa_db->ep_port_tbl, lid, NULL);	/* overkill ??? */
	}

	p_next_port = (struct ep_guid_to_lid_rec *)cl_qmap_head(&p_ssa_db->ep_guid_to_lid_tbl);
	while (p_next_port !=
	       (struct ep_guid_to_lid_rec *)cl_qmap_end(&p_ssa_db->ep_guid_to_lid_tbl)) {
		p_port = p_next_port;
		p_next_port = (struct ep_guid_to_lid_rec *)cl_qmap_next(&p_port->map_item);
		cl_qmap_remove_item(&p_ssa_db->ep_guid_to_lid_tbl,
				    &p_port->map_item);
		ep_guid_to_lid_rec_delete(p_port);
	}

	p_next_node = (struct ep_node_rec *)cl_qmap_head(&p_ssa_db->ep_node_tbl);
	while (p_next_node !=
	       (struct ep_node_rec *)cl_qmap_end(&p_ssa_db->ep_node_tbl)) {
		p_node = p_next_node;
		p_next_node = (struct ep_node_rec *)cl_qmap_next(&p_node->map_item);
		cl_qmap_remove_item(&p_ssa_db->ep_node_tbl,
				    &p_node->map_item);
		ep_node_rec_delete(p_node);
	}

	p_next_link = (struct ep_link_rec *) cl_qmap_head(&p_ssa_db->ep_link_tbl);
	while (p_next_link !=
	       (struct ep_link_rec *) cl_qmap_end(&p_ssa_db->ep_link_tbl)) {
		p_link = p_next_link;
		p_next_link = (struct ep_link_rec *) cl_qmap_next(&p_link->map_item);
		cl_qmap_remove_item(&p_ssa_db->ep_link_tbl,
				    &p_link->map_item);
		ep_link_rec_delete(p_link);
	}

	if (cl_qmap_head(&p_ssa_db->ep_lft_tbl) == cl_qmap_end(&p_ssa_db->ep_lft_tbl))
		p_ssa_db->initialized = 0;
	ssa_log(SSA_LOG_VERBOSE, "]\n");
}

/** =========================================================================
 */
/* TODO:: Add meaningfull return value */
void ssa_db_update(IN struct ssa_events *ssa,
		   IN struct ssa_database *ssa_db)
{
	struct ssa_db *ssa_db_tmp;

	ssa_log(SSA_LOG_VERBOSE, "[\n");

        if (!ssa_db || !ssa_db->p_previous_db || !ssa_db->p_current_db) {
                /* error handling */
                return;
        }

	/* Updating previous SMDB with current one */
	if (ssa_db->p_current_db->initialized) {
		ssa_db_tmp = ssa_db->p_current_db;
		ssa_db->p_current_db = init_ssa_db(ssa);
		if (ssa_db->p_previous_db->initialized)
			ep_lft_qmap_copy(&ssa_db->p_current_db->ep_lft_tbl,
					 &ssa_db->p_previous_db->ep_lft_tbl);
		/* TODO:: merge ssa_db_remove and ssa_db_delete methods */
		ssa_db_remove(ssa, ssa_db->p_previous_db);
		ssa_db_delete(ssa_db->p_previous_db);
		ssa_db->p_previous_db = ssa_db_tmp;
	}
	ssa_db_copy(ssa_db->p_current_db, ssa_db->p_dump_db);

	ssa_log(SSA_LOG_VERBOSE, "]\n");
}
