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
		cl_qmap_init(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed);
		cl_qmap_init(&p_ssa_db_diff->ep_node_tbl_removed);
		cl_qmap_init(&p_ssa_db_diff->ep_port_tbl_removed);
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

		cl_qmap_remove_all(&p_ssa_db_diff->ep_guid_to_lid_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_node_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_port_tbl_added);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_node_tbl_removed);
		cl_qmap_remove_all(&p_ssa_db_diff->ep_port_tbl_removed);
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
static void ssa_db_diff_compare_subnet_nodes(IN struct ssa_db * p_previous_db,
					     IN struct ssa_db * p_current_db,
					     OUT struct ssa_db_diff * const p_ssa_db_diff)
{
	struct ep_port_rec *p_port_rec;
	struct ep_port_rec *p_port_rec_new, *p_port_rec_old;
	struct ep_node_rec *p_node_rec, *p_node_rec_next;
	struct ep_node_rec *p_node_rec_new, *p_node_rec_old;
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec, *p_guid_to_lid_rec_next;
	struct ep_guid_to_lid_rec *p_guid_to_lid_rec_new, *p_guid_to_lid_rec_old;
	cl_qmap_t ep_rec_old, ep_rec_new;
	uint64_t key;
	uint16_t lid, used_blocks;
	uint8_t dirty = p_ssa_db_diff->dirty;

	/*
	 * Comparing ep_guid_to_lid_rec / ep_node_rec / ep_port_rec records
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
	 *    and only afterwards the "added" records may be added)
	 */
	/*
	 * Comparing ep_guid_to_lid_rec records
	 */
	cl_qmap_init(&ep_rec_old);
	cl_qmap_init(&ep_rec_new);
	ep_guid_to_lid_qmap_copy(&ep_rec_old, &p_previous_db->ep_guid_to_lid_tbl);
	ep_guid_to_lid_qmap_copy(&ep_rec_new, &p_current_db->ep_guid_to_lid_tbl);

	cl_qmap_delta(&ep_rec_old,
		      &ep_rec_new,
		      &p_ssa_db_diff->ep_guid_to_lid_tbl_added,
		      &p_ssa_db_diff->ep_guid_to_lid_tbl_removed);
	if (cl_qmap_head(&p_ssa_db_diff->ep_guid_to_lid_tbl_added)
			 != cl_qmap_end(&p_ssa_db_diff->ep_guid_to_lid_tbl_added))
		dirty = 1;
	if (cl_qmap_head(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed)
			 != cl_qmap_end(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed))
		dirty = 1;

	ssa_qmap_apply_func(&ep_rec_old, ep_guid_to_lid_rec_delete_pfn);
	ssa_qmap_apply_func(&ep_rec_new, ep_guid_to_lid_rec_delete_pfn);

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
	cl_qmap_init(&ep_rec_old);
	cl_qmap_init(&ep_rec_new);
	ep_node_qmap_copy(&ep_rec_old, &p_previous_db->ep_node_tbl);
	ep_node_qmap_copy(&ep_rec_new, &p_current_db->ep_node_tbl);

	cl_qmap_delta(&ep_rec_old,
		      &ep_rec_new,
		      &p_ssa_db_diff->ep_node_tbl_added,
		      &p_ssa_db_diff->ep_node_tbl_removed);
	if (cl_qmap_head(&p_ssa_db_diff->ep_node_tbl_added)
			 != cl_qmap_end(&p_ssa_db_diff->ep_node_tbl_added))
		dirty = 1;
	if (cl_qmap_head(&p_ssa_db_diff->ep_node_tbl_removed)
			 != cl_qmap_end(&p_ssa_db_diff->ep_node_tbl_removed))
		dirty = 1;

	ssa_qmap_apply_func(&ep_rec_old, ep_node_rec_delete_pfn);
	ssa_qmap_apply_func(&ep_rec_new, ep_node_rec_delete_pfn);

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
	 * Comparing ep_port_rec records
	 */
	/* new port records that had not exist before */
	for (lid = 1;
	     lid < (uint16_t) cl_ptr_vector_get_size(&p_current_db->ep_port_tbl);
	     lid++) {		/* increment LID by LMC ??? */
		p_port_rec_new = (struct ep_port_rec *) cl_ptr_vector_get(&p_current_db->ep_port_tbl, lid);
		if (p_port_rec_new) {
			p_port_rec_old = (struct ep_port_rec *) cl_ptr_vector_get(&p_previous_db->ep_port_tbl, lid);
			if (!p_port_rec_old) {
				used_blocks = p_port_rec_new->ep_pkey_rec.used_blocks;
				p_port_rec = (struct ep_port_rec *) malloc(sizeof(*p_port_rec) +
					      sizeof(p_port_rec_new->ep_pkey_rec.pkey_tbl[0]) * used_blocks);
				if (!p_port_rec) {
					/* handle failure - bad memory allocation */
				}
				ep_port_rec_copy(p_port_rec, p_port_rec_new);
				cl_qmap_insert(&p_ssa_db_diff->ep_port_tbl_added,
					       lid, &p_port_rec->map_item);
			}
		}
	}

	/* old port records that no longer exist */
	for (lid = 1;
	     lid < (uint16_t) cl_ptr_vector_get_size(&p_previous_db->ep_port_tbl);
	     lid++) {		/* increment LID by LMC ??? */
		p_port_rec_old = (struct ep_port_rec *) cl_ptr_vector_get(&p_previous_db->ep_port_tbl, lid);
		if (p_port_rec_old) {
			p_port_rec_new = (struct ep_port_rec *) cl_ptr_vector_get(&p_current_db->ep_port_tbl, lid);
			if (!p_port_rec_new) {
				used_blocks = p_port_rec_old->ep_pkey_rec.used_blocks;
				p_port_rec = (struct ep_port_rec *) malloc(sizeof(*p_port_rec) +
					      sizeof(p_port_rec_old->ep_pkey_rec.pkey_tbl[0]) * used_blocks);
				if (!p_port_rec) {
					/* handle failure - bad memory allocation */
				}
				ep_port_rec_copy(p_port_rec, p_port_rec_old);
				cl_qmap_insert(&p_ssa_db_diff->ep_port_tbl_removed,
					       lid, &p_port_rec->map_item);
			}
		}
	}

	/* comparing new Vs. old port records */
	for (lid = 1;
	     lid < (uint16_t) cl_ptr_vector_get_size(&p_previous_db->ep_port_tbl);
	     lid++) {		/* increment LID by LMC ??? */
		p_port_rec_old = (struct ep_port_rec *) cl_ptr_vector_get(&p_previous_db->ep_port_tbl, lid);
		if (p_port_rec_old) {
			p_port_rec_new = (struct ep_port_rec *) cl_ptr_vector_get(&p_current_db->ep_port_tbl, lid);
			if (p_port_rec_new) {
				if (ssa_db_port_comp(p_port_rec_old, p_port_rec_new)) {
					used_blocks = p_port_rec_old->ep_pkey_rec.used_blocks;
					p_port_rec = (struct ep_port_rec *) malloc(sizeof(*p_port_rec) +
						      sizeof(p_port_rec_old->ep_pkey_rec.pkey_tbl[0]) * used_blocks);
					if (!p_port_rec) {
						/* handle failure - bad memory allocation */
					}
					ep_port_rec_copy(p_port_rec, p_port_rec_old);
					cl_qmap_insert(&p_ssa_db_diff->ep_port_tbl_removed,
						       lid, &p_port_rec->map_item);
					used_blocks = p_port_rec_new->ep_pkey_rec.used_blocks;
					p_port_rec = (struct ep_port_rec *) malloc(sizeof(*p_port_rec) +
						      sizeof(p_port_rec_new->ep_pkey_rec.pkey_tbl[0]) * used_blocks);
					if (!p_port_rec) {
						/* handle failure - bad memory allocation */
					}
					ep_port_rec_copy(p_port_rec, p_port_rec_new);
					cl_qmap_insert(&p_ssa_db_diff->ep_port_tbl_added,
						       lid, &p_port_rec->map_item);
				}
			}
		}
	}

	p_ssa_db_diff->dirty = dirty;
}

