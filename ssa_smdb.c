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

#include <ssa_smdb.h>
#include <asm/byteorder.h>

static const struct db_table_def def_tbl[] = {
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_SUBNET_OPTS, 0 },
		"SUBNET OPTS", __constant_htonl(sizeof(struct ep_subnet_opts_tbl_rec)), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_SUBNET_OPTS_FIELD_DEF, 0 },
		"SUBNET OPTS fields", __constant_htonl(sizeof(struct db_field_def)), __constant_htonl(SSA_TABLE_ID_SUBNET_OPTS) },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_GUID_TO_LID, 0 },
		"GUID to LID", __constant_htonl(sizeof(struct ep_guid_to_lid_tbl_rec)), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, 0 },
		"GUID to LID fields", __constant_htonl(sizeof(struct db_field_def)), __constant_htonl(SSA_TABLE_ID_GUID_TO_LID) },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_NODE, 0 },
		"NODE", __constant_htonl(sizeof(struct ep_node_tbl_rec)), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, 0 },
		"NODE fields", __constant_htonl(sizeof(struct db_field_def)), __constant_htonl(SSA_TABLE_ID_NODE) },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_LINK, 0 },
		"LINK", __constant_htonl(sizeof(struct ep_link_tbl_rec)), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, 0 },
		"LINK fields", __constant_htonl(sizeof(struct db_field_def)), __constant_htonl(SSA_TABLE_ID_LINK) },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_PORT, 0 },
		"PORT", __constant_htonl(sizeof(struct ep_port_tbl_rec)), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, 0 },
		"PORT fields", __constant_htonl(sizeof(struct db_field_def)), __constant_htonl(SSA_TABLE_ID_PORT) },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_PKEY, 0 },
		"PKEY", __constant_htonl(DB_VARIABLE_SIZE), __constant_htonl(SSA_TABLE_ID_PORT) },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_LFT_TOP, 0 },
		"LFT TOP", __constant_htonl(sizeof(struct ep_lft_top_tbl_rec)), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_LFT_TOP_FIELD_DEF, 0 },
		"LFT TOP fields", __constant_htonl(sizeof(struct db_field_def)), __constant_htonl(SSA_TABLE_ID_LFT_TOP) },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_LFT_BLOCK, 0 },
		"LFT BLOCK", __constant_htonl(sizeof(struct ep_lft_block_tbl_rec)), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, 0 },
		"LFT BLOCK fields", __constant_htonl(sizeof(struct db_field_def)), __constant_htonl(SSA_TABLE_ID_LFT_BLOCK) },
	{ 0 }
};

static const struct db_dataset dataset_tbl[] = {
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_SUBNET_OPTS, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_GUID_TO_LID, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_NODE, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LINK, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_PORT, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_PKEY, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LFT_TOP, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LFT_BLOCK, 0 }, 0, 0, 0, 0 },
	{ 0 }
};

static const struct db_dataset field_dataset_tbl[] = {
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_SUBNET_OPTS_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_PKEY_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LFT_TOP_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 0 }
};

