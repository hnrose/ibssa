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
extern struct ssa_database *ssa_db;
extern uint8_t first_time_subnet_up;

/** =========================================================================
 */
struct ssa_db *ssa_db_extract(struct ssa_events *ssa)
{
	struct ssa_db *p_ssa;
	osm_subn_t *p_subn = &ssa->p_osm->subn;
	osm_node_t *p_node, *p_next_node;
	osm_physp_t *p_physp;
	osm_port_t *p_port, *p_next_port;
	const osm_pkey_tbl_t *p_pkey_tbl;
	cl_map_iterator_t pkey_map_iter;
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
	const ib_pkey_table_t *block;
	char buffer[64];
	char *header_line =    "#in out : 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15";
	char *separator_line = "#--------------------------------------------------------";
	ib_slvl_table_t *p_tbl;
	ib_net16_t pkey;
	uint16_t block_index, pkey_idx, max_pkeys;
	uint8_t out_port, in_port, num_ports;
	uint8_t n;
#endif
	struct ep_map_rec *p_map_rec;
	struct ep_guid_to_lid_tbl_rec *p_guid_to_lid_tbl_rec;
	struct ep_node_tbl_rec *p_node_tbl_rec;
	struct ep_port_tbl_rec *p_port_tbl_rec;
	struct ep_link_tbl_rec *p_link_tbl_rec;
	struct ep_lft_block_rec *p_lft_block_rec;
	struct ep_lft_top_tbl_rec *p_lft_top_tbl_rec;
	uint64_t ep_rec_key;
	uint64_t guid_to_lid_offset = 0;
	uint64_t node_offset = 0;
	uint64_t link_offset = 0;
	uint64_t port_offset = 0;
	uint64_t pkey_base_offset = 0;
	uint64_t pkey_cur_offset = 0;
	uint64_t lft_top_offset = 0;
	uint64_t links, ports, pkeys;
	uint32_t guids, nodes;
	uint32_t switch_ports_num = 0, port_pkeys_num = 0;
	uint16_t lft_tops;
	uint16_t lids, lid_ho, max_block;
	uint16_t i;
	uint16_t *p_pkey;
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
	p_ssa->allow_both_pkeys = (uint8_t) p_subn->opt.allow_both_pkeys;

	nodes = (uint32_t) cl_qmap_count(&p_subn->node_guid_tbl);
	if (!p_ssa->p_node_tbl) {
		p_ssa->p_node_tbl = (struct ep_node_tbl_rec *)
				malloc(sizeof(*p_ssa->p_node_tbl) * nodes);
		if (!p_ssa->p_node_tbl) {
			/* add memory allocation failure handling */
			ssa_log(SSA_LOG_VERBOSE, "NODE rec memory allocation failed");
		}
	}

	p_node_tbl_rec = (struct ep_node_tbl_rec *) malloc(sizeof(*p_node_tbl_rec));
	if (!p_node_tbl_rec) {
			/* TODO: add memory allocation failure handling */
	}

	lft_tops = (uint32_t) cl_qmap_count(&p_subn->sw_guid_tbl);
	if (!ssa_db->p_lft_db->p_db_lft_top_tbl) {
		ssa_db->p_lft_db->p_db_lft_top_tbl = (struct ep_lft_top_tbl_rec *)
			malloc(sizeof(*ssa_db->p_lft_db->p_db_lft_top_tbl) * lft_tops);
		if (!ssa_db->p_lft_db->p_db_lft_top_tbl) {
			/* add memory allocation failure handling */
			ssa_log(SSA_LOG_VERBOSE, "LFT top rec memory allocation failed");
		}
	}

