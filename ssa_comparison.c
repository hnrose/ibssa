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

#include <stdlib.h>
#include <ssa_database.h>
#include <ssa_comparison.h>

extern char *port_state_str[];

struct ssa_db_diff *ssa_db_diff_init()
{
	struct ssa_db_diff *p_ssa_db_diff;

	p_ssa_db_diff = (struct ssa_db_diff *) calloc(1, sizeof(*p_ssa_db_diff));
	if (p_ssa_db_diff) {
		cl_qmap_init(&p_ssa_db_diff->ep_guid_to_lid_tbl_added);
		cl_qmap_init(&p_ssa_db_diff->ep_node_tbl_added);
		cl_qmap_init(&p_ssa_db_diff->ep_port_tbl_added);
		cl_qmap_init(&p_ssa_db_diff->ep_link_tbl_added);
		cl_qmap_init(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed);
		cl_qmap_init(&p_ssa_db_diff->ep_node_tbl_removed);
		cl_qmap_init(&p_ssa_db_diff->ep_port_tbl_removed);
		cl_qmap_init(&p_ssa_db_diff->ep_link_tbl_removed);
		cl_qmap_init(&p_ssa_db_diff->ep_lft_block_tbl);
	}
	return p_ssa_db_diff;
}

/** =========================================================================
 */
void ssa_db_diff_destroy(struct ssa_db_diff * p_ssa_db_diff)
{
	if (p_ssa_db_diff) {
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_guid_to_lid_tbl_added,
				   ep_guid_to_lid_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed,
				   ep_guid_to_lid_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_node_tbl_added,
				   ep_node_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_node_tbl_removed,
				   ep_node_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_port_tbl_added,
				   ep_port_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_port_tbl_removed,
				   ep_port_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_link_tbl_added,
				   ep_link_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_link_tbl_removed,
				   ep_link_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_lft_block_tbl,
				   ep_lft_block_rec_delete_pfn);

		cl_qmap_remove_all(&p_ssa_db_diff->ep_guid_to_lid_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_node_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_port_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_link_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_node_tbl_removed);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_port_tbl_removed);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_link_tbl_removed);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_lft_block_tbl);
		free(p_ssa_db_diff);
	}
}

/** =========================================================================
 */
struct ep_lft_block_rec *ep_lft_block_rec_init(IN struct ep_lft_rec * p_lft_rec,
					       IN uint16_t lid,
					       IN uint16_t block)
{
	struct ep_lft_block_rec *p_lft_block_rec;

	p_lft_block_rec = (struct ep_lft_block_rec*) malloc(sizeof(*p_lft_block_rec));
	if (p_lft_block_rec) {
		p_lft_block_rec->block_num = block;
		p_lft_block_rec->lid = lid;
		memcpy(p_lft_block_rec->block,
		       p_lft_rec->lft + block * IB_SMP_DATA_SIZE,
		       IB_SMP_DATA_SIZE);
	}
	return p_lft_block_rec;
}

/** =========================================================================
 */
void ep_lft_block_rec_delete(struct ep_lft_block_rec * p_lft_block_rec)
{
	free(p_lft_block_rec);
}

/** =========================================================================
 */
void ep_lft_block_rec_delete_pfn(cl_map_item_t *p_map_item)
{
	struct ep_lft_block_rec *p_lft_block_rec;

	p_lft_block_rec = (struct ep_lft_block_rec *) p_map_item;
	ep_lft_block_rec_delete(p_lft_block_rec);
}

/** =========================================================================
 */
static void ssa_db_diff_compare_subnet_opts(IN struct ssa_db * p_previous_db,
					    IN struct ssa_db * p_current_db,
					    OUT struct ssa_db_diff * p_ssa_db_diff)
{
	uint8_t dirty = p_ssa_db_diff->dirty;

	if (!p_previous_db->initialized && p_current_db->initialized) {
		p_ssa_db_diff->subnet_prefix = p_current_db->subnet_prefix;
		p_ssa_db_diff->sm_state = p_current_db->sm_state;
		p_ssa_db_diff->lmc = p_current_db->lmc;
		p_ssa_db_diff->subnet_timeout = p_current_db->subnet_timeout;
		p_ssa_db_diff->fabric_mtu = p_current_db->fabric_mtu;
		p_ssa_db_diff->enable_quirks = p_current_db->enable_quirks;
		p_ssa_db_diff->allow_both_pkeys = p_current_db->allow_both_pkeys;

		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SUBNET_PREFIX;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SM_STATE;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_LMC;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SUBNET_TIMEOUT;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_FABRIC_MTU;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_ENABLE_QUIRKS;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_ALLOW_BOTH_PKEYS;

		dirty = 1;
		goto Exit;
	}

	if (p_previous_db->subnet_prefix != p_current_db->subnet_prefix) {
		p_ssa_db_diff->subnet_prefix = p_current_db->subnet_prefix;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SUBNET_PREFIX;
		dirty = 1;
	}
	if (p_previous_db->sm_state != p_current_db->sm_state) {
		p_ssa_db_diff->sm_state = p_current_db->sm_state;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SM_STATE;
		dirty = 1;
	}
	if (p_previous_db->lmc != p_current_db->lmc) {
		/* TODO: add error log message since the LMC is not supposed to change */
		p_ssa_db_diff->lmc = p_current_db->lmc;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_LMC;
		dirty = 1;
	}
	if (p_previous_db->subnet_timeout != p_current_db->subnet_timeout) {
		p_ssa_db_diff->subnet_timeout = p_current_db->subnet_timeout;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SUBNET_TIMEOUT;
		dirty = 1;
	}
	if (p_previous_db->fabric_mtu != p_current_db->fabric_mtu) {
		p_ssa_db_diff->fabric_mtu = p_current_db->fabric_mtu;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_FABRIC_MTU;
		dirty = 1;
	}
	if (p_previous_db->fabric_rate != p_current_db->fabric_rate) {
		p_ssa_db_diff->fabric_rate = p_current_db->fabric_rate;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_FABRIC_RATE;
		dirty = 1;
	}
	if (p_previous_db->enable_quirks != p_current_db->enable_quirks) {
		p_ssa_db_diff->enable_quirks = p_current_db->enable_quirks;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_ENABLE_QUIRKS;
		dirty = 1;
	}
	if (p_previous_db->allow_both_pkeys != p_current_db->allow_both_pkeys) {
		p_ssa_db_diff->allow_both_pkeys = p_current_db->allow_both_pkeys;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_ALLOW_BOTH_PKEYS;
		dirty = 1;
	}
Exit:
	p_ssa_db_diff->dirty = dirty;
}

