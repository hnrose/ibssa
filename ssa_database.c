/*
 * Copyright (c) 2011-2012 Mellanox Technologies LTD. All rights reserved.
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

#include <stdlib.h>
#include <stddef.h>
#include <ssa_database.h>
#include <opensm/osm_switch.h>

struct ssa_database *ssa_database_init()
{
	struct ssa_database *p_ssa_database =
		(struct ssa_database *) calloc(1, sizeof(struct ssa_database));
	if (p_ssa_database) {
		p_ssa_database->p_lft_db = (struct ssa_db_lft *)
					malloc(sizeof(*p_ssa_database->p_lft_db));
		if (p_ssa_database->p_lft_db) {
			cl_qmap_init(&p_ssa_database->p_lft_db->ep_db_lft_block_tbl);
			cl_qmap_init(&p_ssa_database->p_lft_db->ep_db_lft_top_tbl);
			cl_qmap_init(&p_ssa_database->p_lft_db->ep_dump_lft_block_tbl);
			cl_qmap_init(&p_ssa_database->p_lft_db->ep_dump_lft_top_tbl);
		} else {
			free(p_ssa_database);
			p_ssa_database = NULL;
		}
	}
	return p_ssa_database;
}

void ssa_database_delete(struct ssa_database *p_ssa_db)
{
	if (p_ssa_db) {
		ssa_db_delete(p_ssa_db->p_dump_db);
		ssa_db_delete(p_ssa_db->p_previous_db);
		ssa_db_delete(p_ssa_db->p_current_db);
		if (p_ssa_db->p_lft_db) {
			ssa_qmap_apply_func(&p_ssa_db->p_lft_db->ep_db_lft_block_tbl,
					    ep_lft_block_rec_delete_pfn);
			ssa_qmap_apply_func(&p_ssa_db->p_lft_db->ep_db_lft_top_tbl,
					    ep_lft_top_rec_delete_pfn);
			ssa_qmap_apply_func(&p_ssa_db->p_lft_db->ep_dump_lft_block_tbl,
					    ep_lft_block_rec_delete_pfn);
			ssa_qmap_apply_func(&p_ssa_db->p_lft_db->ep_dump_lft_top_tbl,
					    ep_lft_top_rec_delete_pfn);
			cl_qmap_remove_all(&p_ssa_db->p_lft_db->ep_db_lft_block_tbl);
			cl_qmap_remove_all(&p_ssa_db->p_lft_db->ep_db_lft_top_tbl);
			cl_qmap_remove_all(&p_ssa_db->p_lft_db->ep_dump_lft_block_tbl);
			cl_qmap_remove_all(&p_ssa_db->p_lft_db->ep_dump_lft_top_tbl);

			free(p_ssa_db->p_lft_db);
		}
		free(p_ssa_db);
	}
}

struct ssa_db *ssa_db_init()
{
	struct ssa_db *p_ssa_db;

	p_ssa_db = (struct ssa_db *) calloc(1, sizeof(*p_ssa_db));
	if (p_ssa_db) {
		cl_qmap_init(&p_ssa_db->ep_guid_to_lid_tbl);
		cl_qmap_init(&p_ssa_db->ep_node_tbl);
		cl_qmap_init(&p_ssa_db->ep_port_tbl);
		cl_qmap_init(&p_ssa_db->ep_link_tbl);
	}
	return p_ssa_db;
}

void ssa_db_delete(struct ssa_db *p_ssa_db)
{
	if (p_ssa_db) {
		free(p_ssa_db->p_pkey_tbl);
		free(p_ssa_db->p_port_tbl);
		free(p_ssa_db->p_link_tbl);
		free(p_ssa_db->p_guid_to_lid_tbl);
		free(p_ssa_db->p_node_tbl);

		ssa_qmap_apply_func(&p_ssa_db->ep_guid_to_lid_tbl, ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db->ep_node_tbl, ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db->ep_port_tbl, ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db->ep_link_tbl, ep_map_rec_delete_pfn);

		cl_qmap_remove_all(&p_ssa_db->ep_node_tbl);
		cl_qmap_remove_all(&p_ssa_db->ep_guid_to_lid_tbl);
		cl_qmap_remove_all(&p_ssa_db->ep_port_tbl);
		cl_qmap_remove_all(&p_ssa_db->ep_link_tbl);
		free(p_ssa_db);
	}
}

void ep_guid_to_lid_tbl_rec_init(osm_port_t *p_port,
				 struct ep_guid_to_lid_tbl_rec *p_rec)
{
	p_rec->guid = osm_physp_get_port_guid(p_port->p_physp);
	p_rec->lid = osm_physp_get_base_lid(p_port->p_physp);
	p_rec->lmc = osm_physp_get_lmc(p_port->p_physp);
	p_rec->is_switch = (osm_node_get_type(p_port->p_node) == IB_NODE_TYPE_SWITCH);

	memset(&p_rec->pad, 0, sizeof(p_rec->pad));
}

void ep_node_tbl_rec_init(osm_node_t *p_node, struct ep_node_tbl_rec *p_rec)
{
	p_rec->node_guid = osm_node_get_node_guid(p_node);
	if (p_node->node_info.node_type == IB_NODE_TYPE_SWITCH)
		p_rec->is_enhanced_sp0 =
			ib_switch_info_is_enhanced_port0(&p_node->sw->switch_info);
	else
		p_rec->is_enhanced_sp0 = 0;
	p_rec->node_type = p_node->node_info.node_type;
	memcpy(p_rec->description, p_node->node_desc.description, sizeof(p_rec->description));
	memset(&p_rec->pad, 0, sizeof(p_rec->pad));
}

void ep_link_tbl_rec_init(osm_physp_t *p_physp, struct ep_link_tbl_rec *p_rec)
{
	osm_physp_t *p_remote_physp;

	if (osm_node_get_type(p_physp->p_node) == IB_NODE_TYPE_SWITCH) {
		p_rec->from_lid = osm_node_get_base_lid(p_physp->p_node, 0);
		p_rec->from_port_num = osm_physp_get_port_num(p_physp);
	} else {
		p_rec->from_lid = osm_physp_get_base_lid(p_physp);
		p_rec->from_port_num = 0;
	}

	p_remote_physp = osm_physp_get_remote(p_physp);

	if (osm_node_get_type(p_remote_physp->p_node) ==
						IB_NODE_TYPE_SWITCH) {
		p_rec->to_lid = osm_node_get_base_lid(p_remote_physp->p_node, 0);
		p_rec->to_port_num =osm_physp_get_port_num(p_remote_physp);
	} else {
		p_rec->to_lid = osm_physp_get_base_lid(p_remote_physp);
		p_rec->to_port_num = 0;
	}
	memset(&p_rec->pad, 0, sizeof(p_rec->pad));
}

void ep_port_tbl_rec_init(osm_physp_t *p_physp, struct ep_port_tbl_rec *p_rec)
{
	p_rec->pkey_tbl_offset		= 0;
	p_rec->pkeys			= 0;
	p_rec->port_lid			= osm_physp_get_base_lid(p_physp);
	p_rec->port_num			= osm_physp_get_port_num(p_physp);
	p_rec->neighbor_mtu		= ib_port_info_get_neighbor_mtu(&p_physp->port_info);
	p_rec->link_speed_ext		= p_physp->port_info.link_speed_ext;
	p_rec->link_speed		= p_physp->port_info.link_speed;
	p_rec->link_width_active	= p_physp->port_info.link_width_active;
	p_rec->vl_enforce		= p_physp->port_info.vl_enforce;
	p_rec->is_fdr10_active		= p_physp->ext_port_info.link_speed_active & FDR10;

	memset(&p_rec->pad, 0, sizeof(p_rec->pad));
}

struct ep_lft_block_rec *ep_lft_block_rec_init(osm_switch_t * p_sw,
					       uint16_t lid,
					       uint16_t block)
{
	struct ep_lft_block_rec *p_lft_block_rec;

	p_lft_block_rec = (struct ep_lft_block_rec*) malloc(sizeof(*p_lft_block_rec));
	if (p_lft_block_rec) {
		p_lft_block_rec->lid = lid;
		p_lft_block_rec->block_num = block;
		memcpy(p_lft_block_rec->block, p_sw->lft + block * IB_SMP_DATA_SIZE,
		       IB_SMP_DATA_SIZE);
	}
	return p_lft_block_rec;
}

void ep_lft_block_rec_copy(struct ep_lft_block_rec * p_dest_rec,
			   struct ep_lft_block_rec * p_src_rec)
{
	p_dest_rec->lid = p_src_rec->lid;
	p_dest_rec->block_num = p_src_rec->block_num;
	memcpy(p_dest_rec->block, p_src_rec->block, sizeof(p_dest_rec->block));
}

inline uint64_t ep_lft_block_rec_gen_key(uint16_t lid, uint16_t block_num)
{
	uint64_t key;
	key = (uint64_t) lid;
	key |= (uint64_t) block_num << 16;
	return key;
}

void ep_lft_block_rec_delete(struct ep_lft_block_rec * p_lft_block_rec)
{
	free(p_lft_block_rec);
}

void ep_lft_block_rec_delete_pfn(cl_map_item_t *p_map_item)
{
	struct ep_lft_block_rec *p_lft_block_rec;

	p_lft_block_rec = (struct ep_lft_block_rec *) p_map_item;
	ep_lft_block_rec_delete(p_lft_block_rec);
}

/* TODO: make generic quick map clear method */
void ep_lft_block_rec_qmap_clear(cl_qmap_t * p_map)
{
	struct ep_lft_block_rec *p_lft_block, *p_lft_block_next;

	p_lft_block_next = (struct ep_lft_block_rec *) cl_qmap_head(p_map);
	while (p_lft_block_next !=
	       (struct ep_lft_block_rec *) cl_qmap_end(p_map)) {
		p_lft_block = p_lft_block_next;
		p_lft_block_next = (struct ep_lft_block_rec *) cl_qmap_next(&p_lft_block->map_item);
		cl_qmap_remove_item(p_map, &p_lft_block->map_item);
		ep_lft_block_rec_delete(p_lft_block);
	}
}