	p_lft_top_tbl_rec = (struct ep_lft_top_tbl_rec *) malloc(sizeof(*p_lft_top_tbl_rec));
	if (!p_lft_top_tbl_rec) {
			/* TODO: add memory allocation failure handling */
	}

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
		/* add to node table (p_node_tbl) */
		ep_node_tbl_rec_init(p_node, p_node_tbl_rec);
		memcpy(&p_ssa->p_node_tbl[node_offset], p_node_tbl_rec,
		       sizeof(*p_node_tbl_rec));
		p_map_rec = ep_map_rec_init(node_offset);
		if (!p_map_rec) {
			/* add memory allocation failure handling */
			ssa_log(SSA_LOG_VERBOSE, "Quick MAP rec memory allocation failed");
		}
		node_offset++;
		cl_qmap_insert(&p_ssa->ep_node_tbl,
			       osm_node_get_node_guid(p_node),
			       &p_map_rec->map_item);

		if (osm_node_get_type(p_node) == IB_NODE_TYPE_SWITCH)
			switch_ports_num += p_node->sw->num_ports;

		/* TODO: add more cases when full dump is needed */
		if (!first_time_subnet_up)
			continue;

		ep_lft_block_rec_qmap_clear(&ssa_db->p_lft_db->ep_db_lft_block_tbl);
		ep_qmap_clear(&ssa_db->p_lft_db->ep_db_lft_top_tbl);