/** =========================================================================
 */
static int ssa_db_guid_to_lid_comp(IN struct ep_guid_to_lid_rec * p_rec_old,
				   IN struct ep_guid_to_lid_rec * p_rec_new)
{
	int res = 0;

	if (p_rec_old->lid != p_rec_new->lid)
		res = 1;
	if (p_rec_old->lmc != p_rec_new->lmc)
		res = 1;
	if (p_rec_old->is_switch != p_rec_new->is_switch)
		res = 1;

	return res;
}

/** =========================================================================
 */
static int ssa_db_node_comp(IN struct ep_node_rec * p_rec_old,
			    IN struct ep_node_rec * p_rec_new)
{
	int res = 0;

	if (memcmp(&p_rec_old->node_info, &p_rec_new->node_info,
		   sizeof(p_rec_new->node_info)))
		res = 1;
	if (memcmp(&p_rec_old->node_desc, &p_rec_new->node_desc,
		   sizeof(p_rec_new->node_desc)))
		res = 1;
	if (p_rec_old->is_enhanced_sp0 != p_rec_new->is_enhanced_sp0)
		res = 1;

	return res;
}

/** =========================================================================
 */
static int ssa_db_port_comp(IN const struct ep_port_rec * const p_rec_old,
			    IN const struct ep_port_rec * const p_rec_new)
{
	int res = 0;

	if (memcmp(&p_rec_old->port_info, &p_rec_new->port_info,
		   sizeof(p_rec_new->port_info)))
		res = 1;
	if (p_rec_old->is_fdr10_active != p_rec_new->is_fdr10_active)
		res = 1;

	/* TODO: Add deep comparison of SLVL and PKEY */

	return res;
}

/** =========================================================================
 */
static uint8_t ssa_db_diff_compare_qmap(IN cl_qmap_t * p_qmap_previous,
					IN cl_qmap_t * p_qmap_current,
					IN void (*qmap_copy_pfn)(cl_qmap_t *, cl_qmap_t *),
					IN void (*qmap_delete_pfn)(cl_map_item_t *),
					OUT cl_qmap_t * p_qmap_added,
					OUT cl_qmap_t * p_qmap_removed)
{
	cl_qmap_t ep_rec_old, ep_rec_new;
	uint8_t dirty = 0;

	cl_qmap_init(&ep_rec_old);
	cl_qmap_init(&ep_rec_new);

	qmap_copy_pfn(&ep_rec_old, p_qmap_previous);
	qmap_copy_pfn(&ep_rec_new, p_qmap_current);

	cl_qmap_delta(&ep_rec_old, &ep_rec_new,
		      p_qmap_added, p_qmap_removed);
	if (cl_qmap_head(p_qmap_added) != cl_qmap_end(p_qmap_added))
		dirty = 1;
	if (cl_qmap_head(p_qmap_removed) != cl_qmap_end(p_qmap_removed))
		dirty = 1;

	ssa_qmap_apply_func(&ep_rec_old, qmap_delete_pfn);
	ssa_qmap_apply_func(&ep_rec_new, qmap_delete_pfn);

	return dirty;
}

/** =========================================================================
 */
static int ssa_db_link_comp(IN const struct ep_link_rec * const p_rec_old,
			    IN const struct ep_link_rec * const p_rec_new)
{
	int res = 0;

	if (memcmp(&p_rec_old->link_rec, &p_rec_new->link_rec,
		   sizeof(p_rec_new->link_rec)))
		res = 1;

	return res;
}

/** =========================================================================
 */
