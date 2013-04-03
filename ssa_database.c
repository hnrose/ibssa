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
		ssa_qmap_apply_func(&p_ssa_db->ep_node_tbl, ep_node_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db->ep_guid_to_lid_tbl,
				    ep_guid_to_lid_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db->ep_port_tbl, ep_port_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db->ep_link_tbl, ep_link_rec_delete_pfn);

		cl_qmap_remove_all(&p_ssa_db->ep_node_tbl);
		cl_qmap_remove_all(&p_ssa_db->ep_guid_to_lid_tbl);
		cl_qmap_remove_all(&p_ssa_db->ep_port_tbl);
		cl_qmap_remove_all(&p_ssa_db->ep_link_tbl);
		free(p_ssa_db);
	}
}

struct ep_guid_to_lid_rec *ep_guid_to_lid_rec_init(osm_port_t *p_port)
{
        struct ep_guid_to_lid_rec *p_ep_guid_to_lid_rec;

	p_ep_guid_to_lid_rec = (struct ep_guid_to_lid_rec *) malloc(sizeof(*p_ep_guid_to_lid_rec));
	if (p_ep_guid_to_lid_rec) {
		p_ep_guid_to_lid_rec->lid =
			cl_ntoh16(osm_physp_get_base_lid(p_port->p_physp));
		p_ep_guid_to_lid_rec->lmc = osm_physp_get_lmc(p_port->p_physp);
		p_ep_guid_to_lid_rec->is_switch =
		    (osm_node_get_type(p_port->p_node) == IB_NODE_TYPE_SWITCH);
	}
	return p_ep_guid_to_lid_rec;
}

void ep_guid_to_lid_rec_copy(OUT struct ep_guid_to_lid_rec *p_dest_rec,
			     IN struct ep_guid_to_lid_rec *p_src_rec)
{
	memcpy(&p_dest_rec->lid, &p_src_rec->lid, sizeof(*p_dest_rec) -
	       offsetof(struct ep_guid_to_lid_rec, lid));
}

void ep_guid_to_lid_rec_delete(struct ep_guid_to_lid_rec *p_ep_guid_to_lid_rec)
{
	free(p_ep_guid_to_lid_rec);
}

void ep_guid_to_lid_rec_delete_pfn(cl_map_item_t * p_map_item)
{
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec;

	p_guid_to_lid_rec = (struct ep_guid_to_lid_rec *) p_map_item;
	ep_guid_to_lid_rec_delete(p_guid_to_lid_rec);
}

struct ep_node_rec *ep_node_rec_init(osm_node_t *p_node)
{
	struct ep_node_rec *p_ep_node_rec;

	p_ep_node_rec = (struct ep_node_rec *) malloc(sizeof(*p_ep_node_rec));
	if (p_ep_node_rec) {
		memcpy(&p_ep_node_rec->node_info, &p_node->node_info,
		       sizeof(p_ep_node_rec->node_info));
		memcpy(&p_ep_node_rec->node_desc, &p_node->node_desc,
		       sizeof(p_ep_node_rec->node_desc));
		if (p_node->node_info.node_type == IB_NODE_TYPE_SWITCH)
			p_ep_node_rec->is_enhanced_sp0 =
			    ib_switch_info_is_enhanced_port0(&p_node->sw->switch_info);
		else
			p_ep_node_rec->is_enhanced_sp0 = 0;
	}
	return p_ep_node_rec;
}

void ep_node_rec_copy(OUT struct ep_node_rec *p_dest_rec,
		      IN struct ep_node_rec *p_src_rec)
{
	memcpy(&p_dest_rec->node_info, &p_src_rec->node_info, sizeof(*p_dest_rec) -
	       offsetof(struct ep_node_rec, node_info));
}

void ep_node_rec_delete(struct ep_node_rec *p_ep_node_rec)
{
	free(p_ep_node_rec);
}

void ep_node_rec_delete_pfn(cl_map_item_t * p_map_item)
{
	struct ep_node_rec *p_node_rec;

	p_node_rec = (struct ep_node_rec *) p_map_item;
	ep_node_rec_delete(p_node_rec);
}

struct ep_link_rec *ep_link_rec_init(osm_physp_t * p_physp)
{
	struct ep_link_rec *p_ep_link_rec;
	osm_physp_t *p_remote_physp;