		/*		Adding LFT tables
		 * When the first SMDB dump is performed, all LFTs
		 * are added automatically, further dumps or changes
		 * will be done only on OSM_EVENT_ID_LFT_CHANGE
		 */
		if (osm_node_get_type(p_node) == IB_NODE_TYPE_SWITCH) {
			max_block = p_node->sw->lft_size / IB_SMP_DATA_SIZE;
			lid_ho = cl_ntoh16(osm_node_get_base_lid(p_node, 0));
			ep_rec_key = (uint64_t) lid_ho;

			ep_lft_top_tbl_rec_init(lid_ho, p_node->sw->lft_size, p_lft_top_tbl_rec);
			memcpy(&ssa_db->p_lft_db->p_db_lft_top_tbl[lft_top_offset],
			       p_lft_top_tbl_rec, sizeof(*p_lft_top_tbl_rec));
			p_map_rec = ep_map_rec_init(lft_top_offset++);
			cl_qmap_insert(&ssa_db->p_lft_db->ep_db_lft_top_tbl,
				       ep_rec_key, &p_map_rec->map_item);

			for(i = 0; i < max_block; i++) {
				p_lft_block_rec = ep_lft_block_rec_init(p_node->sw,
									lid_ho, i);
				if (!p_lft_block_rec) {
					/* add handling memory allocation failure */
				}
				ep_rec_key = ep_lft_block_rec_gen_key(lid_ho, i);
				cl_qmap_insert(&ssa_db->p_lft_db->ep_db_lft_block_tbl,
					       ep_rec_key, &p_lft_block_rec->map_item);
			}
		}
	}

	guids = (uint32_t) cl_qmap_count(&p_subn->port_guid_tbl);
	if (!p_ssa->p_guid_to_lid_tbl) {
		p_ssa->p_guid_to_lid_tbl = (struct ep_guid_to_lid_tbl_rec *)
				malloc(sizeof(*p_ssa->p_guid_to_lid_tbl) * guids);
		if (!p_ssa->p_guid_to_lid_tbl) {
			/* add memory allocation failure handling */
			ssa_log(SSA_LOG_VERBOSE, "Port GUID to LID rec memory allocation failed");
		}
	}

	p_guid_to_lid_tbl_rec = (struct ep_guid_to_lid_tbl_rec *)
		malloc(sizeof(*p_guid_to_lid_tbl_rec));
	if (!p_guid_to_lid_tbl_rec) {
			/* TODO: add memory allocation failure handling */
	}

	links = guids + switch_ports_num;
	if (!p_ssa->p_link_tbl) {
		p_ssa->p_link_tbl = (struct ep_link_tbl_rec *)
				malloc(sizeof(*p_ssa->p_link_tbl) * links);
		if (!p_ssa->p_link_tbl) {
			/* add memory allocation failure handling */
			ssa_log(SSA_LOG_VERBOSE, "Link rec memory allocation failed");
		}
	}

	p_link_tbl_rec = (struct ep_link_tbl_rec *)
		malloc(sizeof(*p_link_tbl_rec));
	if (!p_link_tbl_rec) {
			/* TODO: add memory allocation failure handling */
	}

	ports = links;
	if (!p_ssa->p_port_tbl) {
		p_ssa->p_port_tbl = (struct ep_port_tbl_rec *)
				malloc(sizeof(*p_ssa->p_port_tbl) * ports);
		if (!p_ssa->p_port_tbl) {
			/* add memory allocation failure handling */
			ssa_log(SSA_LOG_VERBOSE, "Port rec memory allocation failed");
		}
	}

	p_port_tbl_rec = (struct ep_port_tbl_rec *)
		malloc(sizeof(*p_port_tbl_rec));
	if (!p_port_tbl_rec) {
			/* TODO: add memory allocation failure handling */
	}

	p_next_port = (osm_port_t *)cl_qmap_head(&p_subn->port_guid_tbl);
	while (p_next_port !=
	       (osm_port_t *)cl_qmap_end(&p_subn->port_guid_tbl)) {
		p_port = p_next_port;
		p_next_port = (osm_port_t *)cl_qmap_next(&p_port->map_item);
		p_pkey_tbl = osm_physp_get_pkey_tbl(p_port->p_physp);
		port_pkeys_num += (uint32_t) cl_map_count((const cl_map_t *) &p_pkey_tbl->keys);
	}

	pkeys = port_pkeys_num;
	if (!p_ssa->p_pkey_tbl) {
		p_ssa->p_pkey_tbl = (uint16_t *)
				malloc(sizeof(*p_ssa->p_pkey_tbl) * pkeys);
		if (!p_ssa->p_pkey_tbl) {
			/* add memory allocation failure handling */
			ssa_log(SSA_LOG_VERBOSE, "PKEY rec memory allocation failed");
		}
	}
	p_ssa->pkey_tbl_rec_num = pkeys;

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

		ep_guid_to_lid_tbl_rec_init(p_port, p_guid_to_lid_tbl_rec);
		memcpy(&p_ssa->p_guid_to_lid_tbl[guid_to_lid_offset],
		       p_guid_to_lid_tbl_rec, sizeof(*p_guid_to_lid_tbl_rec));
		p_map_rec = ep_map_rec_init(guid_to_lid_offset);
		if (!p_map_rec) {
			/* add memory allocation failure handling */
			ssa_log(SSA_LOG_VERBOSE, "Quick MAP rec memory allocation failed");
		}
		guid_to_lid_offset++;
		cl_qmap_insert(&p_ssa->ep_guid_to_lid_tbl,
			       osm_physp_get_port_guid(p_port->p_physp),
			       &p_map_rec->map_item);

		/* TODO:: add log info ??? */
		p_node = p_port->p_physp->p_node;
		if (osm_node_get_type(p_node) ==
					IB_NODE_TYPE_SWITCH) {
			for (i = 0; i < p_node->physp_tbl_size; i++) {
				p_physp = osm_node_get_physp_ptr(p_node, i);
				if (!p_physp)
					continue;

				if (i == 0) {
					lids = osm_physp_get_base_lid(p_physp);
					p_pkey_tbl = osm_physp_get_pkey_tbl(p_port->p_physp);
					pkey_map_iter = cl_map_head(&p_pkey_tbl->keys);
					while (pkey_map_iter != cl_map_end(&p_pkey_tbl->keys)) {
						p_pkey = (uint16_t *) cl_map_obj(pkey_map_iter);
						memcpy(&p_ssa->p_pkey_tbl[pkey_base_offset + pkey_cur_offset],
						       p_pkey, sizeof(*p_ssa->p_pkey_tbl));
						pkey_cur_offset++;
						pkey_map_iter = cl_qmap_next(pkey_map_iter);
					}
				}

				ep_rec_key = ep_rec_gen_key(lids, osm_physp_get_port_num(p_physp));

				ep_port_tbl_rec_init(p_physp, p_port_tbl_rec);
				p_port_tbl_rec->port_lid = lids;
				if (i == 0 ) {
					p_port_tbl_rec->pkey_tbl_offset = pkey_base_offset;
					p_port_tbl_rec->pkeys = pkey_cur_offset;
				}
				memcpy(&p_ssa->p_port_tbl[port_offset],
				       p_port_tbl_rec, sizeof(*p_port_tbl_rec));
				p_map_rec = ep_map_rec_init(port_offset++);
				cl_qmap_insert(&p_ssa->ep_port_tbl, ep_rec_key,
					       &p_map_rec->map_item);

				if (!osm_physp_get_remote(p_physp))
					continue;

				ep_link_tbl_rec_init(p_physp, p_link_tbl_rec);
				memcpy(&p_ssa->p_link_tbl[link_offset],
				       p_link_tbl_rec, sizeof(*p_link_tbl_rec));
				p_map_rec = ep_map_rec_init(link_offset++);
				if (p_map_rec)
					cl_qmap_insert(&p_ssa->ep_link_tbl,
						       ep_rec_key,
						       &p_map_rec->map_item);
			}
		} else {
			p_physp = p_port->p_physp;
			ep_rec_key = ep_rec_gen_key(
					cl_ntoh16(osm_physp_get_base_lid(p_physp)),
					osm_physp_get_port_num(p_physp));

			p_pkey_tbl = osm_physp_get_pkey_tbl(p_port->p_physp);
			pkey_map_iter = cl_map_head(&p_pkey_tbl->keys);
			while (pkey_map_iter != cl_map_end(&p_pkey_tbl->keys)) {
				p_pkey = (uint16_t *) cl_map_obj(pkey_map_iter);
				memcpy(&p_ssa->p_pkey_tbl[pkey_base_offset + pkey_cur_offset],
				       p_pkey, sizeof(*p_ssa->p_pkey_tbl));
				pkey_cur_offset++;
				pkey_map_iter = cl_qmap_next(pkey_map_iter);
			}

			ep_port_tbl_rec_init(p_physp, p_port_tbl_rec);
			p_port_tbl_rec->pkey_tbl_offset = pkey_base_offset;
			p_port_tbl_rec->pkeys = pkey_cur_offset;
			memcpy(&p_ssa->p_port_tbl[port_offset],
			       p_port_tbl_rec, sizeof(*p_port_tbl_rec));
			p_map_rec = ep_map_rec_init(port_offset++);
			cl_qmap_insert(&p_ssa->ep_port_tbl, ep_rec_key,
				       &p_map_rec->map_item);

			if (!osm_physp_get_remote(p_physp))
				continue;

			ep_link_tbl_rec_init(p_physp, p_link_tbl_rec);
			memcpy(&p_ssa->p_link_tbl[link_offset],
			       p_link_tbl_rec, sizeof(*p_link_tbl_rec));
			p_map_rec = ep_map_rec_init(link_offset++);
			if (p_map_rec)
				cl_qmap_insert(&p_ssa->ep_link_tbl,
					       ep_rec_key,
					       &p_map_rec->map_item);
		}
		pkey_base_offset += pkey_cur_offset;
		pkey_cur_offset = 0;
	}

	free(p_port_tbl_rec);
	free(p_link_tbl_rec);
	free(p_guid_to_lid_tbl_rec);
	free(p_lft_top_tbl_rec);
	free(p_node_tbl_rec);

	p_ssa->initialized = 1;

	ssa_log(SSA_LOG_VERBOSE, "]\n");

	return p_ssa;
}