static void ssa_db_diff_compare_subnet_nodes(IN struct ssa_db * p_previous_db,
					     IN struct ssa_db * p_current_db,
					     OUT struct ssa_db_diff * const p_ssa_db_diff)
{
	struct ep_port_rec *p_port_rec, *p_port_rec_next;
	struct ep_port_rec *p_port_rec_new, *p_port_rec_old;
	struct ep_node_rec *p_node_rec, *p_node_rec_next;
	struct ep_node_rec *p_node_rec_new, *p_node_rec_old;
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec, *p_guid_to_lid_rec_next;
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec_new, *p_guid_to_lid_rec_old;
	struct ep_link_rec *p_link_rec, *p_link_rec_next;
	struct ep_link_rec *p_link_rec_new, *p_link_rec_old;
	uint64_t key;
	uint16_t used_blocks;
	uint8_t dirty = p_ssa_db_diff->dirty;

	/*
	 * Comparing ep_guid_to_lid_rec / ep_node_rec / ep_port_rec
	 * 	     ep_link_rec records
	 *
	 * For each record in previous SMDB version:
	 *
	 * 1. If the record is not present in current SMDB it will
	 *    be inserted to "removed" records.
	 *
	 * 2. If the record is present in current SMDB and not in
	 *    previous one than it will be added to "added" records.
	 *
	 * 3. If the record presents in both SMDB versions a
	 *    comparison between the versions will be done. In case
	 *    of at least 1 different value for the same field
	 *    the old record will be added to the "removed" records
	 *    and the new one will be added to "added" ones.
	 *
	 *    (when SMDB is updated using the ssa_db_diff
	 *    structure the "removed" records map has to applied first
	 *    and only afterwards the "added" records may be added,
	 *    for LFT records there is only single map for changed
	 *    blocks that need to be set)
	 */
	/*
	 * Comparing ep_guid_to_lid_rec records
	 */
	dirty = ssa_db_diff_compare_qmap(&p_previous_db->ep_guid_to_lid_tbl,
					 &p_current_db->ep_guid_to_lid_tbl,
					 ep_guid_to_lid_qmap_copy,
					 ep_guid_to_lid_rec_delete_pfn,
					 &p_ssa_db_diff->ep_guid_to_lid_tbl_added,
					 &p_ssa_db_diff->ep_guid_to_lid_tbl_removed);

	p_guid_to_lid_rec_next = (struct ep_guid_to_lid_rec *)
			cl_qmap_head(&p_previous_db->ep_guid_to_lid_tbl);
	while (p_guid_to_lid_rec_next != (struct ep_guid_to_lid_rec *)
			cl_qmap_end(&p_previous_db->ep_guid_to_lid_tbl)) {
		p_guid_to_lid_rec = p_guid_to_lid_rec_next;
		p_guid_to_lid_rec_next = (struct ep_guid_to_lid_rec *)
			cl_qmap_next(&p_guid_to_lid_rec->map_item);

		key = cl_qmap_key(&p_guid_to_lid_rec->map_item);
		/* checking if the item is already in added or removed sections */
		if (cl_qmap_get(&p_ssa_db_diff->ep_guid_to_lid_tbl_added, key)
		    != cl_qmap_end(&p_ssa_db_diff->ep_guid_to_lid_tbl_added))
			continue;
		if (cl_qmap_get(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed, key)
		    != cl_qmap_end(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed))
			continue;

		p_guid_to_lid_rec_new = (struct ep_guid_to_lid_rec *)
			cl_qmap_get(&p_current_db->ep_guid_to_lid_tbl, key);
		if (p_guid_to_lid_rec_new == (struct ep_guid_to_lid_rec *)
			cl_qmap_end(&p_current_db->ep_guid_to_lid_tbl)) {
			/* we are not supposed to get here - error occured */
		}

		p_guid_to_lid_rec_old = p_guid_to_lid_rec;
		/* comparing 2 ep_guid_to_lid_rec records with the same key */
		if (ssa_db_guid_to_lid_comp(p_guid_to_lid_rec_old,
					    p_guid_to_lid_rec_new)) {
			p_guid_to_lid_rec = (struct ep_guid_to_lid_rec *)
					     malloc(sizeof(*p_guid_to_lid_rec));
			if (!p_guid_to_lid_rec) {
				/* handle failure - bad memory allocation */
			}
			ep_guid_to_lid_rec_copy(p_guid_to_lid_rec, p_guid_to_lid_rec_old);
			cl_qmap_insert(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed,
				       key, &p_guid_to_lid_rec->map_item);
			p_guid_to_lid_rec = (struct ep_guid_to_lid_rec *)
					     malloc(sizeof(*p_guid_to_lid_rec));
			if (!p_guid_to_lid_rec) {
				/* handle failure - bad memory allocation */
			}
			ep_guid_to_lid_rec_copy(p_guid_to_lid_rec, p_guid_to_lid_rec_new);
			cl_qmap_insert(&p_ssa_db_diff->ep_guid_to_lid_tbl_added,
				       key, &p_guid_to_lid_rec->map_item);
			dirty = 1;
		}
		/* we get here if the there is no difference between 2 records */
	}

	/*
	 * Comparing ep_node_rec records
	 */
	dirty = ssa_db_diff_compare_qmap(&p_previous_db->ep_node_tbl,
					 &p_current_db->ep_node_tbl,
					 ep_node_qmap_copy,
					 ep_node_rec_delete_pfn,
					 &p_ssa_db_diff->ep_node_tbl_added,
					 &p_ssa_db_diff->ep_node_tbl_removed);

	p_node_rec_next = (struct ep_node_rec *)
			cl_qmap_head(&p_previous_db->ep_node_tbl);
	while (p_node_rec_next != (struct ep_node_rec *)
			cl_qmap_end(&p_previous_db->ep_node_tbl)) {
		p_node_rec = p_node_rec_next;
		p_node_rec_next = (struct ep_node_rec *)
			cl_qmap_next(&p_node_rec->map_item);

		key = cl_qmap_key(&p_node_rec->map_item);
		/* checking if the item is already in added or removed sections */
		if (cl_qmap_get(&p_ssa_db_diff->ep_node_tbl_added, key)
		    != cl_qmap_end(&p_ssa_db_diff->ep_node_tbl_added))
			continue;
		if (cl_qmap_get(&p_ssa_db_diff->ep_node_tbl_removed, key)
		    != cl_qmap_end(&p_ssa_db_diff->ep_node_tbl_removed))
			continue;

		p_node_rec_new = (struct ep_node_rec *)
			cl_qmap_get(&p_current_db->ep_node_tbl, key);
		if (p_node_rec_new == (struct ep_node_rec *)
			cl_qmap_end(&p_current_db->ep_node_tbl)) {
			/* we are not supposed to get here - error occured */
		}

		p_node_rec_old = p_node_rec;
		/* comparing 2 ep_node_rec records with the same key */
		if (ssa_db_node_comp(p_node_rec_old, p_node_rec_new)) {
			p_node_rec = (struct ep_node_rec *)
					     malloc(sizeof(*p_node_rec));
			if (!p_node_rec) {
				/* handle failure - bad memory allocation */
			}
			ep_node_rec_copy(p_node_rec, p_node_rec_old);
			cl_qmap_insert(&p_ssa_db_diff->ep_node_tbl_removed,
				       key, &p_node_rec->map_item);
			p_node_rec = (struct ep_node_rec *)
					     malloc(sizeof(*p_node_rec));
			if (!p_node_rec) {
				/* handle failure - bad memory allocation */
			}
			ep_node_rec_copy(p_node_rec, p_node_rec_new);
			cl_qmap_insert(&p_ssa_db_diff->ep_node_tbl_added,
				       key, &p_node_rec->map_item);
			dirty = 1;
		}
		/* we get here if the there is no difference between 2 records */
	}

	/*
	 * Comparing ep_link_rec records
	 */
	dirty = ssa_db_diff_compare_qmap(&p_previous_db->ep_link_tbl,
					 &p_current_db->ep_link_tbl,
					 ep_link_qmap_copy,
					 ep_link_rec_delete_pfn,
					 &p_ssa_db_diff->ep_link_tbl_added,
					 &p_ssa_db_diff->ep_link_tbl_removed);

	p_link_rec_next = (struct ep_link_rec *)
			cl_qmap_head(&p_previous_db->ep_link_tbl);
	while (p_link_rec_next != (struct ep_link_rec *)
			cl_qmap_end(&p_previous_db->ep_link_tbl)) {
		p_link_rec = p_link_rec_next;
		p_link_rec_next = (struct ep_link_rec *)
			cl_qmap_next(&p_link_rec->map_item);

		key = cl_qmap_key(&p_link_rec->map_item);
		/* checking if the item is already in added or removed sections */
		if (cl_qmap_get(&p_ssa_db_diff->ep_link_tbl_added, key)
		    != cl_qmap_end(&p_ssa_db_diff->ep_link_tbl_added))
			continue;
		if (cl_qmap_get(&p_ssa_db_diff->ep_link_tbl_removed, key)
		    != cl_qmap_end(&p_ssa_db_diff->ep_link_tbl_removed))
			continue;

		p_link_rec_new = (struct ep_link_rec *)
			cl_qmap_get(&p_current_db->ep_link_tbl, key);
		if (p_link_rec_new == (struct ep_link_rec *)
			cl_qmap_end(&p_current_db->ep_link_tbl)) {
			/* we are not supposed to get here - error occured */
		}

		p_link_rec_old = p_link_rec;
		/* comparing 2 ep_link_rec records with the same key */
		if (ssa_db_link_comp(p_link_rec_old, p_link_rec_new)) {
			p_link_rec = (struct ep_link_rec *)
					     malloc(sizeof(*p_link_rec));
			if (!p_link_rec) {
				/* handle failure - bad memory allocation */
			}
			ep_link_rec_copy(p_link_rec, p_link_rec_old);
			cl_qmap_insert(&p_ssa_db_diff->ep_link_tbl_removed,
				       key, &p_link_rec->map_item);
			p_link_rec = (struct ep_link_rec *)
					     malloc(sizeof(*p_link_rec));
			if (!p_link_rec) {
				/* handle failure - bad memory allocation */
			}
			ep_link_rec_copy(p_link_rec, p_link_rec_new);
			cl_qmap_insert(&p_ssa_db_diff->ep_link_tbl_added,
				       key, &p_link_rec->map_item);
			dirty = 1;
		}
		/* we get here if the there is no difference between 2 records */
	}

	/*
	 * Comparing ep_port_rec records
	 */
	dirty = ssa_db_diff_compare_qmap(&p_previous_db->ep_port_tbl,
					 &p_current_db->ep_port_tbl,
					 ep_port_qmap_copy,
					 ep_port_rec_delete_pfn,
					 &p_ssa_db_diff->ep_port_tbl_added,
					 &p_ssa_db_diff->ep_port_tbl_removed);

	p_port_rec_next = (struct ep_port_rec *)
			cl_qmap_head(&p_previous_db->ep_port_tbl);
	while (p_port_rec_next != (struct ep_port_rec *)
			cl_qmap_end(&p_previous_db->ep_port_tbl)) {
		p_port_rec = p_port_rec_next;
		p_port_rec_next = (struct ep_port_rec *)
			cl_qmap_next(&p_port_rec->map_item);

		key = cl_qmap_key(&p_port_rec->map_item);
		/* checking if the item is already in added or removed sections */
		if (cl_qmap_get(&p_ssa_db_diff->ep_port_tbl_added, key)
		    != cl_qmap_end(&p_ssa_db_diff->ep_port_tbl_added))
			continue;
		if (cl_qmap_get(&p_ssa_db_diff->ep_port_tbl_removed, key)
		    != cl_qmap_end(&p_ssa_db_diff->ep_port_tbl_removed))
			continue;

		p_port_rec_new = (struct ep_port_rec *)
			cl_qmap_get(&p_current_db->ep_port_tbl, key);
		if (p_port_rec_new == (struct ep_port_rec *)
			cl_qmap_end(&p_current_db->ep_port_tbl)) {
			/* we are not supposed to get here - error occured */
		}

		p_port_rec_old = p_port_rec;
		/* comparing 2 ep_node_rec records with the same key */
		if (ssa_db_port_comp(p_port_rec_old, p_port_rec_new)) {
			used_blocks = p_port_rec_old->ep_pkey_rec.used_blocks;
			p_port_rec = (struct ep_port_rec *)
					malloc(sizeof(*p_port_rec) +
					       sizeof(p_port_rec->ep_pkey_rec.pkey_tbl[0]) *
					       used_blocks);
			if (!p_port_rec) {
				/* handle failure - bad memory allocation */
			}
			ep_port_rec_copy(p_port_rec, p_port_rec_old);
			cl_qmap_insert(&p_ssa_db_diff->ep_port_tbl_removed,
				       key, &p_port_rec->map_item);

			used_blocks = p_port_rec_new->ep_pkey_rec.used_blocks;
			p_port_rec = (struct ep_port_rec *)
					malloc(sizeof(*p_port_rec) +
					       sizeof(p_port_rec->ep_pkey_rec.pkey_tbl[0]) *
					       used_blocks);
			if (!p_port_rec) {
				/* handle failure - bad memory allocation */
			}
			ep_port_rec_copy(p_port_rec, p_port_rec_new);
			cl_qmap_insert(&p_ssa_db_diff->ep_port_tbl_added,
				       key, &p_port_rec->map_item);
			dirty = 1;
		}
		/* we get here if the there is no difference between 2 records */
	}

	p_ssa_db_diff->dirty = dirty;
}

