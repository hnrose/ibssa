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

extern uint8_t first_time_subnet_up;

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
		cl_qmap_init(&p_ssa_db_diff->ep_lft_top_tbl);
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
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_lft_top_tbl,
				   ep_lft_top_rec_delete_pfn);

		cl_qmap_remove_all(&p_ssa_db_diff->ep_guid_to_lid_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_node_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_port_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_link_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_node_tbl_removed);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_port_tbl_removed);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_link_tbl_removed);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_lft_block_tbl);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_lft_top_tbl);
		free(p_ssa_db_diff);
	}
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
		p_ssa_db_diff->enable_quirks = p_current_db->enable_quirks;
		p_ssa_db_diff->allow_both_pkeys = p_current_db->allow_both_pkeys;

		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SUBNET_PREFIX;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SM_STATE;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_LMC;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SUBNET_TIMEOUT;
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
static void ssa_db_guid_to_lid_insert(OUT cl_qmap_t * p_map,
				      IN uint64_t key,
				      IN cl_map_item_t * p_item)
{
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec;

	p_guid_to_lid_rec = (struct ep_guid_to_lid_rec *)
			     malloc(sizeof(*p_guid_to_lid_rec));
	if (!p_guid_to_lid_rec) {
		/* handle failure - bad memory allocation */
	}
	ep_guid_to_lid_rec_copy(p_guid_to_lid_rec, (struct ep_guid_to_lid_rec *) p_item);
	cl_qmap_insert(p_map, key, &p_guid_to_lid_rec->map_item);
}

/** =========================================================================
 */