static const struct db_field_def field_tbl[] = {
	{ 1, 0, DBF_TYPE_NET64, 0, { 0, SSA_TABLE_ID_SUBNET_OPTS_FIELD_DEF, SSA_FIELD_ID_SUBNET_OPTS_CHANGE_MASK }, "change_mask", __constant_htonl(64), 0 },
	{ 1, 0, DBF_TYPE_NET64, 0, { 0, SSA_TABLE_ID_SUBNET_OPTS_FIELD_DEF, SSA_FIELD_ID_SUBNET_OPTS_SUBNET_PREFIX }, "subnet_prefix", __constant_htonl(64), __constant_htonl(64) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_SUBNET_OPTS_FIELD_DEF, SSA_FIELD_ID_SUBNET_OPTS_SM_STATE }, "sm_state", __constant_htonl(8), __constant_htonl(128) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_SUBNET_OPTS_FIELD_DEF, SSA_FIELD_ID_SUBNET_OPTS_LMC }, "lmc", __constant_htonl(8), __constant_htonl(136) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_SUBNET_OPTS_FIELD_DEF, SSA_FIELD_ID_SUBNET_OPTS_SUBNET_TIMEOUT }, "subnet_timeout", __constant_htonl(8), __constant_htonl(144) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_SUBNET_OPTS_FIELD_DEF, SSA_FIELD_ID_SUBNET_OPTS_ALLOW_BOTH_PKEYS }, "allow_both_pkeys", __constant_htonl(8), __constant_htonl(152) },
	{ 1, 0, DBF_TYPE_NET64, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, SSA_FIELD_ID_GUID_TO_LID_GUID }, "guid", __constant_htonl(64), 0 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, SSA_FIELD_ID_GUID_TO_LID_LID }, "lid", __constant_htonl(16), __constant_htonl(64) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, SSA_FIELD_ID_GUID_TO_LID_LMC }, "lmc", __constant_htonl(8), __constant_htonl(80) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, SSA_FIELD_ID_GUID_TO_LID_IS_SWITCH }, "is_switch", __constant_htonl(8), __constant_htonl(88) },
	{ 1, 0, DBF_TYPE_NET64, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, SSA_FIELD_ID_NODE_NODE_GUID }, "node_guid", __constant_htonl(64), 0 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, SSA_FIELD_ID_NODE_IS_ENHANCED_SP0 }, "is_enhanced_sp0", __constant_htonl(8), __constant_htonl(64) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, SSA_FIELD_ID_NODE_NODE_TYPE }, "node_type", __constant_htonl(8), __constant_htonl(72) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, SSA_FIELD_ID_NODE_IS_ENHANCED_SP0 }, "description", __constant_htonl(8 * IB_NODE_DESCRIPTION_SIZE), __constant_htonl(80) },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, SSA_FIELD_ID_LINK_FROM_LID }, "from_lid", __constant_htonl(16), 0 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, SSA_FIELD_ID_LINK_TO_LID }, "to_lid", __constant_htonl(16), __constant_htonl(16) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, SSA_FIELD_ID_LINK_FROM_PORT_NUM }, "from_port_num", __constant_htonl(8), __constant_htonl(32) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, SSA_FIELD_ID_LINK_TO_PORT_NUM }, "to_port_num", __constant_htonl(8), __constant_htonl(40) },
	{ 1, 0, DBF_TYPE_NET64, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_PKEY_TBL_OFFSET }, "pkey_tbl_offset", __constant_htonl(64), 0 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_PKEY_TBL_SIZE }, "pkey_tbl_size", __constant_htonl(16), __constant_htonl(64) },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_PORT_LID }, "port_lid", __constant_htonl(16), __constant_htonl(80) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_PORT_NUM }, "port_num", __constant_htonl(8), __constant_htonl(96) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_NEIGHBOR_MTU }, "neighbor_mtu", __constant_htonl(8), __constant_htonl(104) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_RATE }, "rate", __constant_htonl(8), __constant_htonl(112) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_VL_ENFORCE }, "vl_enforce", __constant_htonl(8), __constant_htonl(120) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_IS_FDR10_ACTIVE }, "is_fdr10_active", __constant_htonl(8), __constant_htonl(128) },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LFT_TOP_FIELD_DEF, SSA_FIELD_ID_LFT_TOP_LID }, "lid", __constant_htonl(16), 0 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LFT_TOP_FIELD_DEF, SSA_FIELD_ID_LFT_TOP_LFT_TOP }, "lft_top", __constant_htonl(16), __constant_htonl(16) },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, SSA_FIELD_ID_LFT_BLOCK_LID }, "lid", __constant_htonl(16), 0 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, SSA_FIELD_ID_LFT_BLOCK_BLOCK_NUM }, "block_num", __constant_htonl(16), __constant_htonl(16) },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, SSA_FIELD_ID_LFT_BLOCK_BLOCK }, "block", __constant_htonl(8 * IB_SMP_DATA_SIZE), __constant_htonl(32) },
	{ 0 }
};

/** =========================================================================
 */