/** =========================================================================
 */
inline uint64_t ep_lft_block_rec_gen_key(uint16_t lid, uint16_t block_num)
{
	uint64_t key;
	key = (uint64_t) lid;
	key |= (uint64_t) block_num << 16;
	return key;
}

/** =========================================================================
 */
static void ssa_db_diff_compare_lfts(IN struct ssa_db * p_current_db,
				     IN struct ssa_db * p_dump_db,
				     OUT struct ssa_db_diff * const p_ssa_db_diff)
{
	struct ep_lft_rec *p_lft_rec, *p_lft_rec_next;
	struct ep_lft_rec *p_lft_rec_old;
	struct ep_lft_block_rec *p_lft_block_rec;
	uint64_t key;
	uint16_t i, lid, max_block_old, max_block_new;
	uint8_t dirty = p_ssa_db_diff->dirty;

	/*
	 * Comparing ep_lft_rec records - in this case the comparison
	 * is done only on LFTs that were changed and only modified
	 * blocks are stored in ssa_db_diff
	 */
	p_lft_rec_next = (struct ep_lft_rec *)
			cl_qmap_head(&p_dump_db->ep_lft_tbl);
	while (p_lft_rec_next != (struct ep_lft_rec *)
			cl_qmap_end(&p_dump_db->ep_lft_tbl)) {
		p_lft_rec = p_lft_rec_next;
		p_lft_rec_next = (struct ep_lft_rec *)
			cl_qmap_next(&p_lft_rec->map_item);
		lid = (uint16_t) cl_qmap_key(&p_lft_rec->map_item);
		p_lft_rec_old = (struct ep_lft_rec *)
					cl_qmap_get(&p_current_db->ep_lft_tbl,
					(uint64_t) lid);
		if (p_lft_rec_old != (struct ep_lft_rec *)
			cl_qmap_end(&p_current_db->ep_lft_tbl)) {
			/* In case of LFT record that was already present
			 * in previous version - perform coparison (by blocks)
			 */
			/* assumption: switch lft size can only grow */
			max_block_old = p_lft_rec_old->lft_size / IB_SMP_DATA_SIZE;
			max_block_new = p_lft_rec->lft_size / IB_SMP_DATA_SIZE;

			for (i = 0; i < max_block_old && i < max_block_new; i++) {
				if (memcmp(p_lft_rec_old->lft + i * IB_SMP_DATA_SIZE,
					   p_lft_rec->lft + i * IB_SMP_DATA_SIZE,
					   IB_SMP_DATA_SIZE)) {
					/* In case of different LFT blocks */
					p_lft_block_rec =
						ep_lft_block_rec_init(p_lft_rec, lid, i);
					key = ep_lft_block_rec_gen_key(lid, i);
					cl_qmap_insert(&p_ssa_db_diff->ep_lft_block_tbl,
						       key, &p_lft_block_rec->map_item);
				}
			}

			for ( ; i < max_block_new; i++) {
				p_lft_block_rec =
					ep_lft_block_rec_init(p_lft_rec, lid, i);
				key = ep_lft_block_rec_gen_key(lid, i);
				cl_qmap_insert(&p_ssa_db_diff->ep_lft_block_tbl,
					       key, &p_lft_block_rec->map_item);
			}

		} else {
			/* In case of new LFT */
			max_block_new = p_lft_rec->lft_size / IB_SMP_DATA_SIZE;
			for (i = 0; i < max_block_new; i++) {
				p_lft_block_rec =
					ep_lft_block_rec_init(p_lft_rec, lid, i);
				key = ep_lft_block_rec_gen_key(lid, i);
				cl_qmap_insert(&p_ssa_db_diff->ep_lft_block_tbl,
					       key, &p_lft_block_rec->map_item);
			}
		}
		dirty = 1;
	}

	p_ssa_db_diff->dirty = dirty;
}