/** =========================================================================
 */
void ssa_db_validate_lft(struct ssa_events *ssa)
{
	struct ep_lft_block_rec *p_lft_block, *p_next_lft_block;
	struct ep_lft_top_tbl_rec lft_top_tbl_rec;
	int i;

	if (!first_time_subnet_up)
		return;

	p_next_lft_block = (struct ep_lft_block_rec *)
				cl_qmap_head(&ssa_db->p_lft_db->ep_db_lft_block_tbl);
	while (p_next_lft_block != (struct ep_lft_block_rec *)
				cl_qmap_end(&ssa_db->p_lft_db->ep_db_lft_block_tbl)) {
		p_lft_block = p_next_lft_block;
		p_next_lft_block = (struct ep_lft_block_rec *)
					cl_qmap_next(&p_lft_block->map_item);
		ssa_log(SSA_LOG_VERBOSE, "LFT Block Record: LID %u Block num %u\n",
			p_lft_block->lid,
			p_lft_block->block_num);
	}

	for (i = 0; i < cl_qmap_count(&ssa_db->p_lft_db->ep_db_lft_top_tbl); i++) {
		lft_top_tbl_rec = ssa_db->p_lft_db->p_db_lft_top_tbl[i];
		ssa_log(SSA_LOG_VERBOSE, "LFT Top Record: LID %u New Top %u\n",
			lft_top_tbl_rec.lid,
			lft_top_tbl_rec.lft_top);
	}
}