	p_ep_link_rec = (struct ep_link_rec *) malloc(sizeof(*p_ep_link_rec));
	if (p_ep_link_rec) {
		if (osm_node_get_type(p_physp->p_node) == IB_NODE_TYPE_SWITCH) {
			p_ep_link_rec->link_rec.from_lid =
				osm_node_get_base_lid(p_physp->p_node, 0);
			p_ep_link_rec->link_rec.from_port_num =
				osm_physp_get_port_num(p_physp);
		} else {
			p_ep_link_rec->link_rec.from_lid =
				osm_physp_get_base_lid(p_physp);
			p_ep_link_rec->link_rec.from_port_num = 0;
		}

		p_remote_physp = osm_physp_get_remote(p_physp);
		if (!p_remote_physp) {
			/* TODO: add handling for remote port missing */
			free(p_ep_link_rec);
			return NULL;
		}

		if (osm_node_get_type(p_remote_physp->p_node) ==
							IB_NODE_TYPE_SWITCH) {
			p_ep_link_rec->link_rec.to_lid =
				osm_node_get_base_lid(p_remote_physp->p_node, 0);
			p_ep_link_rec->link_rec.to_port_num =
				osm_physp_get_port_num(p_remote_physp);
		} else {
			p_ep_link_rec->link_rec.to_lid =
				osm_physp_get_base_lid(p_remote_physp);
			p_ep_link_rec->link_rec.to_port_num = 0;
		}
	}
	return p_ep_link_rec;
}

void ep_link_rec_copy(struct ep_link_rec *p_dest_rec,
		      struct ep_link_rec *p_src_rec)
{
	memcpy(&p_dest_rec->link_rec, &p_src_rec->link_rec,
	       sizeof(p_dest_rec->link_rec));
}

void ep_link_rec_delete(struct ep_link_rec *p_ep_link_rec)
{
	free(p_ep_link_rec);
}

void ep_link_rec_delete_pfn(cl_map_item_t *p_map_item)
{
	struct ep_link_rec *p_link_rec;

	p_link_rec = (struct ep_link_rec *) p_map_item;
	ep_link_rec_delete(p_link_rec);
}

struct ep_lft_block_rec *ep_lft_block_rec_init(IN osm_switch_t * p_sw,
					       IN uint16_t lid,
					       IN uint16_t block)
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

void ep_lft_block_rec_copy(OUT struct ep_lft_block_rec * p_dest_rec,
			   IN struct ep_lft_block_rec * p_src_rec)
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

struct ep_lft_top_rec *ep_lft_top_rec_init(IN uint16_t lid,
					   IN uint16_t lft_top)
{
	struct ep_lft_top_rec *p_lft_top_rec;

	p_lft_top_rec = (struct ep_lft_top_rec*) malloc(sizeof(*p_lft_top_rec));
	if (p_lft_top_rec) {
		p_lft_top_rec->lid = lid;
		p_lft_top_rec->lft_top = lft_top;
	}
	return p_lft_top_rec;
}

void ep_lft_top_rec_copy(OUT struct ep_lft_top_rec * p_dest_rec,
			   IN struct ep_lft_top_rec * p_src_rec)
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

struct ep_port_rec *ep_port_rec_init(osm_physp_t *p_physp)
{
	struct ep_port_rec *p_ep_port_rec;
	ib_pkey_table_t *pkey_tbl;
	ib_slvl_table_t *p_slvl_tbl, *p_slvl_tbl_new;
	cl_status_t status;
	uint16_t used_blocks = p_physp->pkeys.used_blocks;
	uint16_t block_index;
	uint8_t slvl_rec = cl_ptr_vector_get_size(&p_physp->slvl_by_port);
	uint8_t i;