/** =========================================================================
 */
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
static void ssa_db_diff_dump_fabric_params(IN struct ssa_events * ssa,
					   IN struct ssa_db_diff * p_ssa_db_diff)
{
	uint8_t is_changed = 0;

	ssa_log(SSA_LOG_VERBOSE, "Fabric parameters:\n");

	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_SUBNET_PREFIX) {
		ssa_log(SSA_LOG_VERBOSE, "Subnet Prefix: 0x%" PRIx64 "\n",
			p_ssa_db_diff->subnet_prefix);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_SM_STATE) {
		ssa_log(SSA_LOG_VERBOSE, "SM state: %d\n",
			p_ssa_db_diff->sm_state);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_LMC) {
		ssa_log(SSA_LOG_VERBOSE, "LMC: %u\n",
			p_ssa_db_diff->lmc);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_SUBNET_TIMEOUT) {
		ssa_log(SSA_LOG_VERBOSE, "Subnet timeout: %u\n",
			p_ssa_db_diff->subnet_timeout);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_FABRIC_MTU) {
		ssa_log(SSA_LOG_VERBOSE, "Fabric MTU: %d\n",
			p_ssa_db_diff->fabric_mtu);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_FABRIC_RATE) {
		ssa_log(SSA_LOG_VERBOSE, "Fabric rate: %d\n",
			p_ssa_db_diff->fabric_rate);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_ENABLE_QUIRKS) {
		ssa_log(SSA_LOG_VERBOSE, "Quirks %sabled\n",
			p_ssa_db_diff->enable_quirks ? "en" : "dis");
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_ALLOW_BOTH_PKEYS) {
		ssa_log(SSA_LOG_VERBOSE, "Both pkeys %sabled\n",
			p_ssa_db_diff->allow_both_pkeys ? "en" : "dis");
		is_changed = 1;
	}

	if (!is_changed)
		ssa_log(SSA_LOG_VERBOSE, "No changes\n");
}