/** =========================================================================
 */
static void ssa_db_diff_dump(IN struct ssa_events * ssa,
		      IN struct ssa_db_diff * p_ssa_db_diff)
{
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
	struct ep_port_rec *p_port_rec, *p_next_port_rec;
	struct ep_node_rec *p_node, *p_next_node;
	struct ep_guid_to_lid_rec *p_port, *p_next_port;
	const ib_pkey_table_t *block;
	char buffer[64];
	ib_net16_t pkey;
	uint16_t block_index, pkey_idx;
	uint8_t is_changed = 0;

	if (!ssa || !p_ssa_db_diff)
		return;

	fprintf_log(ssa->log_file, "Dumping SMDB changes\n");
	fprintf_log(ssa->log_file, "===================================\n");
	fprintf_log(ssa->log_file, "Fabric parameters:\n");

	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_SUBNET_PREFIX) {
		sprintf(buffer, "Subnet Prefix: 0x%" PRIx64 "\n",
			p_ssa_db_diff->subnet_prefix);
		fprintf_log(ssa->log_file, buffer);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_SM_STATE) {
		sprintf(buffer, "SM state: %d\n",
			p_ssa_db_diff->sm_state);
		fprintf_log(ssa->log_file, buffer);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_LMC) {
		sprintf(buffer, "LMC: %u\n",
			p_ssa_db_diff->lmc);
		fprintf_log(ssa->log_file, buffer);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_SUBNET_TIMEOUT) {
		sprintf(buffer, "Subnet timeout: %u\n",
			p_ssa_db_diff->subnet_timeout);
		fprintf_log(ssa->log_file, buffer);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_FABRIC_MTU) {
		sprintf(buffer, "Fabric MTU: %d\n",
			p_ssa_db_diff->fabric_mtu);
		fprintf_log(ssa->log_file, buffer);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_FABRIC_RATE) {
		sprintf(buffer, "Fabric rate: %d\n",
			p_ssa_db_diff->fabric_rate);
		fprintf_log(ssa->log_file, buffer);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_ENABLE_QUIRKS) {
		sprintf(buffer, "Quirks %sabled\n",
			p_ssa_db_diff->enable_quirks ? "en" : "dis");
		fprintf_log(ssa->log_file, buffer);
		is_changed = 1;
	}
	if (p_ssa_db_diff->change_mask & SSA_DB_CHANGEMASK_ALLOW_BOTH_PKEYS) {
		sprintf(buffer, "Both pkeys %sabled\n",
			p_ssa_db_diff->allow_both_pkeys ? "en" : "dis");
		fprintf_log(ssa->log_file, buffer);
		is_changed = 1;
	}

	if (!is_changed) {
		fprintf_log(ssa->log_file, "No changes\n");
		is_changed = 0;
	}

	/* TODO: make all maps dumps generic and use it in validate_dump_db as well */
	fprintf_log(ssa->log_file, "NODE records:\n");
	fprintf_log(ssa->log_file, "-----------------------------------\n");
	fprintf_log(ssa->log_file, "Added records:\n");
	p_next_node = (struct ep_node_rec *) cl_qmap_head(&p_ssa_db_diff->ep_node_tbl_added);
	while (p_next_node !=
	       (struct ep_node_rec *) cl_qmap_end(&p_ssa_db_diff->ep_node_tbl_added)) {
		p_node = p_next_node;
		p_next_node = (struct ep_node_rec *) cl_qmap_next(&p_node->map_item);
		sprintf(buffer, "Node GUID 0x%" PRIx64 " Type %d%s",
			cl_ntoh64(p_node->node_info.node_guid),
			p_node->node_info.node_type,
			(p_node->node_info.node_type == IB_NODE_TYPE_SWITCH) ? " " : "\n");
		fprintf_log(ssa->log_file, buffer);
		if (p_node->node_info.node_type == IB_NODE_TYPE_SWITCH)
			fprintf(ssa->log_file, "with %s Switch Port 0\n",
				p_node->is_enhanced_sp0 ? "Enhanced" : "Base");
		is_changed = 1;
	}
	if (!is_changed) {
		fprintf_log(ssa->log_file, "No changes\n");
		is_changed = 0;
	}
	fprintf_log(ssa->log_file, "Removed records:\n");
	p_next_node = (struct ep_node_rec *) cl_qmap_head(&p_ssa_db_diff->ep_node_tbl_removed);
	while (p_next_node !=
	       (struct ep_node_rec *) cl_qmap_end(&p_ssa_db_diff->ep_node_tbl_removed)) {
		p_node = p_next_node;
		p_next_node = (struct ep_node_rec *) cl_qmap_next(&p_node->map_item);
		sprintf(buffer, "Node GUID 0x%" PRIx64 " Type %d%s",
			cl_ntoh64(p_node->node_info.node_guid),
			p_node->node_info.node_type,
			(p_node->node_info.node_type == IB_NODE_TYPE_SWITCH) ? " " : "\n");
		fprintf_log(ssa->log_file, buffer);
		if (p_node->node_info.node_type == IB_NODE_TYPE_SWITCH)
			fprintf(ssa->log_file, "with %s Switch Port 0\n",
				p_node->is_enhanced_sp0 ? "Enhanced" : "Base");
		is_changed = 1;
	}
	if (!is_changed) {
		fprintf_log(ssa->log_file, "No changes\n");
		is_changed = 0;
	}
	fprintf_log(ssa->log_file, "-----------------------------------\n");

	fprintf_log(ssa->log_file, "GUID to LID records:\n");
	fprintf_log(ssa->log_file, "-----------------------------------\n");
	fprintf_log(ssa->log_file, "Added records:\n");
	p_next_port = (struct ep_guid_to_lid_rec *) cl_qmap_head(&p_ssa_db_diff->ep_guid_to_lid_tbl_added);
	while (p_next_port !=
	       (struct ep_guid_to_lid_rec *) cl_qmap_end(&p_ssa_db_diff->ep_guid_to_lid_tbl_added)) {
		p_port = p_next_port;
		p_next_port = (struct ep_guid_to_lid_rec *) cl_qmap_next(&p_port->map_item);
		sprintf(buffer, "Port GUID 0x%" PRIx64 " LID %u LMC %u is_switch %d\n",
			cl_ntoh64(cl_map_key((cl_map_iterator_t) p_port)),
			p_port->lid, p_port->lmc, p_port->is_switch);
		fprintf_log(ssa->log_file, buffer);
		is_changed = 1;
	}
	if (!is_changed) {
		fprintf_log(ssa->log_file, "No changes\n");
		is_changed = 0;
	}
	fprintf_log(ssa->log_file, "Removed records:\n");
	p_next_port = (struct ep_guid_to_lid_rec *) cl_qmap_head(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed);
	while (p_next_port !=
	       (struct ep_guid_to_lid_rec *) cl_qmap_end(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed)) {
		p_port = p_next_port;
		p_next_port = (struct ep_guid_to_lid_rec *) cl_qmap_next(&p_port->map_item);
		sprintf(buffer, "Port GUID 0x%" PRIx64 " LID %u LMC %u is_switch %d\n",
			cl_ntoh64(cl_map_key((cl_map_iterator_t) p_port)),
			p_port->lid, p_port->lmc, p_port->is_switch);
		fprintf_log(ssa->log_file, buffer);
		is_changed = 1;
	}
	if (!is_changed) {
		fprintf_log(ssa->log_file, "No changes\n");
		is_changed = 0;
	}
	fprintf_log(ssa->log_file, "-----------------------------------\n");

	fprintf_log(ssa->log_file, "PORT records:\n");
	fprintf_log(ssa->log_file, "-----------------------------------\n");
	fprintf_log(ssa->log_file, "Added records:\n");
	p_next_port_rec = (struct ep_port_rec *) cl_qmap_head(&p_ssa_db_diff->ep_port_tbl_added);
	while (p_next_port_rec !=
	       (struct ep_port_rec *) cl_qmap_end(&p_ssa_db_diff->ep_port_tbl_added)) {
		p_port_rec = p_next_port_rec;
		p_next_port_rec = (struct ep_port_rec *) cl_qmap_next(&p_port_rec->map_item);
		fprintf_log(ssa->log_file, "-------------------\n");
		sprintf(buffer, "Port LID %u LMC %u Port state %d (%s)\n",
			cl_ntoh16(p_port_rec->port_info.base_lid),
			ib_port_info_get_lmc(&p_port_rec->port_info),
			ib_port_info_get_port_state(&p_port_rec->port_info),
			(ib_port_info_get_port_state(&p_port_rec->port_info) < 5
			 ? port_state_str[ib_port_info_get_port_state
			 (&p_port_rec->port_info)] : "???"));
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
		is_changed = 1;
	}
	if (!is_changed) {
		fprintf_log(ssa->log_file, "No changes\n");
		is_changed = 0;
	}
	fprintf_log(ssa->log_file, "Removed records:\n");
	p_next_port_rec = (struct ep_port_rec *) cl_qmap_head(&p_ssa_db_diff->ep_port_tbl_removed);
	while (p_next_port_rec !=
	       (struct ep_port_rec *) cl_qmap_end(&p_ssa_db_diff->ep_port_tbl_removed)) {
		p_port_rec = p_next_port_rec;
		p_next_port_rec = (struct ep_port_rec *) cl_qmap_next(&p_port_rec->map_item);
		fprintf_log(ssa->log_file, "-------------------\n");
		sprintf(buffer, "Port LID %u LMC %u Port state %d (%s)\n",
			cl_ntoh16(p_port_rec->port_info.base_lid),
			ib_port_info_get_lmc(&p_port_rec->port_info),
			ib_port_info_get_port_state(&p_port_rec->port_info),
			(ib_port_info_get_port_state(&p_port_rec->port_info) < 5
			 ? port_state_str[ib_port_info_get_port_state
			 (&p_port_rec->port_info)] : "???"));
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
		is_changed = 1;
	}
	if (!is_changed) {
		fprintf_log(ssa->log_file, "No changes\n");
		is_changed = 0;
	}
	fprintf_log(ssa->log_file, "-----------------------------------\n");
	fprintf_log(ssa->log_file, "===================================\n");