	p_ep_port_rec = (struct ep_port_rec *) malloc(sizeof(*p_ep_port_rec) +
						      sizeof(p_ep_port_rec->ep_pkey_rec.pkey_tbl[0]) * used_blocks);
	if (p_ep_port_rec) {
		/* PORT INFO */
		p_ep_port_rec->mtu_cap			= p_physp->port_info.mtu_cap;
		p_ep_port_rec->link_speed_ext		= p_physp->port_info.link_speed_ext;
		p_ep_port_rec->link_speed		= p_physp->port_info.link_speed;
		p_ep_port_rec->link_width_active	= p_physp->port_info.link_width_active;
		p_ep_port_rec->vl_enforce		= p_physp->port_info.vl_enforce;

		/* slvl tables vector initialization */
		status = cl_ptr_vector_init(&p_ep_port_rec->slvl_by_port, slvl_rec, 1);
		if (status != CL_SUCCESS) {
			/* handle failure !!! */
		}
		for (i = 0; i < slvl_rec; i++) {
			cl_ptr_vector_at(&p_physp->slvl_by_port, i, (void*)&p_slvl_tbl);
			if (!p_slvl_tbl)
				continue;
			p_slvl_tbl_new = (ib_slvl_table_t *) malloc(sizeof(*p_slvl_tbl_new));
			if (!p_slvl_tbl_new) {
				/* handle failure !!! */
			}
			memcpy(p_slvl_tbl_new, p_slvl_tbl, sizeof(*p_slvl_tbl_new));
			cl_ptr_vector_set(&p_ep_port_rec->slvl_by_port, i, p_slvl_tbl_new);
		}

		p_ep_port_rec->is_fdr10_active =
			p_physp->ext_port_info.link_speed_active & FDR10;
		p_ep_port_rec->ep_pkey_rec.max_pkeys =
			cl_ntoh16(p_physp->p_node->node_info.partition_cap);
		p_ep_port_rec->ep_pkey_rec.used_blocks = used_blocks;
		for (block_index = 0; block_index < used_blocks;
		     block_index++) {
			pkey_tbl = osm_pkey_tbl_block_get(osm_physp_get_pkey_tbl(p_physp), block_index);
			if (pkey_tbl)
				memcpy(&p_ep_port_rec->ep_pkey_rec.pkey_tbl[block_index],
				       pkey_tbl,
				       sizeof(p_ep_port_rec->ep_pkey_rec.pkey_tbl[0]));
			else {
				/* handle failure !!! */

			}
		}
	}
	return p_ep_port_rec;
}

void ep_port_rec_copy(OUT struct ep_port_rec *p_dest_rec,
		      IN struct ep_port_rec *p_src_rec)
{
	ib_slvl_table_t *p_slvl_tbl, *p_slvl_tbl_new;
	uint16_t used_blocks;
	uint8_t i, slvl_num;

	/* PORT INFO data */
	p_dest_rec->mtu_cap			= p_src_rec->mtu_cap;
	p_dest_rec->link_speed_ext		= p_src_rec->link_speed_ext;
	p_dest_rec->link_speed			= p_src_rec->link_speed;
	p_dest_rec->link_width_active		= p_src_rec->link_width_active;
	p_dest_rec->vl_enforce			= p_src_rec->vl_enforce;

	p_dest_rec->is_fdr10_active = p_src_rec->is_fdr10_active;

	slvl_num = cl_ptr_vector_get_size(&p_src_rec->slvl_by_port);
	cl_ptr_vector_init(&p_dest_rec->slvl_by_port, slvl_num, 1);
	for (i = 0; i < slvl_num; i++) {
		p_slvl_tbl = cl_ptr_vector_get(&p_src_rec->slvl_by_port, i);
		p_slvl_tbl_new = (ib_slvl_table_t *) malloc(sizeof(*p_slvl_tbl_new));
		if (!p_slvl_tbl_new) {
			/* handle failure !!! */
		}
		memcpy(p_slvl_tbl_new, p_slvl_tbl, sizeof(*p_slvl_tbl_new));
		cl_ptr_vector_set(&p_dest_rec->slvl_by_port, i, p_slvl_tbl_new);
	}

	used_blocks = p_src_rec->ep_pkey_rec.used_blocks;
	p_dest_rec->ep_pkey_rec.max_pkeys = p_src_rec->ep_pkey_rec.max_pkeys;
	p_dest_rec->ep_pkey_rec.used_blocks = used_blocks;
	memcpy(p_dest_rec->ep_pkey_rec.pkey_tbl, p_src_rec->ep_pkey_rec.pkey_tbl,
	       sizeof(p_dest_rec->ep_pkey_rec.pkey_tbl[0]) * used_blocks);
}

void ep_port_rec_delete(struct ep_port_rec *p_ep_port_rec)
{
	size_t i, num_slvl;
	if (!p_ep_port_rec)
		return;

	/* TODO:: fix size to capacity */
	num_slvl = cl_ptr_vector_get_size(&p_ep_port_rec->slvl_by_port);
	for (i = 0; i < num_slvl; i++)
		free(cl_ptr_vector_get(&p_ep_port_rec->slvl_by_port, i));
	cl_ptr_vector_destroy(&p_ep_port_rec->slvl_by_port);
	free(p_ep_port_rec);
}

void ep_port_rec_delete_pfn(cl_map_item_t * p_map_item)
{
	struct ep_port_rec *p_port_rec;

	p_port_rec = (struct ep_port_rec *) p_map_item;
	ep_port_rec_delete(p_port_rec);
}

uint64_t ep_rec_gen_key(uint16_t lid, uint8_t port_num)
{
	uint64_t key;
	key = (uint64_t) lid;
	key |= (uint64_t) port_num << 16;
	return key;
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
