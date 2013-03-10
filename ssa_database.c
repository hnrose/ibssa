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
#include <ssa_database.h>
#include <opensm/osm_switch.h>

struct ssa_database *ssa_database_init()
{
	return (struct ssa_database *) calloc(1, sizeof(struct ssa_database));
}

void ssa_database_delete(struct ssa_database *p_ssa_db)
{
	if (p_ssa_db) {
		ssa_db_delete(p_ssa_db->p_dump_db);
		ssa_db_delete(p_ssa_db->p_previous_db);
		ssa_db_delete(p_ssa_db->p_current_db);
		free(p_ssa_db);
	}
}

struct ssa_db *ssa_db_init(uint16_t lids)
{
	struct ssa_db *p_ssa_db;
	cl_status_t status;

	p_ssa_db = (struct ssa_db *) calloc(1, sizeof(*p_ssa_db));
	if (p_ssa_db) {
		cl_qmap_init(&p_ssa_db->ep_guid_to_lid_tbl);
		cl_qmap_init(&p_ssa_db->ep_node_tbl);
		status = cl_ptr_vector_init(&p_ssa_db->ep_port_tbl, 0, 1);
		if (status != CL_SUCCESS) {
			free(p_ssa_db);
			return NULL;
		}
		status = cl_ptr_vector_set_capacity(&p_ssa_db->ep_port_tbl,
						    lids);
		if (status != CL_SUCCESS) {
			free(p_ssa_db);
			return NULL;
		}
		cl_ptr_vector_set(&p_ssa_db->ep_port_tbl, 0, NULL);
	}
	return p_ssa_db;
}

void ssa_db_delete(struct ssa_db *p_ssa_db)
{
	if (p_ssa_db) {
		/* change removals once memory allocated!!! */
		/* TODO: slvl vector elements for each port_rec have to be freed */
		cl_ptr_vector_destroy(&p_ssa_db->ep_port_tbl);
		/* See ssa_plugin.c:remove_dump_db !!! */
		cl_qmap_remove_all(&p_ssa_db->ep_node_tbl);
		cl_qmap_remove_all(&p_ssa_db->ep_guid_to_lid_tbl);
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

void ep_guid_to_lid_rec_delete(struct ep_guid_to_lid_rec *p_ep_guid_to_lid_rec)
{
	free(p_ep_guid_to_lid_rec);
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

void ep_node_rec_delete(struct ep_node_rec *p_ep_node_rec)
{
	free(p_ep_node_rec);
}

struct ep_port_rec *ep_port_rec_init(osm_port_t *p_port)
{
	struct ep_port_rec *p_ep_port_rec;
	ib_pkey_table_t *pkey_tbl;
	ib_slvl_table_t *p_slvl_tbl, *p_slvl_tbl_new;
	cl_status_t status;
	uint16_t used_blocks = p_port->p_physp->pkeys.used_blocks;
	uint16_t block_index;
	uint8_t slvl_rec = cl_ptr_vector_get_size(&p_port->p_physp->slvl_by_port);
	uint8_t i;

	p_ep_port_rec = (struct ep_port_rec *) malloc(sizeof(*p_ep_port_rec) +
						      sizeof(p_ep_port_rec->ep_pkey_rec.pkey_tbl[0]) * used_blocks);
	if (p_ep_port_rec) {
		memcpy(&p_ep_port_rec->port_info, &p_port->p_physp->port_info,
		       sizeof(p_ep_port_rec->port_info));

		/* slvl tables vector initialization */
		status = cl_ptr_vector_init(&p_ep_port_rec->slvl_by_port, slvl_rec, 1);
		if (status != CL_SUCCESS) {
			/* handle failure !!! */
		}
		for (i = 0; i < slvl_rec; i++) {
			cl_ptr_vector_at(&p_port->p_physp->slvl_by_port, i, (void*)&p_slvl_tbl);
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
			p_port->p_physp->ext_port_info.link_speed_active & FDR10;
		p_ep_port_rec->ep_pkey_rec.max_pkeys =
			cl_ntoh16(p_port->p_node->node_info.partition_cap);
		p_ep_port_rec->ep_pkey_rec.used_blocks = used_blocks;
		for (block_index = 0; block_index < used_blocks;
		     block_index++) {
			pkey_tbl = osm_pkey_tbl_block_get(osm_physp_get_pkey_tbl(p_port->p_physp), block_index);
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

void ep_port_rec_delete(struct ep_port_rec *p_ep_port_rec)
{
	size_t i, num_slvl;
	if (!p_ep_port_rec)
		return;

	num_slvl = cl_ptr_vector_get_size(&p_ep_port_rec->slvl_by_port);
	for (i = 0; i < num_slvl; i++)
		free(cl_ptr_vector_get(&p_ep_port_rec->slvl_by_port, i));
	cl_ptr_vector_destroy(&p_ep_port_rec->slvl_by_port);
	free(p_ep_port_rec);
}