#endif
}


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
struct ssa_db_diff *ssa_db_compare(IN struct ssa_events * ssa,
				   IN struct ssa_db * p_previous_db,
				   IN struct ssa_db * p_current_db)
{
	char buffer[64];
	struct ssa_db_diff *p_ssa_db_diff = NULL;

	sprintf(buffer, "Start SMDB comparison\n");
	fprintf_log(ssa->log_file, buffer);

	if (!p_previous_db || !p_current_db) {
		/* bad arguments - error handling */
		sprintf(buffer, "SMDB Comparison: bad arguments\n");
		fprintf_log(ssa->log_file, buffer);
		goto Exit;
	}

	p_ssa_db_diff = ssa_db_diff_init();
	if (!p_ssa_db_diff) {
		/* error handling */
		sprintf(buffer, "SMDB Comparison: bad diff struct initialization\n");
		fprintf_log(ssa->log_file, buffer);
		goto Exit;
	}

	ssa_db_diff_compare_subnet_opts(p_previous_db, p_current_db, p_ssa_db_diff);
	ssa_db_diff_compare_subnet_nodes(p_previous_db, p_current_db, p_ssa_db_diff);

	if (!p_ssa_db_diff->dirty) {
		sprintf(buffer, "SMDB was not changed\n");
		fprintf_log(ssa->log_file, buffer);
		ssa_db_diff_destroy(p_ssa_db_diff);
		p_ssa_db_diff = NULL;
		goto Exit;
	}

	ssa_db_diff_dump(ssa, p_ssa_db_diff);
Exit:
	sprintf(buffer, "Finished SMDB comparison\n");
        fprintf_log(ssa->log_file, buffer);

	return p_ssa_db_diff;
}