struct ep_lft_top_rec *ep_lft_top_rec_init(uint16_t lid,
					   uint16_t lft_top)
{
	struct ep_lft_top_rec *p_lft_top_rec;

	p_lft_top_rec = (struct ep_lft_top_rec*) malloc(sizeof(*p_lft_top_rec));
	if (p_lft_top_rec) {
		p_lft_top_rec->lid = lid;
		p_lft_top_rec->lft_top = lft_top;
	}
	return p_lft_top_rec;
}

void ep_lft_top_rec_copy(struct ep_lft_top_rec * p_dest_rec,
			 struct ep_lft_top_rec * p_src_rec)
{
	p_dest_rec->lid = p_src_rec->lid;
	p_dest_rec->lft_top = p_src_rec->lft_top;
}

inline uint64_t ep_lft_top_rec_gen_key(uint16_t lid)
{
	return (uint64_t) lid;
}

void ep_lft_top_rec_delete(struct ep_lft_top_rec * p_lft_top_rec)
{
	free(p_lft_top_rec);
}

void ep_lft_top_rec_delete_pfn(cl_map_item_t *p_map_item)
{
	struct ep_lft_top_rec *p_lft_top_rec;

	p_lft_top_rec = (struct ep_lft_top_rec *) p_map_item;
	ep_lft_top_rec_delete(p_lft_top_rec);
}