/** =========================================================================
 */
static void ssa_db_diff_dump_node_rec(IN struct ssa_events * ssa,
				      IN cl_map_item_t * p_item)
{
	struct ep_node_rec *p_node_rec = (struct ep_node_rec *) p_item;
	char buffer[64];

	if (p_node_rec) {
		if (p_node_rec->node_info.node_type == IB_NODE_TYPE_SWITCH)
			sprintf(buffer, " with %s Switch Port 0\n",
				p_node_rec->is_enhanced_sp0 ? "Enhanced" : "Base");
		else
			sprintf(buffer, "\n");
		ssa_log(SSA_LOG_VERBOSE, "Node GUID 0x%" PRIx64 " Type %d%s",
			cl_ntoh64(p_node_rec->node_info.node_guid),
			p_node_rec->node_info.node_type,
			buffer);
	}
}

/** =========================================================================
 */
static void ssa_db_diff_dump_guid_to_lid_rec(IN struct ssa_events * ssa,
					     IN cl_map_item_t * p_item)
{
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec = (struct ep_guid_to_lid_rec *) p_item;

	if (p_guid_to_lid_rec) {
		ssa_log(SSA_LOG_VERBOSE, "Port GUID 0x%" PRIx64 " LID %u LMC %u is_switch %d\n",
			cl_ntoh64(cl_map_key((cl_map_iterator_t) p_guid_to_lid_rec)),
			p_guid_to_lid_rec->lid,
			p_guid_to_lid_rec->lmc,
			p_guid_to_lid_rec->is_switch);
	}
}

/** =========================================================================
 */
static void ssa_db_diff_dump_port_rec(IN struct ssa_events * ssa,
					  IN cl_map_item_t * p_item)
{
	struct ep_port_rec *p_port_rec = (struct ep_port_rec *) p_item;
	const ib_pkey_table_t *block;
	ib_net16_t pkey;
	uint16_t block_index, pkey_idx;

	if (p_port_rec) {
		ssa_log(SSA_LOG_VERBOSE, "-------------------\n");
		ssa_log(SSA_LOG_VERBOSE, "Port LID %u Port Num %u LMC %u Port state %d (%s)\n",
			(uint16_t) cl_qmap_key(&p_port_rec->map_item),
			(uint8_t) (cl_qmap_key(&p_port_rec->map_item) >> 16),
			ib_port_info_get_lmc(&p_port_rec->port_info),
			ib_port_info_get_port_state(&p_port_rec->port_info),
			(ib_port_info_get_port_state(&p_port_rec->port_info) < 5
			 ? port_state_str[ib_port_info_get_port_state
			 (&p_port_rec->port_info)] : "???"));
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

/** =========================================================================
 */
static void ssa_db_diff_dump_lft_block_rec(IN struct ssa_events * ssa,
					   IN cl_map_item_t * p_item)
{
	struct ep_lft_block_rec *p_lft_block_rec = (struct ep_lft_block_rec *) p_item;

	if (p_lft_block_rec)
		ssa_log(SSA_LOG_VERBOSE, "LFT Block Record: LID %u block # %u\n",
			p_lft_block_rec->lid, p_lft_block_rec->block_num);
}

/** =========================================================================
 */
static void ssa_db_diff_dump_link_rec(IN struct ssa_events * ssa,
				      IN cl_map_item_t * p_item)
{
	struct ep_link_rec *p_link_rec = (struct ep_link_rec *) p_item;

	if (p_link_rec)
		ssa_log(SSA_LOG_VERBOSE, "From LID %u port %u to LID %u port %u\n",
			cl_ntoh16(p_link_rec->link_rec.from_lid),
			p_link_rec->link_rec.from_port_num,
			cl_ntoh16(p_link_rec->link_rec.to_lid),
			p_link_rec->link_rec.to_port_num);
}

/** =========================================================================
 */
static void ssa_db_diff_dump_qmap(IN cl_qmap_t * p_qmap,
				  IN struct ssa_events * ssa,
				  IN void (*pfn_dump)(struct ssa_events *, cl_map_item_t *))
{
	cl_map_item_t *p_map_item, *p_map_item_next;
	uint8_t is_changed = 0;

        p_map_item_next = cl_qmap_head(p_qmap);
        while (p_map_item_next != cl_qmap_end(p_qmap)) {
                p_map_item = p_map_item_next;
                p_map_item_next = cl_qmap_next(p_map_item);
                pfn_dump(ssa, p_map_item);
		is_changed = 1;
	}

	if (!is_changed)
		ssa_log(SSA_LOG_VERBOSE, "No changes\n");
}

/** =========================================================================
 */
static void ssa_db_diff_dump(IN struct ssa_events * ssa,
			     IN struct ssa_db_diff * p_ssa_db_diff)
{
	int ssa_log_level = SSA_LOG_VERBOSE;

	if (!ssa || !p_ssa_db_diff)
		return;

	ssa_log(ssa_log_level, "Dumping SMDB changes\n");
	ssa_log(ssa_log_level, "===================================\n");
	ssa_db_diff_dump_fabric_params(ssa, p_ssa_db_diff);

	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "NODE records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "Added records:\n");
	ssa_db_diff_dump_qmap(&p_ssa_db_diff->ep_node_tbl_added,
			      ssa, ssa_db_diff_dump_node_rec);
	ssa_log(ssa_log_level, "Removed records:\n");
	ssa_db_diff_dump_qmap(&p_ssa_db_diff->ep_node_tbl_removed, ssa,
			      ssa_db_diff_dump_node_rec);

	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "GUID to LID records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "Added records:\n");
	ssa_db_diff_dump_qmap(&p_ssa_db_diff->ep_guid_to_lid_tbl_added,
			      ssa, ssa_db_diff_dump_guid_to_lid_rec);
	ssa_log(ssa_log_level, "Removed records:\n");
	ssa_db_diff_dump_qmap(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed,
			      ssa, ssa_db_diff_dump_guid_to_lid_rec);

	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "PORT records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "Added records:\n");
	ssa_db_diff_dump_qmap(&p_ssa_db_diff->ep_port_tbl_added,
			      ssa, ssa_db_diff_dump_port_rec);
	ssa_log(ssa_log_level, "Removed records:\n");
	ssa_db_diff_dump_qmap(&p_ssa_db_diff->ep_port_tbl_removed,
			      ssa, ssa_db_diff_dump_port_rec);

	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "LFT block records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_db_diff_dump_qmap(&p_ssa_db_diff->ep_lft_block_tbl,
			      ssa, ssa_db_diff_dump_lft_block_rec);

	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "Link Records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "Added records:\n");
	ssa_db_diff_dump_qmap(&p_ssa_db_diff->ep_link_tbl_added,
			      ssa, ssa_db_diff_dump_link_rec);
	ssa_log(ssa_log_level, "Removed records:\n");
	ssa_db_diff_dump_qmap(&p_ssa_db_diff->ep_link_tbl_removed,
			      ssa, ssa_db_diff_dump_link_rec);
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "===================================\n");
}
#endif