static int ssa_db_guid_to_lid_cmp(IN cl_map_item_t * p_item_old,
				  IN cl_map_item_t * p_item_new)
{
	struct ep_guid_to_lid_rec *p_rec_old =
			(struct ep_guid_to_lid_rec *) p_item_old;
	struct ep_guid_to_lid_rec *p_rec_new =
			(struct ep_guid_to_lid_rec *) p_item_new;
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
static void ssa_db_node_insert(OUT cl_qmap_t * p_map,
			       IN uint64_t key,
			       IN cl_map_item_t * p_item)
{
	struct ep_node_rec *p_node_rec;
	p_node_rec = (struct ep_node_rec *)
			     malloc(sizeof(*p_node_rec));
	if (!p_node_rec) {
		/* handle failure - bad memory allocation */
	}
	ep_node_rec_copy(p_node_rec, (struct ep_node_rec *) p_item);
	cl_qmap_insert(p_map, key, &p_node_rec->map_item);
}

/** =========================================================================
 */
static int ssa_db_node_cmp(IN cl_map_item_t * p_item_old,
			   IN cl_map_item_t * p_item_new)
{
	struct ep_node_rec *p_rec_old = (struct ep_node_rec *) p_item_old;
	struct ep_node_rec *p_rec_new = (struct ep_node_rec *) p_item_new;
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
static void ssa_db_port_insert(OUT cl_qmap_t * p_map,
			       IN uint64_t key,
			       IN cl_map_item_t * p_item)
{
	struct ep_port_rec *p_port_rec, *p_port_rec_old;
	uint16_t used_blocks;

	p_port_rec_old = (struct ep_port_rec *) p_item;
	used_blocks = p_port_rec_old->ep_pkey_rec.used_blocks;

	p_port_rec = (struct ep_port_rec *)
			malloc(sizeof(*p_port_rec) +
			       sizeof(p_port_rec->ep_pkey_rec.pkey_tbl[0]) *
			       used_blocks);
	if (!p_port_rec) {
		/* handle failure - bad memory allocation */
	}

	ep_port_rec_copy(p_port_rec, p_port_rec_old);
	cl_qmap_insert(p_map, key, &p_port_rec->map_item);
}

/** =========================================================================
 */
static int ssa_db_port_cmp(IN cl_map_item_t * const p_item_old,
			   IN cl_map_item_t * const p_item_new)
{
	struct ep_port_rec *p_rec_old = (struct ep_port_rec *) p_item_old;
	struct ep_port_rec *p_rec_new = (struct ep_port_rec *) p_item_new;
	int res = 0;

	/* TODO: remove magic number */
	if (memcmp(&p_rec_old->mtu_cap, &p_rec_new->mtu_cap, 5)) /* 5 uint8_t fields are taken from port_info */
		res = 1;
	if (p_rec_old->is_fdr10_active != p_rec_new->is_fdr10_active)
		res = 1;

	/* TODO: Add deep comparison of SLVL and PKEY */

	return res;
}

/** =========================================================================
 */
static void ssa_db_link_insert(OUT cl_qmap_t * p_map,
			       IN uint64_t key,
			       IN cl_map_item_t * p_item)
{
	struct ep_link_rec *p_link_rec;
	p_link_rec = (struct ep_link_rec *)
			     malloc(sizeof(*p_link_rec));
	if (!p_link_rec) {
		/* handle failure - bad memory allocation */
	}
	ep_link_rec_copy(p_link_rec, (struct ep_link_rec *) p_item);
	cl_qmap_insert(p_map, key, &p_link_rec->map_item);
}

/** =========================================================================
 */
static int ssa_db_link_cmp(IN cl_map_item_t * p_item_old,
			   IN cl_map_item_t * p_item_new)
{
	struct ep_link_rec *p_rec_old = (struct ep_link_rec *) p_item_old;
	struct ep_link_rec *p_rec_new = (struct ep_link_rec *) p_item_new;
	int res = 0;

	if (memcmp(&p_rec_old->link_rec, &p_rec_new->link_rec,
		   sizeof(p_rec_new->link_rec)))
		res = 1;

	return res;
}

/** =========================================================================
 */
static uint8_t ssa_db_diff_table_cmp(IN cl_qmap_t * p_map_old,
				     IN cl_qmap_t * p_map_new,
				     IN void (*qmap_insert_pfn)(cl_qmap_t *,
								uint64_t, cl_map_item_t *),
				     IN int (*cmp_pfn)(cl_map_item_t *, cl_map_item_t *),
				     OUT cl_qmap_t * p_map_added,
				     OUT cl_qmap_t * p_map_removed)
{
	cl_map_item_t *p_item_old, *p_item_new;
	uint64_t key_old, key_new;
	uint8_t dirty = 0;

	p_item_old = cl_qmap_head(p_map_old);
	p_item_new = cl_qmap_head(p_map_new);
	while (p_item_old != cl_qmap_end(p_map_old) && p_item_new != cl_qmap_end(p_map_new)) {
		key_old = cl_qmap_key(p_item_old);
		key_new = cl_qmap_key(p_item_new);
		if (key_old < key_new) {
			qmap_insert_pfn(p_map_removed, key_old, p_item_old);
			p_item_old = cl_qmap_next(p_item_old);
			dirty = 1;
		} else if (key_old > key_new) {
			qmap_insert_pfn(p_map_added, key_new, p_item_new);
			p_item_new = cl_qmap_next(p_item_new);
			dirty = 1;
		} else {
			if (cmp_pfn(p_item_old, p_item_new)) {
				qmap_insert_pfn(p_map_removed, key_old, p_item_old);
				qmap_insert_pfn(p_map_added, key_new, p_item_new);
				dirty = 1;
			}
			p_item_old = cl_qmap_next(p_item_old);
			p_item_new = cl_qmap_next(p_item_new);
		}
	}

	while (p_item_new != cl_qmap_end(p_map_new)) {
		key_new = cl_qmap_key(p_item_new);
		qmap_insert_pfn(p_map_added, key_new, p_item_new);
		p_item_new = cl_qmap_next(p_item_new);
		dirty = 1;
	}

	while (p_item_old != cl_qmap_end(p_map_old)) {
		key_old = cl_qmap_key(p_item_old);
		qmap_insert_pfn(p_map_removed, key_old, p_item_old);
		p_item_old = cl_qmap_next(p_item_old);
		dirty = 1;
	}

	return dirty;
}

/** =========================================================================
 */
static void ssa_db_diff_compare_subnet_tables(IN struct ssa_db * p_previous_db,
					     IN struct ssa_db * p_current_db,
					     OUT struct ssa_db_diff * const p_ssa_db_diff)
{
	uint8_t dirty = 0;
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
	dirty |= ssa_db_diff_table_cmp(&p_previous_db->ep_guid_to_lid_tbl,
				      &p_current_db->ep_guid_to_lid_tbl,
				      ssa_db_guid_to_lid_insert,
				      ssa_db_guid_to_lid_cmp,
				      &p_ssa_db_diff->ep_guid_to_lid_tbl_added,
				      &p_ssa_db_diff->ep_guid_to_lid_tbl_removed);

	dirty = dirty << 1;
	/*
	 * Comparing ep_node_rec records
	 */
	dirty |= ssa_db_diff_table_cmp(&p_previous_db->ep_node_tbl,
				      &p_current_db->ep_node_tbl,
				      ssa_db_node_insert,
				      ssa_db_node_cmp,
				      &p_ssa_db_diff->ep_node_tbl_added,
				      &p_ssa_db_diff->ep_node_tbl_removed);

	dirty = dirty << 1;
	/*
	 * Comparing ep_link_rec records
	 */
	dirty |= ssa_db_diff_table_cmp(&p_previous_db->ep_link_tbl,
				      &p_current_db->ep_link_tbl,
				      ssa_db_link_insert,
				      ssa_db_link_cmp,
				      &p_ssa_db_diff->ep_link_tbl_added,
				      &p_ssa_db_diff->ep_link_tbl_removed);

	dirty = dirty << 1;
	/*
	 * Comparing ep_port_rec records
	 */
	dirty |= ssa_db_diff_table_cmp(&p_previous_db->ep_port_tbl,
				      &p_current_db->ep_port_tbl,
				      ssa_db_port_insert,
				      ssa_db_port_cmp,
				      &p_ssa_db_diff->ep_port_tbl_added,
				      &p_ssa_db_diff->ep_port_tbl_removed);
	if (dirty)
		p_ssa_db_diff->dirty = 1;
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
		ssa_log(SSA_LOG_VERBOSE, "Port LID %u Port Num %u\n",
			(uint16_t) cl_qmap_key(&p_port_rec->map_item),
			(uint8_t) (cl_qmap_key(&p_port_rec->map_item) >> 16));
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
static void ssa_db_diff_dump_lft_top_rec(IN struct ssa_events * ssa,
					 IN cl_map_item_t * p_item)
{
	struct ep_lft_top_rec *p_lft_top_rec = (struct ep_lft_top_rec *) p_item;

	if (p_lft_top_rec)
		ssa_log(SSA_LOG_VERBOSE, "LID %u new LFT top %u\n",
			p_lft_top_rec->lid, p_lft_top_rec->lft_top);
}

/** =========================================================================
 */
static void ssa_db_diff_dump_lft_block_rec(IN struct ssa_events * ssa,
					   IN cl_map_item_t * p_item)
{
	struct ep_lft_block_rec *p_lft_block_rec = (struct ep_lft_block_rec *) p_item;

	if (p_lft_block_rec)
		ssa_log(SSA_LOG_VERBOSE, "LID %u LFT block # %u\n",
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
	ssa_log(ssa_log_level, "LFT top records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_db_diff_dump_qmap(&p_ssa_db_diff->ep_lft_top_tbl,
			      ssa, ssa_db_diff_dump_lft_top_rec);

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

/** =========================================================================
 */
static void ep_lft_block_qmap_copy(cl_qmap_t *p_dest_qmap, cl_qmap_t * p_src_qmap)
{
	struct ep_lft_block_rec *p_lft_block_rec, *p_lft_block_rec_next;
	struct ep_lft_block_rec *p_lft_block_rec_old, *p_lft_block_rec_new;

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
		p_lft_block_rec_old =
			(struct ep_lft_block_rec *) cl_qmap_insert(
						p_dest_qmap,
						cl_qmap_key(&p_lft_block_rec->map_item),
						&p_lft_block_rec_new->map_item);
		if (p_lft_block_rec_old != p_lft_block_rec_new) {
			/* in case of existing record with the same key */
			cl_qmap_remove(p_dest_qmap, cl_qmap_key(&p_lft_block_rec->map_item));
			ep_lft_block_rec_delete(p_lft_block_rec_old);
			cl_qmap_insert(p_dest_qmap, cl_qmap_key(&p_lft_block_rec->map_item),
				       &p_lft_block_rec_new->map_item);
		}
	}
}

/** =========================================================================
 */
static void ep_lft_top_qmap_copy(cl_qmap_t *p_dest_qmap, cl_qmap_t * p_src_qmap)
{
	struct ep_lft_top_rec *p_lft_top_rec, *p_lft_top_rec_next;
	struct ep_lft_top_rec *p_lft_top_rec_old, *p_lft_top_rec_new;

	p_lft_top_rec_next = (struct ep_lft_top_rec *) cl_qmap_head(p_src_qmap);
	while (p_lft_top_rec_next !=
	       (struct ep_lft_top_rec *) cl_qmap_end(p_src_qmap)) {
		p_lft_top_rec = p_lft_top_rec_next;
		p_lft_top_rec_next = (struct ep_lft_top_rec *)
				   cl_qmap_next(&p_lft_top_rec->map_item);
		p_lft_top_rec_new = (struct ep_lft_top_rec *)
				  malloc(sizeof(*p_lft_top_rec_new));
		if (!p_lft_top_rec_new) {
			/* handle failure - bad memory allocation */
		}
		ep_lft_top_rec_copy(p_lft_top_rec_new, p_lft_top_rec);
		p_lft_top_rec_old =
			(struct ep_lft_top_rec *) cl_qmap_insert(
						p_dest_qmap,
						cl_qmap_key(&p_lft_top_rec->map_item),
						&p_lft_top_rec_new->map_item);
		if(p_lft_top_rec_old != p_lft_top_rec_new) {
			/* in case of existing record with the same key */
			cl_qmap_remove(p_dest_qmap, cl_qmap_key(&p_lft_top_rec->map_item));
			ep_lft_top_rec_delete(p_lft_top_rec_old);
			cl_qmap_insert(p_dest_qmap, cl_qmap_key(&p_lft_top_rec->map_item),
				       &p_lft_top_rec_new->map_item);
		}
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
	    !ssa_db->p_current_db || !ssa_db->p_dump_db ||
	    !ssa_db->p_lft_db) {
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
	ssa_db_diff_compare_subnet_tables(ssa_db->p_previous_db,
					  ssa_db->p_current_db, p_ssa_db_diff);

	if (first_time_subnet_up) {
		ep_lft_block_qmap_copy(&p_ssa_db_diff->ep_lft_block_tbl, &ssa_db->p_lft_db->ep_db_lft_block_tbl);
		ep_lft_top_qmap_copy(&p_ssa_db_diff->ep_lft_top_tbl, &ssa_db->p_lft_db->ep_db_lft_top_tbl);
	} else {
		ep_lft_block_qmap_copy(&p_ssa_db_diff->ep_lft_block_tbl, &ssa_db->p_lft_db->ep_dump_lft_block_tbl);
		ep_lft_top_qmap_copy(&p_ssa_db_diff->ep_lft_top_tbl, &ssa_db->p_lft_db->ep_dump_lft_top_tbl);
		/* Apply LFT block / top changes on existing LFT database */
		ep_lft_block_qmap_copy(&ssa_db->p_lft_db->ep_db_lft_block_tbl, &ssa_db->p_lft_db->ep_dump_lft_block_tbl);
		ep_lft_top_qmap_copy(&ssa_db->p_lft_db->ep_db_lft_top_tbl, &ssa_db->p_lft_db->ep_dump_lft_top_tbl);
		/* Clear LFT dump data */
		ep_lft_block_rec_qmap_clear(&ssa_db->p_lft_db->ep_dump_lft_block_tbl);
		ep_lft_top_rec_qmap_clear(&ssa_db->p_lft_db->ep_dump_lft_top_tbl);
	}

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