struct ssa_db *ssa_db_smdb_init(uint64_t guid_to_lid_num_recs,
				uint64_t node_num_recs,
				uint64_t link_num_recs,
				uint64_t port_num_recs,
				uint64_t pkey_num_recs,
				uint64_t lft_top_num_recs,
				uint64_t lft_block_num_recs)
{
	struct ssa_db *p_ssa_db;
	uint64_t num_recs_arr[SSA_TABLE_ID_MAX];
	uint64_t num_field_recs_arr[SSA_TABLE_ID_MAX];
	size_t recs_size_arr[SSA_TABLE_ID_MAX];

	num_recs_arr[SSA_TABLE_ID_SUBNET_OPTS] = 1; /* subnet options - single record */
	num_recs_arr[SSA_TABLE_ID_GUID_TO_LID] = guid_to_lid_num_recs;
	num_recs_arr[SSA_TABLE_ID_NODE] = node_num_recs;
	num_recs_arr[SSA_TABLE_ID_LINK] = link_num_recs;
	num_recs_arr[SSA_TABLE_ID_PORT] = port_num_recs;
	num_recs_arr[SSA_TABLE_ID_PKEY] = pkey_num_recs;
	num_recs_arr[SSA_TABLE_ID_LFT_TOP] = lft_top_num_recs;
	num_recs_arr[SSA_TABLE_ID_LFT_BLOCK] = lft_block_num_recs;

	recs_size_arr[SSA_TABLE_ID_SUBNET_OPTS] = sizeof(struct ep_subnet_opts_tbl_rec);
	recs_size_arr[SSA_TABLE_ID_GUID_TO_LID] = sizeof(struct ep_guid_to_lid_tbl_rec);
	recs_size_arr[SSA_TABLE_ID_NODE] = sizeof(struct ep_node_tbl_rec);
	recs_size_arr[SSA_TABLE_ID_LINK] = sizeof(struct ep_link_tbl_rec);
	recs_size_arr[SSA_TABLE_ID_PORT] = sizeof(struct ep_port_tbl_rec);
	recs_size_arr[SSA_TABLE_ID_PKEY] = sizeof(uint16_t);
	recs_size_arr[SSA_TABLE_ID_LFT_TOP] = sizeof(struct ep_lft_top_tbl_rec);
	recs_size_arr[SSA_TABLE_ID_LFT_BLOCK] = sizeof(struct ep_lft_block_tbl_rec);

	num_field_recs_arr[SSA_TABLE_ID_SUBNET_OPTS] = SSA_FIELD_ID_SUBNET_OPTS_MAX;
	num_field_recs_arr[SSA_TABLE_ID_GUID_TO_LID] = SSA_FIELD_ID_GUID_TO_LID_MAX;
	num_field_recs_arr[SSA_TABLE_ID_NODE] = SSA_FIELD_ID_NODE_MAX;
	num_field_recs_arr[SSA_TABLE_ID_LINK] = SSA_FIELD_ID_LINK_MAX;
	num_field_recs_arr[SSA_TABLE_ID_PORT] = SSA_FIELD_ID_PORT_MAX;
	num_field_recs_arr[SSA_TABLE_ID_PKEY] = DB_VARIABLE_SIZE; /* variabled size records */
	num_field_recs_arr[SSA_TABLE_ID_LFT_TOP] = SSA_FIELD_ID_LFT_TOP_MAX;
	num_field_recs_arr[SSA_TABLE_ID_LFT_BLOCK] = SSA_FIELD_ID_LFT_BLOCK_MAX;

	p_ssa_db = ssa_db_create(num_recs_arr, recs_size_arr, num_field_recs_arr, SSA_TABLE_ID_MAX);

	ssa_db_init(p_ssa_db, "SMDB", 12 /*just some db_id */, def_tbl, dataset_tbl,
		    field_dataset_tbl, field_tbl);

	return p_ssa_db;
}

/** =========================================================================
 */
void ssa_db_smdb_destroy(struct ssa_db * p_smdb)
{
	ssa_db_destroy(p_smdb);
}

/** =========================================================================
 */