/** =========================================================================
 */
void ssa_db_validate(struct ssa_events *ssa, struct ssa_db *p_ssa_db)
{
	struct ep_guid_to_lid_tbl_rec guid_to_lid_tbl_rec;
	struct ep_node_tbl_rec node_tbl_rec;
	struct ep_link_tbl_rec link_tbl_rec;
	struct ep_port_tbl_rec port_tbl_rec;
	char buffer[64];
	uint64_t i;

	if (!p_ssa_db || !p_ssa_db->initialized)
		return;

	ssa_log(SSA_LOG_VERBOSE, "[\n");

	/* First, most Fabric/SM related parameters */
	ssa_log(SSA_LOG_VERBOSE, "Subnet prefix 0x%" PRIx64 "\n", p_ssa_db->subnet_prefix);
	ssa_log(SSA_LOG_VERBOSE, "LMC %u Subnet timeout %u Both Pkeys %sabled\n",
		p_ssa_db->lmc, p_ssa_db->subnet_timeout,
		p_ssa_db->allow_both_pkeys ? "en" : "dis");

	for (i = 0; i < cl_qmap_count(&p_ssa_db->ep_node_tbl); i++) {
		node_tbl_rec = p_ssa_db->p_node_tbl[i];
		if (node_tbl_rec.node_type == IB_NODE_TYPE_SWITCH)
			sprintf(buffer, " with %s Switch Port 0\n",
				node_tbl_rec.is_enhanced_sp0 ? "Enhanced" : "Base");
		else
			sprintf(buffer, "\n");
		ssa_log(SSA_LOG_VERBOSE, "Node GUID 0x%" PRIx64 " Type %d%s",
			cl_ntoh64(node_tbl_rec.node_guid),
			node_tbl_rec.node_type,
			buffer);
	}

	for (i = 0; i < cl_qmap_count(&p_ssa_db->ep_guid_to_lid_tbl); i++) {
		guid_to_lid_tbl_rec = p_ssa_db->p_guid_to_lid_tbl[i];
		ssa_log(SSA_LOG_VERBOSE, "Port GUID 0x%" PRIx64 " LID %u LMC %u is_switch %d\n",
			cl_ntoh64(guid_to_lid_tbl_rec.guid), cl_ntoh16(guid_to_lid_tbl_rec.lid),
			guid_to_lid_tbl_rec.lmc, guid_to_lid_tbl_rec.is_switch);

	}

	for (i = 0; i < cl_qmap_count(&p_ssa_db->ep_port_tbl); i++) {
		port_tbl_rec = p_ssa_db->p_port_tbl[i];
		ssa_log(SSA_LOG_VERBOSE, "Port LID %u Port Num %u\n",
			cl_ntoh16(port_tbl_rec.port_lid), port_tbl_rec.port_num);
		ssa_log(SSA_LOG_VERBOSE, "FDR10 %s active\n",
			port_tbl_rec.is_fdr10_active ? "" : "not");
		ssa_log(SSA_LOG_VERBOSE, "PKeys %u \n", port_tbl_rec.pkeys);
	}

	for (i = 0; i < cl_qmap_count(&p_ssa_db->ep_link_tbl); i++) {
		link_tbl_rec = p_ssa_db->p_link_tbl[i];
		ssa_log(SSA_LOG_VERBOSE, "Link Record: from LID %u port %u to LID %u port %u\n",
			cl_ntoh16(link_tbl_rec.from_lid), link_tbl_rec.from_port_num,
			cl_ntoh16(link_tbl_rec.to_lid), link_tbl_rec.to_port_num);
	}

	ssa_log(SSA_LOG_VERBOSE, "]\n");
}