void ep_lft_top_rec_qmap_clear(cl_qmap_t * p_map)
{
	struct ep_lft_top_rec *p_lft_top, *p_lft_top_next;

	p_lft_top_next = (struct ep_lft_top_rec *) cl_qmap_head(p_map);
	while (p_lft_top_next !=
	       (struct ep_lft_top_rec *) cl_qmap_end(p_map)) {
		p_lft_top = p_lft_top_next;
		p_lft_top_next = (struct ep_lft_top_rec *) cl_qmap_next(&p_lft_top->map_item);
		cl_qmap_remove_item(p_map, &p_lft_top->map_item);
		ep_lft_top_rec_delete(p_lft_top);
	}
}


uint64_t ep_rec_gen_key(uint16_t lid, uint8_t port_num)
{
	uint64_t key;
	key = (uint64_t) lid;
	key |= (uint64_t) port_num << 16;
	return key;
}

struct ep_map_rec *ep_map_rec_init(uint64_t offset)
{
        struct ep_map_rec *p_map_rec;

	p_map_rec = (struct ep_map_rec *) malloc(sizeof(*p_map_rec));
	if (p_map_rec)
		p_map_rec->offset = offset;

	return p_map_rec;
}

void ep_map_rec_delete(struct ep_map_rec *p_map_rec)
{
	free(p_map_rec);
}

void ep_map_rec_delete_pfn(cl_map_item_t * p_map_item)
{
	struct ep_map_rec *p_map_rec;

	p_map_rec = (struct ep_map_rec *) p_map_item;
	ep_map_rec_delete(p_map_rec);
}

void ssa_qmap_apply_func(cl_qmap_t *p_qmap, void (*pfn_func)(cl_map_item_t *))
{
	cl_map_item_t *p_map_item, *p_map_item_next;
        p_map_item_next = cl_qmap_head(p_qmap);
        while (p_map_item_next != cl_qmap_end(p_qmap)) {
		p_map_item = p_map_item_next;
		p_map_item_next = cl_qmap_next(p_map_item);
                pfn_func(p_map_item);
        }
}