/* TODO: make qmap copy functions generic */
/** =========================================================================
 */
void ep_guid_to_lid_qmap_copy(cl_qmap_t * p_dest_qmap, cl_qmap_t * p_src_qmap)
{
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec, *p_guid_to_lid_rec_next;
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec_new;

	p_guid_to_lid_rec_next = (struct ep_guid_to_lid_rec *) cl_qmap_head(p_src_qmap);
	while (p_guid_to_lid_rec_next !=
	       (struct ep_guid_to_lid_rec *) cl_qmap_end(p_src_qmap)) {
		p_guid_to_lid_rec = p_guid_to_lid_rec_next;
		p_guid_to_lid_rec_next = (struct ep_guid_to_lid_rec *)
					  cl_qmap_next(&p_guid_to_lid_rec->map_item);
		p_guid_to_lid_rec_new = (struct ep_guid_to_lid_rec *)
					 malloc(sizeof(*p_guid_to_lid_rec_new));
		if (!p_guid_to_lid_rec_new) {
			/* handle failure - bad memory allocation */
		}
		ep_guid_to_lid_rec_copy(p_guid_to_lid_rec_new, p_guid_to_lid_rec);
		cl_qmap_insert(p_dest_qmap,
			       cl_qmap_key(&p_guid_to_lid_rec->map_item),
			       &p_guid_to_lid_rec_new->map_item);
	}
}

/** =========================================================================
 */
void ep_node_qmap_copy(cl_qmap_t *p_dest_qmap, cl_qmap_t * p_src_qmap)
{
	struct ep_node_rec *p_node_rec, *p_node_rec_next;
	struct ep_node_rec *p_node_rec_new;

	p_node_rec_next = (struct ep_node_rec *) cl_qmap_head(p_src_qmap);
	while (p_node_rec_next !=
	       (struct ep_node_rec *) cl_qmap_end(p_src_qmap)) {
		p_node_rec = p_node_rec_next;
		p_node_rec_next = (struct ep_node_rec *)
				   cl_qmap_next(&p_node_rec->map_item);
		p_node_rec_new = (struct ep_node_rec *)
				  malloc(sizeof(*p_node_rec_new));
		if (!p_node_rec_new) {
			/* handle failure - bad memory allocation */
		}
		ep_node_rec_copy(p_node_rec_new, p_node_rec);
		cl_qmap_insert(p_dest_qmap,
			       cl_qmap_key(&p_node_rec->map_item),
			       &p_node_rec_new->map_item);
	}
}

/** =========================================================================
 */
void ep_port_qmap_copy(cl_qmap_t *p_dest_qmap, cl_qmap_t * p_src_qmap)
{
	struct ep_port_rec *p_port_rec, *p_port_rec_next;
	struct ep_port_rec *p_port_rec_new;
	uint16_t used_blocks;

	p_port_rec_next = (struct ep_port_rec *) cl_qmap_head(p_src_qmap);
	while (p_port_rec_next !=
	       (struct ep_port_rec *) cl_qmap_end(p_src_qmap)) {
		p_port_rec = p_port_rec_next;
		p_port_rec_next = (struct ep_port_rec *)
				   cl_qmap_next(&p_port_rec->map_item);
		used_blocks = p_port_rec->ep_pkey_rec.used_blocks;
		p_port_rec_new = (struct ep_port_rec *)
				  malloc(sizeof(*p_port_rec_new) +
					 sizeof(p_port_rec_new->ep_pkey_rec.pkey_tbl[0]) *
					 used_blocks);
		if (!p_port_rec_new) {
			/* handle failure - bad memory allocation */
		}
		ep_port_rec_copy(p_port_rec_new, p_port_rec);
		cl_qmap_insert(p_dest_qmap,
			       cl_qmap_key(&p_port_rec->map_item),
			       &p_port_rec_new->map_item);
	}
}

/** =========================================================================
 */
void ep_lft_block_rec_copy(OUT struct ep_lft_block_rec * p_dest_rec,
			   IN struct ep_lft_block_rec * p_src_rec)
{
	p_dest_rec->lid = p_src_rec->lid;
	p_dest_rec->block_num = p_src_rec->block_num;
	memcpy(p_dest_rec->block, p_src_rec->block, sizeof(p_dest_rec->block));
}

/** =========================================================================
 */