/** =========================================================================
 */
void ssa_db_remove(struct ssa_events *ssa, struct ssa_db *p_ssa_db)
{
	struct ep_map_rec *p_map_rec, *p_map_rec_next;

	if (!p_ssa_db || !p_ssa_db->initialized)
		return;

	ssa_log(SSA_LOG_VERBOSE, "[\n");


	p_map_rec_next = (struct ep_map_rec *)cl_qmap_head(&p_ssa_db->ep_guid_to_lid_tbl);
	while (p_map_rec_next !=
	       (struct ep_map_rec *)cl_qmap_end(&p_ssa_db->ep_guid_to_lid_tbl)) {
		p_map_rec = p_map_rec_next;
		p_map_rec_next = (struct ep_map_rec *)cl_qmap_next(&p_map_rec->map_item);
		cl_qmap_remove_item(&p_ssa_db->ep_guid_to_lid_tbl,
				    &p_map_rec->map_item);
		ep_map_rec_delete(p_map_rec);
	}

	p_map_rec_next = (struct ep_map_rec *)cl_qmap_head(&p_ssa_db->ep_node_tbl);
	while (p_map_rec_next !=
	       (struct ep_map_rec *)cl_qmap_end(&p_ssa_db->ep_node_tbl)) {
		p_map_rec = p_map_rec_next;
		p_map_rec_next = (struct ep_map_rec *)cl_qmap_next(&p_map_rec->map_item);
		cl_qmap_remove_item(&p_ssa_db->ep_node_tbl,
				    &p_map_rec->map_item);
		ep_map_rec_delete(p_map_rec);
	}

	p_map_rec_next = (struct ep_map_rec *)cl_qmap_head(&p_ssa_db->ep_port_tbl);
	while (p_map_rec_next !=
	       (struct ep_map_rec *)cl_qmap_end(&p_ssa_db->ep_port_tbl)) {
		p_map_rec = p_map_rec_next;
		p_map_rec_next = (struct ep_map_rec *)cl_qmap_next(&p_map_rec->map_item);
		cl_qmap_remove_item(&p_ssa_db->ep_port_tbl,
				    &p_map_rec->map_item);
		ep_map_rec_delete(p_map_rec);
	}

	p_map_rec_next = (struct ep_map_rec *)cl_qmap_head(&p_ssa_db->ep_link_tbl);
	while (p_map_rec_next !=
	       (struct ep_map_rec *)cl_qmap_end(&p_ssa_db->ep_link_tbl)) {
		p_map_rec = p_map_rec_next;
		p_map_rec_next = (struct ep_map_rec *)cl_qmap_next(&p_map_rec->map_item);
		cl_qmap_remove_item(&p_ssa_db->ep_link_tbl,
				    &p_map_rec->map_item);
		ep_map_rec_delete(p_map_rec);
	}

	p_ssa_db->initialized = 0;
	ssa_log(SSA_LOG_VERBOSE, "]\n");
}

/** =========================================================================
 */
/* TODO:: Add meaningfull return value */
void ssa_db_update(struct ssa_events *ssa,
		   struct ssa_database *ssa_db)
{
	ssa_log(SSA_LOG_VERBOSE, "[\n");

        if (!ssa_db || !ssa_db->p_previous_db ||
	    !ssa_db->p_current_db || !ssa_db->p_dump_db) {
                /* error handling */
                return;
        }

	/* Updating previous SMDB with current one */
	if (ssa_db->p_current_db->initialized) {
		ssa_db_remove(ssa, ssa_db->p_previous_db);
		ssa_db_delete(ssa_db->p_previous_db);
		ssa_db->p_previous_db = ssa_db->p_current_db;
	}
	ssa_db->p_current_db = ssa_db->p_dump_db;
	ssa_db->p_dump_db = ssa_db_init();

	ssa_log(SSA_LOG_VERBOSE, "]\n");
}