void ep_subnet_opts_tbl_rec_init(osm_subn_t * p_subn,
				 struct ep_subnet_opts_tbl_rec * p_rec)
{
	p_rec->change_mask = 0;
	p_rec->subnet_prefix = p_subn->opt.subnet_prefix;
	p_rec->sm_state = p_subn->sm_state;
	p_rec->lmc = p_subn->opt.lmc;
	p_rec->subnet_timeout = p_subn->opt.subnet_timeout;
	p_rec->allow_both_pkeys = (uint8_t) p_subn->opt.allow_both_pkeys;

	memset(&p_rec->pad, 0, sizeof(p_rec->pad));
}

/** =========================================================================
 */
void ep_guid_to_lid_tbl_rec_init(osm_port_t *p_port,
				 struct ep_guid_to_lid_tbl_rec *p_rec)
{
	p_rec->guid = osm_physp_get_port_guid(p_port->p_physp);
	p_rec->lid = osm_physp_get_base_lid(p_port->p_physp);
	p_rec->lmc = osm_physp_get_lmc(p_port->p_physp);
	p_rec->is_switch = (osm_node_get_type(p_port->p_node) == IB_NODE_TYPE_SWITCH);

	memset(&p_rec->pad, 0, sizeof(p_rec->pad));
}

/** =========================================================================
 */
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

/** =========================================================================
 */
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

/** =========================================================================
 */
void ep_port_tbl_rec_init(osm_physp_t *p_physp, struct ep_port_tbl_rec *p_rec)
{
	const ib_port_info_t *p_pi;
	const osm_physp_t *p_physp0;
	uint8_t is_fdr10_active;
	uint8_t is_switch;

	if (osm_node_get_type(p_physp->p_node) == IB_NODE_TYPE_SWITCH &&
	    osm_physp_get_port_num(p_physp) > 0) {
		/* for SW external ports, port 0 Capability Mask is used  */
		p_physp0 = osm_node_get_physp_ptr((osm_node_t *)p_physp->p_node, 0);
		p_pi = &p_physp0->port_info;
	} else {
		p_pi = &p_physp->port_info;
	}

	is_fdr10_active = ((p_physp->ext_port_info.link_speed_active & FDR10) ? 0xff : 0) &
					  SSA_DB_PORT_IS_FDR10_ACTIVE_MASK;
	is_switch = ((osm_node_get_type(p_physp->p_node) == IB_NODE_TYPE_SWITCH) ? 0xff : 0) &
					  SSA_DB_PORT_IS_SWITCH_MASK;

	p_rec->pkey_tbl_offset		= 0;
	p_rec->pkey_tbl_size		= 0;
	p_rec->port_lid			= osm_physp_get_base_lid(p_physp);
	p_rec->port_num			= osm_physp_get_port_num(p_physp);
	p_rec->neighbor_mtu		= ib_port_info_get_neighbor_mtu(&p_physp->port_info);
	p_rec->rate			= ib_port_info_compute_rate(&p_physp->port_info,
								    p_pi->capability_mask & IB_PORT_CAP_HAS_EXT_SPEEDS) &
					  SSA_DB_PORT_RATE_MASK;
	p_rec->vl_enforce		= p_physp->port_info.vl_enforce;
	p_rec->rate			= (uint8_t) (p_rec->rate | is_fdr10_active | is_switch);
}

/** =========================================================================
 */
void ep_lft_block_tbl_rec_init(osm_switch_t * p_sw, uint16_t lid, uint16_t block,
			       struct ep_lft_block_tbl_rec *p_rec)
{
	p_rec->lid		= htons(lid);
	p_rec->block_num	= htons(block);
	memcpy(p_rec->block, p_sw->lft + block * IB_SMP_DATA_SIZE, IB_SMP_DATA_SIZE);
}

/** =========================================================================
 */
void ep_lft_top_tbl_rec_init(uint16_t lid, uint16_t lft_top, struct ep_lft_top_tbl_rec *p_rec)
{
	p_rec->lid = htons(lid);
	p_rec->lft_top = htons(lft_top);
}