void ep_lft_block_qmap_copy(cl_qmap_t *p_dest_qmap, cl_qmap_t * p_src_qmap)
{
	struct ep_lft_block_rec *p_lft_block_rec, *p_lft_block_rec_next;
	struct ep_lft_block_rec *p_lft_block_rec_new;

	p_lft_block_rec_next = (struct ep_lft_block_rec *) cl_qmap_head(p_src_qmap);
	while (p_lft_block_rec_next !=
	       (struct ep_lft_block_rec *) cl_qmap_end(p_src_qmap)) {
		p_lft_block_rec = p_lft_block_rec_next;
		p_lft_block_rec_next = (struct ep_lft_block_rec *)
				   cl_qmap_next(&p_lft_block_rec->map_item);
		p_lft_block_rec_new = (struct ep_lft_block_rec *)
				  malloc(sizeof(*p_lft_block_rec_new));
		if (!p_lft_block_rec_new) {
			/* handle failure - bad memory allocation */
		}
		ep_lft_block_rec_copy(p_lft_block_rec_new, p_lft_block_rec);
		cl_qmap_insert(p_dest_qmap,
			       cl_qmap_key(&p_lft_block_rec->map_item),
			       &p_lft_block_rec_new->map_item);
	}
}

/** =========================================================================
 */
void ep_lft_qmap_copy(cl_qmap_t *p_dest_qmap, cl_qmap_t * p_src_qmap)
{
	struct ep_lft_rec *p_next_lft, *p_lft, *p_tmp_lft, *p_old_lft;

	p_next_lft= (struct ep_lft_rec *)cl_qmap_head(p_src_qmap);
	while (p_next_lft !=
	       (struct ep_lft_rec *)cl_qmap_end(p_src_qmap)) {
		p_lft = p_next_lft;
		p_next_lft = (struct ep_lft_rec *) cl_qmap_next(&p_lft->map_item);
		p_tmp_lft = (struct ep_lft_rec *) malloc(sizeof(*p_tmp_lft));
		if (!p_tmp_lft) {
			/* handle failure - bad memory allocation */
		}
		p_tmp_lft->lft = malloc(p_lft->lft_size);
		if (!p_tmp_lft->lft) {
			/* handle failure - bad memory allocation */
		}
		ep_lft_rec_copy(p_tmp_lft, p_lft);

		/* check if LFT record for the same LID already exists */
		p_old_lft = (struct ep_lft_rec *)
				cl_qmap_get(p_dest_qmap,
					    cl_qmap_key(&p_lft->map_item));
		if (p_old_lft != (struct ep_lft_rec *)
					cl_qmap_end(p_dest_qmap)) {
			cl_qmap_remove(p_dest_qmap,
				       cl_qmap_key(&p_lft->map_item));
			ep_lft_rec_delete(p_old_lft);
		}
		cl_qmap_insert(p_dest_qmap,
			       cl_qmap_key(&p_lft->map_item),
			       &p_tmp_lft->map_item);
	}
}

/** =========================================================================
 */
void ep_link_qmap_copy(cl_qmap_t *p_dest_qmap, cl_qmap_t * p_src_qmap)
{
	struct ep_link_rec *p_link_rec, *p_link_rec_next;
	struct ep_link_rec *p_link_rec_new;

	p_link_rec_next = (struct ep_link_rec *) cl_qmap_head(p_src_qmap);
	while (p_link_rec_next !=
	       (struct ep_link_rec *) cl_qmap_end(p_src_qmap)) {
		p_link_rec = p_link_rec_next;
		p_link_rec_next = (struct ep_link_rec *)
				   cl_qmap_next(&p_link_rec->map_item);
		p_link_rec_new = (struct ep_link_rec *)
				  malloc(sizeof(*p_link_rec_new));
		if (!p_link_rec_new) {
			/* handle failure - bad memory allocation */
		}
		ep_link_rec_copy(p_link_rec_new, p_link_rec);
		cl_qmap_insert(p_dest_qmap,
			       cl_qmap_key(&p_link_rec->map_item),
			       &p_link_rec_new->map_item);
	}
}

/** =========================================================================
 */
struct ssa_db_diff *ssa_db_compare(IN struct ssa_events * ssa,
				   IN struct ssa_database * ssa_db)
{
	struct ssa_db_diff *p_ssa_db_diff = NULL;

	ssa_log(SSA_LOG_VERBOSE, "[\n");

	if (!ssa_db || !ssa_db->p_previous_db ||
	    !ssa_db->p_current_db || !ssa_db->p_dump_db) {
		/* bad arguments - error handling */
		ssa_log(SSA_LOG_ALL, "SMDB Comparison: bad arguments\n");
		goto Exit;
	}

	p_ssa_db_diff = ssa_db_diff_init();
	if (!p_ssa_db_diff) {
		/* error handling */
		ssa_log(SSA_LOG_ALL, "SMDB Comparison: bad diff struct initialization\n");
		goto Exit;
	}

	ssa_db_diff_compare_subnet_opts(ssa_db->p_previous_db,
					ssa_db->p_current_db, p_ssa_db_diff);
	ssa_db_diff_compare_subnet_nodes(ssa_db->p_previous_db,
					 ssa_db->p_current_db, p_ssa_db_diff);

	ssa_db_diff_compare_lfts(ssa_db->p_current_db,
				 ssa_db->p_dump_db, p_ssa_db_diff);

	/* update all records in current_db and remove all from dump_db */
	ep_lft_qmap_copy(&ssa_db->p_current_db->ep_lft_tbl,
			 &ssa_db->p_dump_db->ep_lft_tbl);
	ssa_qmap_apply_func(&ssa_db->p_dump_db->ep_lft_tbl, ep_lft_rec_delete_pfn);
	cl_qmap_remove_all(&ssa_db->p_dump_db->ep_lft_tbl);

	if (!p_ssa_db_diff->dirty) {
                ssa_log(SSA_LOG_VERBOSE, "SMDB was not changed\n");
                ssa_db_diff_destroy(p_ssa_db_diff);
                p_ssa_db_diff = NULL;
                goto Exit;
        }
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
	ssa_db_diff_dump(ssa, p_ssa_db_diff);
#endif
Exit:
	ssa_log(SSA_LOG_VERBOSE, "]\n");

	return p_ssa_db_diff;
}
