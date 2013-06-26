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

static const struct db_table_def def_tbl[] = {
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_GUID_TO_LID, 0 }, "GUID to LID", sizeof(struct ep_guid_to_lid_tbl_rec), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, 0 },
							"GUID to LID fields", sizeof(struct db_field_def), SSA_TABLE_ID_GUID_TO_LID },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_NODE, 0 }, "NODE", sizeof(struct ep_node_tbl_rec), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, 0 }, "NODE fields", sizeof(struct db_field_def), SSA_TABLE_ID_NODE },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_LINK, 0 }, "LINK", sizeof(struct ep_link_tbl_rec), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, 0 }, "LINK fields", sizeof(struct db_field_def), SSA_TABLE_ID_LINK },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_PORT, 0 }, "PORT", sizeof(struct ep_port_tbl_rec), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, 0 }, "PORT fields", sizeof(struct db_field_def), SSA_TABLE_ID_PORT },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_PKEY, 0 }, "PKEY", DB_VARIABLE_SIZE, SSA_TABLE_ID_PORT },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_LFT_TOP, 0 }, "LFT TOP", sizeof(struct ep_lft_top_tbl_rec), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_LFT_TOP_FIELD_DEF, 0 }, "LFT TOP fields", sizeof(struct db_field_def), SSA_TABLE_ID_LFT_TOP },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DATA, 0, { 0, SSA_TABLE_ID_LFT_BLOCK, 0 }, "LFT BLOCK", sizeof(struct ep_lft_block_tbl_rec), 0 },
	{ 1, sizeof(struct db_table_def), DBT_TYPE_DEF, 0, { 0, SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, 0 }, "LFT BLOCK fields", sizeof(struct db_field_def), SSA_TABLE_ID_LFT_BLOCK },
	{ 0 }
};

static const struct db_dataset dataset_tbl[] = {
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_GUID_TO_LID, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_NODE, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LINK, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_PORT, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_PKEY, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LFT_TOP, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LFT_TOP_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LFT_BLOCK, 0 }, 0, 0, 0, 0 },
	{ 1, sizeof(struct db_dataset), 0, 0, { 0, SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, 0 }, 0, 0, 0, 0 },
	{ 0 }
};

static const struct db_field_def field_tbl[] = {
	{ 1, 0, DBF_TYPE_NET64, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, SSA_FIELD_ID_GUID_TO_LID_GUID }, "guid", 64, 0 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, SSA_FIELD_ID_GUID_TO_LID_LID }, "lid", 16, 64 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, SSA_FIELD_ID_GUID_TO_LID_LMC }, "lmc", 8, 80 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, SSA_FIELD_ID_GUID_TO_LID_IS_SWITCH }, "is_switch", 8, 88 },
	{ 1, 0, DBF_TYPE_NET64, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, SSA_FIELD_ID_NODE_NODE_GUID }, "node_guid", 64, 0 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, SSA_FIELD_ID_NODE_IS_ENHANCED_SP0 }, "is_enhanced_sp0", 8, 64 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, SSA_FIELD_ID_NODE_NODE_TYPE }, "node_type", 8, 72 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_NODE_FIELD_DEF, SSA_FIELD_ID_NODE_IS_ENHANCED_SP0 }, "description", 8 * IB_NODE_DESCRIPTION_SIZE, 80 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, SSA_FIELD_ID_LINK_FROM_LID }, "from_lid", 16, 0 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, SSA_FIELD_ID_LINK_TO_LID }, "to_lid", 16, 16 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, SSA_FIELD_ID_LINK_FROM_PORT_NUM }, "from_port_num", 8, 32 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_LINK_FIELD_DEF, SSA_FIELD_ID_LINK_TO_PORT_NUM }, "to_port_num", 8, 40 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_MAX_PKEYS }, "max_pkeys", 16, 0 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_USED_BLOCKS }, "used_blocks", 16, 16 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_PORT_LID }, "port_lid", 16, 32 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_PORT_NUM }, "port_num", 8, 48 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_NEIGHBOR_MTU }, "neighbor_mtu", 8, 56 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_LINK_SPEED_EXT }, "link_speed_ext", 8, 64 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_LINK_SPEED }, "link_speed", 8, 72 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_LINK_WIDTH_ACTIVE }, "link_width_active", 8, 80 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_VL_ENFORCE }, "vl_enforce", 8, 88 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_IS_FDR10_ACTIVE }, "is_fdr10_active", 8, 96 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LFT_TOP_FIELD_DEF, SSA_FIELD_ID_LFT_TOP_LID }, "lid", 16, 0 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LFT_TOP_FIELD_DEF, SSA_FIELD_ID_LFT_TOP_LFT_TOP }, "lft_top", 16, 16 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, SSA_FIELD_ID_LFT_BLOCK_LID }, "lid", 16, 0 },
	{ 1, 0, DBF_TYPE_NET16, 0, { 0, SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, SSA_FIELD_ID_LFT_BLOCK_BLOCK_NUM }, "block_num", 16, 16 },
	{ 1, 0, DBF_TYPE_U8, 0, { 0, SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, SSA_FIELD_ID_LFT_BLOCK_BLOCK }, "block", 8 * IB_SMP_DATA_SIZE, 32 },
	{ 0 }
};

struct db_field {
	enum ssa_db_diff_table_id	table_id;
	uint8_t				fields_num;
};

static const struct db_field field_per_table[] = {
	{ SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF, SSA_FIELD_ID_GUID_TO_LID_MAX},
	{ SSA_TABLE_ID_NODE_FIELD_DEF, SSA_FIELD_ID_NODE_MAX},
	{ SSA_TABLE_ID_LINK_FIELD_DEF, SSA_FIELD_ID_LINK_MAX},
	{ SSA_TABLE_ID_PORT_FIELD_DEF, SSA_FIELD_ID_PORT_MAX},
	{ SSA_TABLE_ID_LFT_TOP_FIELD_DEF, SSA_FIELD_ID_LFT_TOP_MAX},
	{ SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF, SSA_FIELD_ID_LFT_BLOCK_MAX},
	{ 0 }
};

void ssa_db_diff_db_def_init(struct db_def * p_db_def,
			     uint8_t version, uint8_t size,
			     uint8_t db_id, uint8_t table_id,
			     uint8_t field_id, const char * name,
			     uint32_t table_def_size)
{
	p_db_def->version		= version;
	p_db_def->size			= size;
	p_db_def->id.db			= db_id;
	p_db_def->id.table		= table_id;
	p_db_def->id.field		= field_id;
	strcpy(p_db_def->name, name);
	p_db_def->table_def_size	= htonl(table_def_size);
}

/** =========================================================================
 */
void ssa_db_diff_dataset_init(struct db_dataset * p_dataset,
			      uint8_t version, uint8_t size,
			      uint8_t access, uint8_t db_id,
			      uint8_t table_id, uint8_t field_id,
			      uint64_t epoch, uint64_t set_size,
			      uint64_t set_offset, uint64_t set_count)
{
	p_dataset->version	= version;
	p_dataset->size		= size;
	p_dataset->access	= access;
	p_dataset->id.db	= db_id;
	p_dataset->id.table	= table_id;
	p_dataset->id.field	= field_id;
	p_dataset->epoch	= htonll(epoch);
	p_dataset->set_size	= htonll(set_size);
	p_dataset->set_offset	= htonll(set_offset);
	p_dataset->set_count	= htonll(set_count);
}

/** =========================================================================
 */
void ssa_db_diff_table_def_insert(struct db_table_def * p_tbl,
				  struct db_dataset * p_dataset,
				  uint8_t version, uint8_t size,
				  uint8_t type, uint8_t access,
				  uint8_t db_id, uint8_t table_id,
				  uint8_t field_id, const char * name,
				  uint32_t record_size, uint32_t ref_table_id)
{
	struct db_table_def db_table_def_rec;

	memset(&db_table_def_rec, 0, sizeof(db_table_def_rec));

	db_table_def_rec.version	= version;
	db_table_def_rec.size		= size;
	db_table_def_rec.type		= type;
	db_table_def_rec.access		= access;
	db_table_def_rec.id.db		= db_id;
	db_table_def_rec.id.table	= table_id;
	db_table_def_rec.id.field	= field_id;
	strcpy(db_table_def_rec.name, name);
	db_table_def_rec.record_size	= htonl(record_size);
	db_table_def_rec.ref_table_id	= htonl(ref_table_id);

	memcpy(&p_tbl[p_dataset->set_count++], &db_table_def_rec,
	       sizeof(*p_tbl));

	p_dataset->set_size += sizeof(*p_tbl);
}

/** =========================================================================
 */
void ssa_db_diff_field_def_insert(struct db_field_def * p_tbl,
				  struct db_dataset * p_dataset,
				  uint8_t version, uint8_t type,
				  uint8_t db_id, uint8_t table_id,
				  uint8_t field_id, const char * name,
				  uint32_t field_size, uint32_t field_offset)
{
	struct db_field_def db_field_def_rec;

	memset(&db_field_def_rec, 0, sizeof(db_field_def_rec));

	db_field_def_rec.version	= version;
	db_field_def_rec.type		= type;
	db_field_def_rec.id.db		= db_id;
	db_field_def_rec.id.table	= table_id;
	db_field_def_rec.id.field	= field_id;
	strcpy(db_field_def_rec.name, name);
	db_field_def_rec.field_size	= htonl(field_size);
	db_field_def_rec.field_offset	= htonl(field_offset);

	memcpy(&p_tbl[p_dataset->set_count++], &db_field_def_rec,
	       sizeof(*p_tbl));

	p_dataset->set_size += sizeof(*p_tbl);
}

/** =========================================================================
 */
void ssa_db_diff_tables_init(struct ssa_db_diff * p_ssa_db_diff)
{
	const struct db_table_def *p_tbl_def;
	const struct db_dataset *p_dataset;
	const struct db_field_def *p_field_def;
	const struct db_field *p_field;

	/*
	 * db_def initialization
	 */
	ssa_db_diff_db_def_init(&p_ssa_db_diff->db_def,
				0, sizeof(p_ssa_db_diff->db_def),
				12 /* just some db_id */, 0, 0, "SMDB",
				sizeof(*p_ssa_db_diff->p_def_tbl));

	/*
	 * Definition tables dataset initialization
	 */
	ssa_db_diff_dataset_init(&p_ssa_db_diff->db_table_def,
				 0, sizeof(p_ssa_db_diff->db_table_def),
				 0, 0, SSA_TABLE_ID_TABLE_DEF, 0,
				 0, 0, 0, 0);

	p_ssa_db_diff->p_def_tbl = (struct db_table_def *)
		malloc(sizeof(*p_ssa_db_diff->p_def_tbl) * SSA_TABLE_ID_MAX);
	if (!p_ssa_db_diff->p_def_tbl) {
		/* add handling memory allocation failure */
	}

	/* adding table definitions */
	for (p_tbl_def = def_tbl; p_tbl_def->version; p_tbl_def++)
		ssa_db_diff_table_def_insert(p_ssa_db_diff->p_def_tbl,
					     &p_ssa_db_diff->db_table_def,
					     p_tbl_def->version, p_tbl_def->size,
					     p_tbl_def->type, p_tbl_def->access,
					     p_tbl_def->id.db, p_tbl_def->id.table,
					     p_tbl_def->id.field, p_tbl_def->name,
					     p_tbl_def->record_size, p_tbl_def->ref_table_id);

	/* data tables datasets initialization */
	for (p_dataset = dataset_tbl; p_dataset->version; p_dataset++)
		ssa_db_diff_dataset_init(&p_ssa_db_diff->db_tables[p_dataset->id.table],
					 p_dataset->version, p_dataset->size,
					 p_dataset->access, p_dataset->id.db,
					 p_dataset->id.table, p_dataset->id.field,
					 p_dataset->epoch, p_dataset->set_size,
					 p_dataset->set_offset, p_dataset->set_count);

	/* field tables initialization */
	for (p_field = field_per_table; p_field->table_id; p_field++) {
		p_ssa_db_diff->p_tables[p_field->table_id] =
			malloc(sizeof(struct db_field_def) * p_field->fields_num);
		if (!p_ssa_db_diff->p_tables[p_field->table_id]) {
			/* add handling memory allocation failure */
		}
		for (p_field_def = field_tbl; p_field_def->version; p_field_def++) {
			if (p_field_def->id.table == p_field->table_id)
				ssa_db_diff_field_def_insert(p_ssa_db_diff->p_tables[p_field->table_id],
							     &p_ssa_db_diff->db_tables[p_field->table_id],
							     p_field_def->version, p_field_def->type,
							     p_field_def->id.db, p_field_def->id.table,
							     p_field_def->id.field, p_field_def->name,
							     p_field_def->field_size, p_field_def->field_offset);
		}
	}
}

struct ssa_db_diff *ssa_db_diff_init(uint64_t guid_to_lid_num_recs,
				     uint64_t node_num_recs,
				     uint64_t link_num_recs,
				     uint64_t port_num_recs,
				     uint64_t pkey_num_recs,
				     uint64_t lft_top_num_recs,
				     uint64_t lft_block_num_recs)
{
	struct ssa_db_diff *p_ssa_db_diff;

	p_ssa_db_diff = (struct ssa_db_diff *) calloc(1, sizeof(*p_ssa_db_diff));
	if (p_ssa_db_diff) {
		ssa_db_diff_tables_init(p_ssa_db_diff);

		p_ssa_db_diff->p_tables[SSA_TABLE_ID_GUID_TO_LID] =
			malloc(sizeof(struct ep_guid_to_lid_tbl_rec) * guid_to_lid_num_recs);

		if (!p_ssa_db_diff->p_tables[SSA_TABLE_ID_GUID_TO_LID]) {
			/* TODO: add handling memory allocation failure */
		}

		p_ssa_db_diff->p_tables[SSA_TABLE_ID_NODE] =
			malloc(sizeof(struct ep_node_tbl_rec) * node_num_recs);

		if (!p_ssa_db_diff->p_tables[SSA_TABLE_ID_NODE]) {
			/* TODO: add handling memory allocation failure */
		}

		p_ssa_db_diff->p_tables[SSA_TABLE_ID_LINK] =
			malloc(sizeof(struct ep_link_tbl_rec) * link_num_recs);

		if (!p_ssa_db_diff->p_tables[SSA_TABLE_ID_LINK]) {
			/* TODO: add handling memory allocation failure */
		}

		p_ssa_db_diff->p_tables[SSA_TABLE_ID_PORT] =
			malloc(sizeof(struct ep_port_tbl_rec) * port_num_recs);

		if (!p_ssa_db_diff->p_tables[SSA_TABLE_ID_PORT]) {
			/* TODO: add handling memory allocation failure */
		}

		p_ssa_db_diff->p_tables[SSA_TABLE_ID_PKEY] =
			malloc(sizeof(uint16_t) * pkey_num_recs);

		if (!p_ssa_db_diff->p_tables[SSA_TABLE_ID_PKEY]) {
			/* TODO: add handling memory allocation failure */
		}

		p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_TOP] =
			malloc(sizeof(uint16_t) * lft_top_num_recs);

		if (!p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_TOP]) {
			/* TODO: add handling memory allocation failure */
		}

		p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_BLOCK] =
			malloc(sizeof(struct ep_lft_block_tbl_rec) * lft_block_num_recs);

		if (!p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_BLOCK]) {
			/* TODO: add handling memory allocation failure */
		}

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
void ssa_db_diff_tables_destroy(struct ssa_db_diff * p_ssa_db_diff)
{
	int i;

	if (!p_ssa_db_diff)
		return;

	for (i = 0; i < SSA_TABLE_ID_MAX; i++)
		free(p_ssa_db_diff->p_tables[i]);
}

/** =========================================================================
 */
void ssa_db_diff_destroy(struct ssa_db_diff * p_ssa_db_diff)
{
	if (p_ssa_db_diff) {
		ssa_db_diff_tables_destroy(p_ssa_db_diff);

		ssa_qmap_apply_func(&p_ssa_db_diff->ep_guid_to_lid_tbl_added,
				   ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed,
				   ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_node_tbl_added,
				   ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_node_tbl_removed,
				   ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_port_tbl_added,
				   ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_port_tbl_removed,
				   ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_link_tbl_added,
				   ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_link_tbl_removed,
				   ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_lft_block_tbl,
				   ep_map_rec_delete_pfn);
		ssa_qmap_apply_func(&p_ssa_db_diff->ep_lft_top_tbl,
				   ep_map_rec_delete_pfn);

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
static void ssa_db_diff_compare_subnet_opts(struct ssa_db * p_previous_db,
					    struct ssa_db * p_current_db,
					    struct ssa_db_diff * p_ssa_db_diff)
{
	uint8_t dirty = p_ssa_db_diff->dirty;

	if (!p_previous_db->initialized && p_current_db->initialized) {
		p_ssa_db_diff->subnet_prefix = p_current_db->subnet_prefix;
		p_ssa_db_diff->sm_state = p_current_db->sm_state;
		p_ssa_db_diff->lmc = p_current_db->lmc;
		p_ssa_db_diff->subnet_timeout = p_current_db->subnet_timeout;
		p_ssa_db_diff->allow_both_pkeys = p_current_db->allow_both_pkeys;

		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SUBNET_PREFIX;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SM_STATE;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_LMC;
		p_ssa_db_diff->change_mask |= SSA_DB_CHANGEMASK_SUBNET_TIMEOUT;
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
static void ssa_db_guid_to_lid_insert(cl_qmap_t *p_map,
				      struct db_dataset *p_dataset,
				      void **p_data_tbl,
				      uint64_t key,
				      cl_map_item_t * p_item,
				      void * p_data_tbl_src)
{
	struct ep_map_rec *p_map_rec_new, *p_map_rec_old;
	struct ep_guid_to_lid_tbl_rec *p_guid_to_lid_tbl_rec_dest;
	struct ep_guid_to_lid_tbl_rec *p_guid_to_lid_tbl_rec_src;

	p_guid_to_lid_tbl_rec_dest = (struct ep_guid_to_lid_tbl_rec *) *p_data_tbl;
	p_guid_to_lid_tbl_rec_src = (struct ep_guid_to_lid_tbl_rec *) p_data_tbl_src;

	p_map_rec_new = (struct ep_map_rec *)
			     malloc(sizeof(*p_map_rec_new));
	if (!p_map_rec_new) {
		/* handle failure - bad memory allocation */
	}
	p_map_rec_new->offset = p_dataset->set_count;
	cl_qmap_insert(p_map, key, &p_map_rec_new->map_item);

	if (!p_guid_to_lid_tbl_rec_dest) {
		/* handle failure - bad memory allocation */
	}

	p_map_rec_old = (struct ep_map_rec *) p_item;
	memcpy(&p_guid_to_lid_tbl_rec_dest[p_dataset->set_count],
	       &p_guid_to_lid_tbl_rec_src[p_map_rec_old->offset],
	       sizeof(*p_guid_to_lid_tbl_rec_dest));
	*p_data_tbl = p_guid_to_lid_tbl_rec_dest;
	p_dataset->set_size += sizeof(*p_guid_to_lid_tbl_rec_dest);
	p_dataset->set_count++;
}

/** =========================================================================
 */
static int ssa_db_guid_to_lid_cmp(cl_map_item_t * p_item_old,
				  void *p_data_tbl_old,
				  cl_map_item_t * p_item_new,
				  void *p_data_tbl_new)
{
	struct ep_map_rec *p_map_rec_old =
			(struct ep_map_rec *) p_item_old;
	struct ep_map_rec *p_map_rec_new =
			(struct ep_map_rec *) p_item_new;
	struct ep_guid_to_lid_tbl_rec *p_tbl_rec_old =
			(struct ep_guid_to_lid_tbl_rec *) p_data_tbl_old;
	struct ep_guid_to_lid_tbl_rec *p_tbl_rec_new =
			(struct ep_guid_to_lid_tbl_rec *) p_data_tbl_new;

	int res = 0;

	p_tbl_rec_old += p_map_rec_old->offset;
	p_tbl_rec_new += p_map_rec_new->offset;

	if (p_tbl_rec_old->lid != p_tbl_rec_new->lid ||
	    p_tbl_rec_old->lmc != p_tbl_rec_new->lmc ||
	    p_tbl_rec_old->is_switch != p_tbl_rec_new->is_switch)
		res = 1;

	return res;
}

/** =========================================================================
 */
static void ssa_db_node_insert(cl_qmap_t *p_map,
			       struct db_dataset *p_dataset,
			       void **p_data_tbl,
			       uint64_t key,
			       cl_map_item_t * p_item,
			       void *p_data_tbl_src)
{
	struct ep_map_rec *p_map_rec_new, *p_map_rec_old;
	struct ep_node_tbl_rec *p_node_tbl_rec_dest;
	struct ep_node_tbl_rec *p_node_tbl_rec_src;

	p_node_tbl_rec_dest = (struct ep_node_tbl_rec *) *p_data_tbl;
	p_node_tbl_rec_src = (struct ep_node_tbl_rec *) p_data_tbl_src;

	p_map_rec_new = (struct ep_map_rec *)
			     malloc(sizeof(*p_map_rec_new));
	if (!p_map_rec_new) {
		/* handle failure - bad memory allocation */
	}
	p_map_rec_new->offset = p_dataset->set_count;
	cl_qmap_insert(p_map, key, &p_map_rec_new->map_item);

	if (!p_node_tbl_rec_dest) {
		/* handle failure - bad memory allocation */
	}

	p_map_rec_old = (struct ep_map_rec *) p_item;
	memcpy(&p_node_tbl_rec_dest[p_dataset->set_count],
	       &p_node_tbl_rec_src[p_map_rec_old->offset],
	       sizeof(*p_node_tbl_rec_dest));
	*p_data_tbl = p_node_tbl_rec_dest;
	p_dataset->set_size += sizeof(*p_node_tbl_rec_dest);
	p_dataset->set_count++;
}

/** =========================================================================
 */
static int ssa_db_node_cmp(cl_map_item_t * p_item_old,
			   void *p_data_tbl_old,
			   cl_map_item_t * p_item_new,
			   void *p_data_tbl_new)
{
	struct ep_map_rec *p_map_rec_old =
			(struct ep_map_rec *) p_item_old;
	struct ep_map_rec *p_map_rec_new =
			(struct ep_map_rec *) p_item_new;
	struct ep_node_tbl_rec *p_tbl_rec_old =
			(struct ep_node_tbl_rec *) p_data_tbl_old;
	struct ep_node_tbl_rec *p_tbl_rec_new =
			(struct ep_node_tbl_rec *) p_data_tbl_new;
	int res = 0;

	p_tbl_rec_old += p_map_rec_old->offset;
	p_tbl_rec_new += p_map_rec_new->offset;

	if (p_tbl_rec_old->is_enhanced_sp0 != p_tbl_rec_new->is_enhanced_sp0 ||
	    p_tbl_rec_old->node_type != p_tbl_rec_new->node_type ||
	    memcmp(p_tbl_rec_old->description, p_tbl_rec_new->description,
		   IB_NODE_DESCRIPTION_SIZE))
		res = 1;

	return res;
}

/** =========================================================================
 */
static void ssa_db_port_insert(cl_qmap_t *p_map,
			       struct db_dataset *p_dataset,
			       void **p_data_tbl,
			       struct db_dataset *p_ref_dataset,
			       void **p_data_ref_tbl,
			       uint64_t *p_offset,
			       uint64_t key,
			       cl_map_item_t * p_item,
			       void *p_data_tbl_src,
			       void *p_data_ref_tbl_src)
{
	struct ep_map_rec *p_map_rec_new, *p_map_rec_old;
	struct ep_port_tbl_rec *p_port_tbl_rec_dest;
	struct ep_port_tbl_rec *p_port_tbl_rec_src;
	uint64_t offset_src;
	uint16_t size_pkey_tbl_src;
	uint16_t *p_pkey_tbl_dest;
	uint16_t *p_pkey_tbl_src;

	p_port_tbl_rec_dest = (struct ep_port_tbl_rec *) *p_data_tbl;
	p_port_tbl_rec_src = (struct ep_port_tbl_rec *) p_data_tbl_src;

	p_map_rec_new = (struct ep_map_rec *)
			     malloc(sizeof(*p_map_rec_new));
	if (!p_map_rec_new) {
		/* handle failure - bad memory allocation */
	}

	p_map_rec_new->offset = p_dataset->set_count;
	cl_qmap_insert(p_map, key, &p_map_rec_new->map_item);

	if (!p_port_tbl_rec_dest) {
		/* handle failure - bad memory allocation */
	}

	p_map_rec_old = (struct ep_map_rec *) p_item;
	memcpy(&p_port_tbl_rec_dest[p_dataset->set_count],
	       &p_port_tbl_rec_src[p_map_rec_old->offset],
	       sizeof(*p_port_tbl_rec_dest));
	*p_data_tbl = p_port_tbl_rec_dest;
	p_dataset->set_size += sizeof(*p_port_tbl_rec_dest);
	p_dataset->set_count++;

	if (p_data_ref_tbl && p_ref_dataset &&
	    p_data_ref_tbl_src && p_offset) {
		p_pkey_tbl_dest = (uint16_t *) *p_data_ref_tbl;
		p_pkey_tbl_src = (uint16_t *) p_data_ref_tbl_src;

		offset_src = p_port_tbl_rec_src[p_map_rec_old->offset].pkey_tbl_offset;
		size_pkey_tbl_src = p_port_tbl_rec_src[p_map_rec_old->offset].pkeys;

		if (size_pkey_tbl_src == 0)
			return;

		memcpy(&p_pkey_tbl_dest[*p_offset], &p_pkey_tbl_src[offset_src],
		       size_pkey_tbl_src * sizeof(uint16_t));
		p_port_tbl_rec_dest[p_dataset->set_count - 1].pkey_tbl_offset = *p_offset;
		p_port_tbl_rec_dest[p_dataset->set_count - 1].pkeys = size_pkey_tbl_src;
		p_ref_dataset->set_size += size_pkey_tbl_src;
		*p_offset += size_pkey_tbl_src;
	}
}

/** =========================================================================
 */
static int ssa_db_port_cmp(cl_map_item_t * p_item_old,
			   void *p_data_tbl_old,
			   void *p_data_ref_tbl_old,
			   cl_map_item_t * p_item_new,
			   void *p_data_tbl_new,
			   void *p_data_ref_tbl_new)
{
	struct ep_map_rec *p_map_rec_old =
			(struct ep_map_rec *) p_item_old;
	struct ep_map_rec *p_map_rec_new =
			(struct ep_map_rec *) p_item_new;
	struct ep_port_tbl_rec *p_tbl_rec_old =
			(struct ep_port_tbl_rec *) p_data_tbl_old;
	struct ep_port_tbl_rec *p_tbl_rec_new =
			(struct ep_port_tbl_rec *) p_data_tbl_new;
	uint16_t *p_tbl_ref_rec_old = (uint16_t *) p_data_ref_tbl_old;
	uint16_t *p_tbl_ref_rec_new = (uint16_t *) p_data_ref_tbl_new;
	int res = 0;

	p_tbl_rec_old += p_map_rec_old->offset;
	p_tbl_rec_old += p_map_rec_new->offset;
	p_tbl_ref_rec_old += p_tbl_rec_old->pkey_tbl_offset;
	p_tbl_ref_rec_new += p_tbl_rec_new->pkey_tbl_offset;

	if ((p_tbl_rec_old->pkeys != p_tbl_rec_new->pkeys) ||
	    (p_tbl_rec_old->port_lid != p_tbl_rec_new->port_lid) ||
	    (p_tbl_rec_old->neighbor_mtu != p_tbl_rec_new->neighbor_mtu) ||
	    (p_tbl_rec_old->link_speed_ext != p_tbl_rec_new->link_speed_ext) ||
	    (p_tbl_rec_old->link_speed != p_tbl_rec_new->link_speed) ||
	    (p_tbl_rec_old->link_width_active != p_tbl_rec_new->link_width_active) ||
	    (p_tbl_rec_old->vl_enforce != p_tbl_rec_new->vl_enforce) ||
	    (p_tbl_rec_old->is_fdr10_active != p_tbl_rec_new->is_fdr10_active))
		res = 1;

	/* comparying pkeys */
	if (res == 0 && p_data_ref_tbl_old && p_data_ref_tbl_new &&
	    memcmp(&p_tbl_ref_rec_old, &p_tbl_ref_rec_new,
		   p_tbl_rec_old->pkeys * sizeof(uint16_t)))
		res = 1;

	return res;
}

/** =========================================================================
 */
static void ssa_db_link_insert(cl_qmap_t *p_map,
			       struct db_dataset *p_dataset,
			       void **p_data_tbl,
			       uint64_t key,
			       cl_map_item_t * p_item,
			       void *p_data_tbl_src)
{
	struct ep_map_rec *p_map_rec_new, *p_map_rec_old;
	struct ep_link_tbl_rec *p_link_tbl_rec_dest;
	struct ep_link_tbl_rec *p_link_tbl_rec_src;

	p_link_tbl_rec_dest = (struct ep_link_tbl_rec *) *p_data_tbl;
	p_link_tbl_rec_src = (struct ep_link_tbl_rec *) p_data_tbl_src;

	p_map_rec_new = (struct ep_map_rec *)
			     malloc(sizeof(*p_map_rec_new));
	if (!p_map_rec_new) {
		/* handle failure - bad memory allocation */
	}

	p_map_rec_new->offset = p_dataset->set_count;
	cl_qmap_insert(p_map, key, &p_map_rec_new->map_item);

	if (!p_link_tbl_rec_dest) {
		ssa_log(SSA_LOG_ALL, "ILYA DEBUG: no LINK table was allocated\n");
		/* handle failure - bad memory allocation */
	}

	p_map_rec_old = (struct ep_map_rec *) p_item;
	memcpy(&p_link_tbl_rec_dest[p_dataset->set_count],
	       &p_link_tbl_rec_src[p_map_rec_old->offset],
	       sizeof(*p_link_tbl_rec_dest));
	*p_data_tbl = p_link_tbl_rec_dest;
	p_dataset->set_size += sizeof(*p_link_tbl_rec_dest);
	p_dataset->set_count++;
}

/** =========================================================================
 */
static int ssa_db_link_cmp(cl_map_item_t * p_item_old,
			   void *p_data_tbl_old,
			   cl_map_item_t * p_item_new,
			   void *p_data_tbl_new)
{
	struct ep_map_rec *p_map_rec_old =
			(struct ep_map_rec *) p_item_old;
	struct ep_map_rec *p_map_rec_new =
			(struct ep_map_rec *) p_item_new;
	struct ep_link_tbl_rec *p_tbl_rec_old =
			(struct ep_link_tbl_rec *) p_data_tbl_old;
	struct ep_link_tbl_rec *p_tbl_rec_new =
			(struct ep_link_tbl_rec *) p_data_tbl_new;
	int res = 0;

	p_tbl_rec_old += p_map_rec_old->offset;
	p_tbl_rec_old += p_map_rec_new->offset;

	if ((p_tbl_rec_old->from_lid != p_tbl_rec_new->from_lid) ||
	    (p_tbl_rec_old->to_lid != p_tbl_rec_new->to_lid) ||
	    (p_tbl_rec_old->from_port_num != p_tbl_rec_new->from_port_num) ||
	    (p_tbl_rec_old->to_port_num != p_tbl_rec_new->to_port_num))
		res = 1;

	return res;
}

 /** =========================================================================
  */
static uint8_t ssa_db_diff_table_cmp_v2(cl_qmap_t * p_map_old,
					cl_qmap_t * p_map_new,
					void *p_data_tbl_old,
					void *p_data_tbl_new,
					void (*qmap_insert_pfn)
					       (cl_qmap_t *,
						struct db_dataset *,
						void **, uint64_t,
						cl_map_item_t *,
						void *),
					int (*cmp_pfn)
						(cl_map_item_t *, void *,
						 cl_map_item_t *, void *),
					cl_qmap_t * p_map_added,
					cl_qmap_t * p_map_removed,
					struct db_dataset *p_dataset,
					void **p_data_tbl)
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
			qmap_insert_pfn(p_map_removed, p_dataset, p_data_tbl,
					key_old, p_item_old, p_data_tbl_old);
			p_item_old = cl_qmap_next(p_item_old);
			dirty = 1;
		} else if (key_old > key_new) {
			qmap_insert_pfn(p_map_added, p_dataset, p_data_tbl,
					key_new, p_item_new, p_data_tbl_new);
			p_item_new = cl_qmap_next(p_item_new);
			dirty = 1;
		} else {
			if (cmp_pfn(p_item_old, p_data_tbl_old, p_item_new, p_data_tbl_new)) {
				qmap_insert_pfn(p_map_removed, p_dataset, p_data_tbl,
						key_old, p_item_old, p_data_tbl_old);
				qmap_insert_pfn(p_map_added, p_dataset, p_data_tbl,
						key_new, p_item_new, p_data_tbl_new);
				dirty = 1;
			}
			p_item_old = cl_qmap_next(p_item_old);
			p_item_new = cl_qmap_next(p_item_new);
		}
	}

	while (p_item_new != cl_qmap_end(p_map_new)) {
		key_new = cl_qmap_key(p_item_new);
		qmap_insert_pfn(p_map_added, p_dataset, p_data_tbl,
				key_new, p_item_new, p_data_tbl_new);
		p_item_new = cl_qmap_next(p_item_new);
		dirty = 1;
	}

	while (p_item_old != cl_qmap_end(p_map_old)) {
		key_old = cl_qmap_key(p_item_old);
		qmap_insert_pfn(p_map_removed, p_dataset, p_data_tbl,
				key_old, p_item_old, p_data_tbl_old);
		p_item_old = cl_qmap_next(p_item_old);
		dirty = 1;
	}

	return dirty;
}

/** =========================================================================
 */
static uint8_t ssa_db_diff_var_size_table_cmp(cl_qmap_t * p_map_old,
					      cl_qmap_t * p_map_new,
					      void *p_data_tbl_old,
					      void *p_data_ref_tbl_old,
					      void *p_data_tbl_new,
					      void *p_data_ref_tbl_new,
					      void (*qmap_insert_pfn)
						       (cl_qmap_t *, struct db_dataset *,
							void **, struct db_dataset *,
							void **, uint64_t *, uint64_t,
							cl_map_item_t *,
							void *, void *),
					      int (*cmp_pfn)
							(cl_map_item_t *, void *,
							 void *, cl_map_item_t *,
							 void *, void *),
					      cl_qmap_t * p_map_added,
					      cl_qmap_t * p_map_removed,
					      struct db_dataset *p_dataset,
					      void **p_data_tbl,
					      struct db_dataset *p_ref_dataset,
					      void **p_data_ref_tbl)
{
	cl_map_item_t *p_item_old, *p_item_new;
	uint64_t key_old, key_new;
	uint64_t ref_tbl_offset = 0;
	uint8_t dirty = 0;

	p_item_old = cl_qmap_head(p_map_old);
	p_item_new = cl_qmap_head(p_map_new);
	while (p_item_old != cl_qmap_end(p_map_old) && p_item_new != cl_qmap_end(p_map_new)) {
		key_old = cl_qmap_key(p_item_old);
		key_new = cl_qmap_key(p_item_new);
		if (key_old < key_new) {
			qmap_insert_pfn(p_map_removed, p_dataset, p_data_tbl, NULL,
					NULL, NULL, key_old, p_item_old,
					p_data_tbl_old, NULL);
			p_item_old = cl_qmap_next(p_item_old);
			dirty = 1;
		} else if (key_old > key_new) {
			qmap_insert_pfn(p_map_added, p_dataset, p_data_tbl, p_ref_dataset,
					p_data_ref_tbl, &ref_tbl_offset, key_new,
					p_item_new, p_data_tbl_new, p_data_ref_tbl_new);
			p_item_new = cl_qmap_next(p_item_new);
			dirty = 1;
		} else {
			if (cmp_pfn(p_item_old, p_data_tbl_old, p_data_ref_tbl_old,
				    p_item_new, p_data_tbl_new, p_data_ref_tbl_new)) {
				qmap_insert_pfn(p_map_removed, p_dataset, p_data_tbl, NULL,
						NULL, NULL, key_old, p_item_old,
						p_data_tbl_old, NULL);
				qmap_insert_pfn(p_map_added, p_dataset, p_data_tbl, p_ref_dataset,
						p_data_ref_tbl, &ref_tbl_offset, key_new,
						p_item_new, p_data_tbl_new, p_data_ref_tbl_new);
				dirty = 1;
			}
			p_item_old = cl_qmap_next(p_item_old);
			p_item_new = cl_qmap_next(p_item_new);
		}
	}

	while (p_item_new != cl_qmap_end(p_map_new)) {
		key_new = cl_qmap_key(p_item_new);
		qmap_insert_pfn(p_map_added, p_dataset, p_data_tbl, p_ref_dataset,
				p_data_ref_tbl, &ref_tbl_offset, key_new,
				p_item_new, p_data_tbl_new, p_data_ref_tbl_new);
		p_item_new = cl_qmap_next(p_item_new);
		dirty = 1;
	}

	while (p_item_old != cl_qmap_end(p_map_old)) {
		key_old = cl_qmap_key(p_item_old);
		qmap_insert_pfn(p_map_removed, p_dataset, p_data_tbl, NULL,
				NULL, NULL, key_old, p_item_old,
				p_data_tbl_old, NULL);
		p_item_old = cl_qmap_next(p_item_old);
		dirty = 1;
	}

	return dirty;
}

/** =========================================================================
 */
static void ssa_db_diff_compare_subnet_tables(struct ssa_db * p_previous_db,
					      struct ssa_db * p_current_db,
					      struct ssa_db_diff * const p_ssa_db_diff)
{
	uint8_t dirty = 0;
	/*
	 * Comparing GUID2LID / ep_node_rec / ep_port_rec
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
	dirty |= ssa_db_diff_table_cmp_v2(&p_previous_db->ep_guid_to_lid_tbl,
					  &p_current_db->ep_guid_to_lid_tbl,
					  p_previous_db->p_guid_to_lid_tbl,
					  p_current_db->p_guid_to_lid_tbl,
					  ssa_db_guid_to_lid_insert,
					  ssa_db_guid_to_lid_cmp,
					  &p_ssa_db_diff->ep_guid_to_lid_tbl_added,
					  &p_ssa_db_diff->ep_guid_to_lid_tbl_removed,
					  &p_ssa_db_diff->db_tables[SSA_TABLE_ID_GUID_TO_LID],
					  (void **) &p_ssa_db_diff->p_tables[SSA_TABLE_ID_GUID_TO_LID]);

	dirty = dirty << 1;
	/*
	 * Comparing ep_node_rec records
	 */
	dirty |= ssa_db_diff_table_cmp_v2(&p_previous_db->ep_node_tbl,
					  &p_current_db->ep_node_tbl,
					  p_previous_db->p_node_tbl,
					  p_current_db->p_node_tbl,
					  ssa_db_node_insert,
					  ssa_db_node_cmp,
					  &p_ssa_db_diff->ep_node_tbl_added,
					  &p_ssa_db_diff->ep_node_tbl_removed,
					  &p_ssa_db_diff->db_tables[SSA_TABLE_ID_NODE],
					  (void **) &p_ssa_db_diff->p_tables[SSA_TABLE_ID_NODE]);

	dirty = dirty << 1;
	/*
	 * Comparing ep_link_rec records
	 */
	dirty |= ssa_db_diff_table_cmp_v2(&p_previous_db->ep_link_tbl,
					  &p_current_db->ep_link_tbl,
					  p_previous_db->p_link_tbl,
					  p_current_db->p_link_tbl,
					  ssa_db_link_insert,
					  ssa_db_link_cmp,
					  &p_ssa_db_diff->ep_link_tbl_added,
					  &p_ssa_db_diff->ep_link_tbl_removed,
					  &p_ssa_db_diff->db_tables[SSA_TABLE_ID_LINK],
					  (void **) &p_ssa_db_diff->p_tables[SSA_TABLE_ID_LINK]);

	dirty = dirty << 1;
	/*
	 * Comparing ep_port_rec records
	 */
	dirty |= ssa_db_diff_var_size_table_cmp(&p_previous_db->ep_port_tbl,
						&p_current_db->ep_port_tbl,
						p_previous_db->p_port_tbl,
						p_previous_db->p_pkey_tbl,
						p_current_db->p_port_tbl,
						p_current_db->p_pkey_tbl,
						ssa_db_port_insert,
						ssa_db_port_cmp,
						&p_ssa_db_diff->ep_port_tbl_added,
						&p_ssa_db_diff->ep_port_tbl_removed,
						&p_ssa_db_diff->db_tables[SSA_TABLE_ID_PORT],
						(void **) &p_ssa_db_diff->p_tables[SSA_TABLE_ID_PORT],
						&p_ssa_db_diff->db_tables[SSA_TABLE_ID_PKEY],
						(void **) &p_ssa_db_diff->p_tables[SSA_TABLE_ID_PKEY]);

	if (dirty)
		p_ssa_db_diff->dirty = 1;
}

/** =========================================================================
 */
#ifdef SSA_PLUGIN_VERBOSE_LOGGING
static void ssa_db_diff_dump_fabric_params(struct ssa_events * ssa,
					   struct ssa_db_diff * p_ssa_db_diff)
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
static void ssa_db_diff_dump_field_rec(struct ssa_events * ssa,
				       void * p_tbl, uint16_t max_rec)
{
	struct db_field_def *p_field_tbl = (struct db_field_def *) p_tbl;
	struct db_field_def *p_field_rec;
	uint8_t i;

	for (i = 0; i < max_rec; i++) {
		p_field_rec = &p_field_tbl[i];
		ssa_log(SSA_LOG_VERBOSE, "Field %s size %u offset %u\n",
			p_field_rec->name,
			ntohl(p_field_rec->field_size),
			ntohl(p_field_rec->field_offset));
	}
}

/** =========================================================================
 */
static void ssa_db_diff_dump_node_rec(struct ssa_events * ssa,
				      cl_map_item_t * p_item,
				      void * p_tbl)
{
	struct ep_map_rec *p_map_rec = (struct ep_map_rec *) p_item;
	struct ep_node_tbl_rec *p_node_tbl_rec, node_tbl_rec;
	char buffer[64];

	assert(p_map_rec);

	p_node_tbl_rec = (struct ep_node_tbl_rec *) p_tbl;
	if (p_node_tbl_rec) {
		node_tbl_rec = p_node_tbl_rec[p_map_rec->offset];
		if (node_tbl_rec.node_type == IB_NODE_TYPE_SWITCH)
			sprintf(buffer, " with %s Switch Port 0\n",
				node_tbl_rec.is_enhanced_sp0 ? "Enhanced" : "Base");
		else
			sprintf(buffer, "\n");
		ssa_log(SSA_LOG_VERBOSE, "Node GUID 0x%" PRIx64 " Type %d%s",
			cl_ntoh64(node_tbl_rec.node_guid), node_tbl_rec.node_type, buffer);
	}
}

/** =========================================================================
 */
static void ssa_db_diff_dump_guid_to_lid_rec(struct ssa_events * ssa,
					     cl_map_item_t * p_item,
					     void * p_tbl)
{
	struct ep_map_rec *p_map_rec = (struct ep_map_rec *) p_item;
	struct ep_guid_to_lid_tbl_rec *p_guid_to_lid_tbl_rec, guid_to_lid_tbl_rec;

	assert(p_map_rec);

	p_guid_to_lid_tbl_rec = (struct ep_guid_to_lid_tbl_rec *) p_tbl;
	if (p_guid_to_lid_tbl_rec) {
		guid_to_lid_tbl_rec = p_guid_to_lid_tbl_rec[p_map_rec->offset];
		ssa_log(SSA_LOG_VERBOSE, "Port GUID 0x%" PRIx64 " LID %u LMC %u is_switch %d\n",
			cl_ntoh64(guid_to_lid_tbl_rec.guid),
			cl_ntoh16(guid_to_lid_tbl_rec.lid),
			guid_to_lid_tbl_rec.lmc,
			guid_to_lid_tbl_rec.is_switch);
	}
}

/** =========================================================================
 */
static void ssa_db_diff_dump_port_rec(struct ssa_events * ssa,
				      cl_map_item_t * p_item,
				      void * p_tbl)
{
	struct ep_map_rec *p_map_rec = (struct ep_map_rec *) p_item;
	struct ep_port_tbl_rec *p_port_tbl_rec, port_tbl_rec;

	if (!p_map_rec)
		return;

	p_port_tbl_rec = (struct ep_port_tbl_rec *) p_tbl;
	if (p_port_tbl_rec) {
		port_tbl_rec = p_port_tbl_rec[p_map_rec->offset];
		ssa_log(SSA_LOG_VERBOSE, "Port LID %u Port Num %u\n",
			cl_ntoh16(port_tbl_rec.port_lid),
			port_tbl_rec.port_num);
		ssa_log(SSA_LOG_VERBOSE, "FDR10 %s active\n",
			port_tbl_rec.is_fdr10_active ? "" : "not");
		ssa_log(SSA_LOG_VERBOSE, "PKeys %u\n",
			port_tbl_rec.pkeys);
		ssa_log(SSA_LOG_VERBOSE, "PKey Table offset %u \n",
			port_tbl_rec.pkey_tbl_offset);
	}
}

/** =========================================================================
 */
static void ssa_db_diff_dump_lft_top_rec(struct ssa_events * ssa,
					 cl_map_item_t * p_item,
					 void * p_tbl)
{
	struct ep_map_rec *p_map_rec = (struct ep_map_rec *) p_item;
	struct ep_lft_top_tbl_rec *p_lft_top_tbl_rec, lft_top_tbl_rec;

	assert(p_map_rec);

	p_lft_top_tbl_rec = (struct ep_lft_top_tbl_rec *) p_tbl;
	if (p_lft_top_tbl_rec) {
		lft_top_tbl_rec = p_lft_top_tbl_rec[p_map_rec->offset];
		ssa_log(SSA_LOG_VERBOSE, "LID %u new LFT top %u\n",
			cl_ntoh16(p_lft_top_tbl_rec->lid), p_lft_top_tbl_rec->lft_top);
	}
}

/** =========================================================================
 */
static void ssa_db_diff_dump_lft_block_rec(struct ssa_events * ssa,
					   cl_map_item_t * p_item,
					   void * p_tbl)
{
	struct ep_map_rec *p_map_rec = (struct ep_map_rec *) p_item;
	struct ep_lft_block_tbl_rec *p_lft_block_tbl_rec, lft_block_tbl_rec;

	assert(p_map_rec);

	p_lft_block_tbl_rec = (struct ep_lft_block_tbl_rec *) p_tbl;
	if (p_lft_block_tbl_rec) {
		lft_block_tbl_rec = p_lft_block_tbl_rec[p_map_rec->offset];
		ssa_log(SSA_LOG_VERBOSE, "LID %u block #%u\n",
			cl_ntoh16(lft_block_tbl_rec.lid), lft_block_tbl_rec.block_num);
	}
}

/** =========================================================================
 */
static void ssa_db_diff_dump_link_rec(struct ssa_events * ssa,
				      cl_map_item_t * p_item,
				      void * p_tbl)
{
	struct ep_map_rec *p_map_rec = (struct ep_map_rec *) p_item;
	struct ep_link_tbl_rec *p_link_tbl_rec, link_tbl_rec;

	if (!p_map_rec)
		return;

	p_link_tbl_rec = (struct ep_link_tbl_rec *) p_tbl;
	if (p_link_tbl_rec) {
		link_tbl_rec = p_link_tbl_rec[p_map_rec->offset];
		ssa_log(SSA_LOG_VERBOSE, "From LID %u port %u to LID %u port %u\n",
			cl_ntoh16(link_tbl_rec.from_lid),
			link_tbl_rec.from_port_num,
			cl_ntoh16(link_tbl_rec.to_lid),
			link_tbl_rec.to_port_num);
	}
}

/** =========================================================================
 */
static void ssa_db_diff_dump_qmap_v2(cl_qmap_t * p_qmap,
				     struct ssa_events * ssa,
				     void (*pfn_dump)(struct ssa_events *,
				           cl_map_item_t *, void *),
				     void * p_tbl)
{
	cl_map_item_t *p_map_item, *p_map_item_next;
	uint8_t is_changed = 0;

        p_map_item_next = cl_qmap_head(p_qmap);
        while (p_map_item_next != cl_qmap_end(p_qmap)) {
                p_map_item = p_map_item_next;
                p_map_item_next = cl_qmap_next(p_map_item);
                pfn_dump(ssa, p_map_item, p_tbl);
		is_changed = 1;
	}

	if (!is_changed)
		ssa_log(SSA_LOG_VERBOSE, "No changes\n");
}

/** =========================================================================
 */
static void ssa_db_diff_dump(struct ssa_events * ssa,
			     struct ssa_db_diff * p_ssa_db_diff)
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
	ssa_log(ssa_log_level, "NODE field definitions:\n");
	ssa_db_diff_dump_field_rec(ssa, p_ssa_db_diff->p_tables[SSA_TABLE_ID_NODE_FIELD_DEF],
				   SSA_FIELD_ID_NODE_MAX);
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "Added records:\n");
	ssa_db_diff_dump_qmap_v2(&p_ssa_db_diff->ep_node_tbl_added,
				 ssa, ssa_db_diff_dump_node_rec,
				 p_ssa_db_diff->p_tables[SSA_TABLE_ID_NODE]);
	ssa_log(ssa_log_level, "Removed records:\n");
	ssa_db_diff_dump_qmap_v2(&p_ssa_db_diff->ep_node_tbl_removed,
				 ssa, ssa_db_diff_dump_node_rec,
				 p_ssa_db_diff->p_tables[SSA_TABLE_ID_NODE]);

	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "GUID to LID records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "GUID to LID field definitions:\n");
	ssa_db_diff_dump_field_rec(ssa, p_ssa_db_diff->p_tables[SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF],
				   SSA_FIELD_ID_GUID_TO_LID_MAX);
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "Added records:\n");
	ssa_db_diff_dump_qmap_v2(&p_ssa_db_diff->ep_guid_to_lid_tbl_added,
				 ssa, ssa_db_diff_dump_guid_to_lid_rec,
				 p_ssa_db_diff->p_tables[SSA_TABLE_ID_GUID_TO_LID]);
	ssa_log(ssa_log_level, "Removed records:\n");
	ssa_db_diff_dump_qmap_v2(&p_ssa_db_diff->ep_guid_to_lid_tbl_removed,
				 ssa, ssa_db_diff_dump_guid_to_lid_rec,
				 p_ssa_db_diff->p_tables[SSA_TABLE_ID_GUID_TO_LID]);

	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "PORT records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "PORT field definitions:\n");
	ssa_db_diff_dump_field_rec(ssa, p_ssa_db_diff->p_tables[SSA_TABLE_ID_PORT_FIELD_DEF],
				   SSA_FIELD_ID_PORT_MAX);
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "Added records:\n");
	ssa_db_diff_dump_qmap_v2(&p_ssa_db_diff->ep_port_tbl_added,
				 ssa, ssa_db_diff_dump_port_rec,
				 p_ssa_db_diff->p_tables[SSA_TABLE_ID_PORT]);
	ssa_log(ssa_log_level, "Removed records:\n");
	ssa_db_diff_dump_qmap_v2(&p_ssa_db_diff->ep_port_tbl_removed,
				 ssa, ssa_db_diff_dump_port_rec,
				 p_ssa_db_diff->p_tables[SSA_TABLE_ID_PORT]);

	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "LFT block records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "LFT block field definitions:\n");
	ssa_db_diff_dump_field_rec(ssa, p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF],
				   SSA_FIELD_ID_LFT_BLOCK_MAX);
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_db_diff_dump_qmap_v2(&p_ssa_db_diff->ep_lft_block_tbl,
				 ssa, ssa_db_diff_dump_lft_block_rec,
				 p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_BLOCK]);

	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "LFT top records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "LFT top field definitions:\n");
	ssa_db_diff_dump_field_rec(ssa, p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_TOP_FIELD_DEF],
				   SSA_FIELD_ID_LFT_TOP_MAX);
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_db_diff_dump_qmap_v2(&p_ssa_db_diff->ep_lft_top_tbl,
				 ssa, ssa_db_diff_dump_lft_top_rec,
				 p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_TOP]);

	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "Link Records:\n");
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "LINK field definitions:\n");
	ssa_db_diff_dump_field_rec(ssa, p_ssa_db_diff->p_tables[SSA_TABLE_ID_LINK_FIELD_DEF],
				   SSA_FIELD_ID_LINK_MAX);
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "Added records:\n");
	ssa_db_diff_dump_qmap_v2(&p_ssa_db_diff->ep_link_tbl_added,
				 ssa, ssa_db_diff_dump_link_rec,
				 p_ssa_db_diff->p_tables[SSA_TABLE_ID_LINK]);
	ssa_log(ssa_log_level, "Removed records:\n");
	ssa_db_diff_dump_qmap_v2(&p_ssa_db_diff->ep_link_tbl_removed,
				 ssa, ssa_db_diff_dump_link_rec,
				 p_ssa_db_diff->p_tables[SSA_TABLE_ID_LINK]);
	ssa_log(ssa_log_level, "-----------------------------------\n");
	ssa_log(ssa_log_level, "===================================\n");
}
#endif

/** =========================================================================
 */
static void ep_lft_block_qmap_copy(cl_qmap_t *p_dest_qmap,
				   struct db_dataset *p_dest_dataset,
				   struct ep_lft_block_tbl_rec *p_dest_tbl,
				   cl_qmap_t *p_src_qmap,
				   struct ep_lft_block_tbl_rec *p_src_tbl)
{
	struct ep_map_rec *p_map_rec, *p_map_rec_next;
	struct ep_map_rec *p_map_rec_new, *p_map_rec_tmp;
	struct ep_lft_block_tbl_rec *p_lft_block_tbl_rec;
	uint64_t offset;

	p_map_rec_next = (struct ep_map_rec *) cl_qmap_head(p_src_qmap);
	while (p_map_rec_next !=
	       (struct ep_map_rec *) cl_qmap_end(p_src_qmap)) {
		p_map_rec = p_map_rec_next;
		p_map_rec_next = (struct ep_map_rec *)
				   cl_qmap_next(&p_map_rec->map_item);
		p_lft_block_tbl_rec = &p_src_tbl[p_map_rec->offset];

		p_map_rec_tmp = (struct ep_map_rec *)
			cl_qmap_get(p_dest_qmap, cl_qmap_key(&p_map_rec->map_item));

		if (p_map_rec_tmp != (struct ep_map_rec *) cl_qmap_end(p_dest_qmap)) {
			/* in case of existing record */
			memcpy(&p_dest_tbl[p_map_rec_tmp->offset],
			       p_lft_block_tbl_rec, sizeof(*p_lft_block_tbl_rec));
		} else {
			/* in case of new record added */
			if (p_dest_dataset) {
				offset = p_dest_dataset->set_count;
				p_dest_dataset->set_size += sizeof(*p_lft_block_tbl_rec);
				p_dest_dataset->set_count++;
			} else {
				offset = cl_qmap_count(p_dest_qmap);
			}

			p_map_rec_new = ep_map_rec_init(offset);
			cl_qmap_insert(p_dest_qmap, cl_qmap_key(&p_map_rec->map_item),
				       &p_map_rec_new->map_item);

			memcpy(&p_dest_tbl[offset],
			       p_lft_block_tbl_rec, sizeof(*p_lft_block_tbl_rec));
		}
	}
}

/** =========================================================================
 */
static void ep_lft_top_qmap_copy(cl_qmap_t *p_dest_qmap,
				 struct db_dataset *p_dest_dataset,
				 struct ep_lft_top_tbl_rec *p_dest_tbl,
				 cl_qmap_t *p_src_qmap,
				 struct ep_lft_top_tbl_rec *p_src_tbl)
{
	struct ep_map_rec *p_map_rec, *p_map_rec_next;
	struct ep_map_rec *p_map_rec_new, *p_map_rec_tmp;
	struct ep_lft_top_tbl_rec *p_lft_top_tbl_rec;
	uint64_t offset;

	p_map_rec_next = (struct ep_map_rec *) cl_qmap_head(p_src_qmap);
	while (p_map_rec_next !=
	       (struct ep_map_rec *) cl_qmap_end(p_src_qmap)) {
		p_map_rec = p_map_rec_next;
		p_map_rec_next = (struct ep_map_rec *)
				   cl_qmap_next(&p_map_rec->map_item);
		p_lft_top_tbl_rec = &p_src_tbl[p_map_rec->offset];

		p_map_rec_tmp = (struct ep_map_rec *)
			cl_qmap_get(p_dest_qmap, cl_qmap_key(&p_map_rec->map_item));

		if (p_map_rec_tmp != (struct ep_map_rec *) cl_qmap_end(p_dest_qmap)) {
			/* in case of existing record */
			memcpy(&p_dest_tbl[p_map_rec_tmp->offset],
			       p_lft_top_tbl_rec, sizeof(*p_lft_top_tbl_rec));
		} else {
			/* in case of new record added */
			if (p_dest_dataset) {
				offset = p_dest_dataset->set_count;
				p_dest_dataset->set_size += sizeof(*p_lft_top_tbl_rec);
				p_dest_dataset->set_count++;
			} else {
				offset = cl_qmap_count(p_dest_qmap);
			}

			p_map_rec_new = ep_map_rec_init(offset);
			cl_qmap_insert(p_dest_qmap, cl_qmap_key(&p_map_rec->map_item),
				       &p_map_rec_new->map_item);

			memcpy(&p_dest_tbl[offset],
			       p_lft_top_tbl_rec, sizeof(*p_lft_top_tbl_rec));
		}
	}
}

/** =========================================================================
 */
struct ssa_db_diff *ssa_db_compare(struct ssa_events * ssa,
				   struct ssa_database * ssa_db)
{
	struct ssa_db_diff *p_ssa_db_diff = NULL;
	uint64_t guid_to_lid_num_recs, node_num_recs;
	uint64_t link_num_recs, port_num_recs, pkey_num_recs;
	uint64_t lft_top_num_recs, lft_block_num_recs;

	ssa_log(SSA_LOG_VERBOSE, "[\n");

	if (!ssa_db || !ssa_db->p_previous_db ||
	    !ssa_db->p_current_db || !ssa_db->p_dump_db ||
	    !ssa_db->p_lft_db) {
		/* bad arguments - error handling */
		ssa_log(SSA_LOG_ALL, "SMDB Comparison: bad arguments\n");
		goto Exit;
	}

	guid_to_lid_num_recs = cl_qmap_count(&ssa_db->p_current_db->ep_guid_to_lid_tbl) +
			cl_qmap_count(&ssa_db->p_previous_db->ep_guid_to_lid_tbl);
	node_num_recs = cl_qmap_count(&ssa_db->p_current_db->ep_node_tbl) +
			cl_qmap_count(&ssa_db->p_previous_db->ep_node_tbl);
	link_num_recs = cl_qmap_count(&ssa_db->p_current_db->ep_link_tbl) +
			cl_qmap_count(&ssa_db->p_previous_db->ep_link_tbl);
	port_num_recs = cl_qmap_count(&ssa_db->p_current_db->ep_port_tbl) +
			cl_qmap_count(&ssa_db->p_previous_db->ep_port_tbl);
	pkey_num_recs = ssa_db->p_current_db->pkey_tbl_rec_num;
	lft_top_num_recs = cl_qmap_count(&ssa_db->p_lft_db->ep_db_lft_top_tbl) +
			cl_qmap_count(&ssa_db->p_lft_db->ep_dump_lft_top_tbl);
	lft_block_num_recs = cl_qmap_count(&ssa_db->p_lft_db->ep_db_lft_block_tbl) +
			cl_qmap_count(&ssa_db->p_lft_db->ep_dump_lft_block_tbl);

	p_ssa_db_diff = ssa_db_diff_init(guid_to_lid_num_recs, node_num_recs,
					 link_num_recs, port_num_recs, pkey_num_recs,
					 lft_top_num_recs, lft_block_num_recs);
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
		ep_lft_block_qmap_copy(&p_ssa_db_diff->ep_lft_block_tbl, &p_ssa_db_diff->db_tables[SSA_TABLE_ID_LFT_BLOCK],
				     p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_BLOCK], &ssa_db->p_lft_db->ep_db_lft_block_tbl,
				     ssa_db->p_lft_db->p_db_lft_block_tbl);
		ep_lft_top_qmap_copy(&p_ssa_db_diff->ep_lft_top_tbl, &p_ssa_db_diff->db_tables[SSA_TABLE_ID_LFT_TOP],
				     p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_TOP], &ssa_db->p_lft_db->ep_db_lft_top_tbl,
				     ssa_db->p_lft_db->p_db_lft_top_tbl);
	} else {
		ep_lft_block_qmap_copy(&p_ssa_db_diff->ep_lft_block_tbl, &p_ssa_db_diff->db_tables[SSA_TABLE_ID_LFT_BLOCK],
				     p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_BLOCK], &ssa_db->p_lft_db->ep_dump_lft_block_tbl,
				     ssa_db->p_lft_db->p_dump_lft_block_tbl);
		ep_lft_top_qmap_copy(&p_ssa_db_diff->ep_lft_top_tbl, &p_ssa_db_diff->db_tables[SSA_TABLE_ID_LFT_TOP],
				     p_ssa_db_diff->p_tables[SSA_TABLE_ID_LFT_TOP], &ssa_db->p_lft_db->ep_dump_lft_top_tbl,
				     ssa_db->p_lft_db->p_dump_lft_top_tbl);

		if (cl_qmap_count(&ssa_db->p_lft_db->ep_dump_lft_top_tbl) > 0) {
			ssa_db->p_lft_db->p_db_lft_top_tbl = (struct ep_lft_top_tbl_rec *)
						realloc(&ssa_db->p_lft_db->p_db_lft_top_tbl[0],
							(cl_qmap_count(&ssa_db->p_lft_db->ep_db_lft_top_tbl) +
							 cl_qmap_count(&ssa_db->p_lft_db->ep_dump_lft_top_tbl)) *
							 sizeof(*ssa_db->p_lft_db->p_db_lft_top_tbl));
		}
		if (cl_qmap_count(&ssa_db->p_lft_db->ep_dump_lft_block_tbl) > 0) {
			ssa_db->p_lft_db->p_db_lft_block_tbl = (struct ep_lft_block_tbl_rec *)
						realloc(&ssa_db->p_lft_db->p_db_lft_block_tbl[0],
							(cl_qmap_count(&ssa_db->p_lft_db->ep_db_lft_block_tbl) +
							 cl_qmap_count(&ssa_db->p_lft_db->ep_dump_lft_block_tbl)) *
							 sizeof(*ssa_db->p_lft_db->p_db_lft_block_tbl));
		}
		/* Apply LFT block / top changes on existing LFT database */
		ep_lft_block_qmap_copy(&ssa_db->p_lft_db->ep_db_lft_block_tbl, NULL,
				     ssa_db->p_lft_db->p_db_lft_block_tbl, &ssa_db->p_lft_db->ep_dump_lft_block_tbl,
				     ssa_db->p_lft_db->p_dump_lft_block_tbl);
		ep_lft_top_qmap_copy(&ssa_db->p_lft_db->ep_db_lft_top_tbl, NULL,
				     ssa_db->p_lft_db->p_db_lft_top_tbl, &ssa_db->p_lft_db->ep_dump_lft_top_tbl,
				     ssa_db->p_lft_db->p_dump_lft_top_tbl);
		/* Clear LFT dump data */
		ep_qmap_clear(&ssa_db->p_lft_db->ep_dump_lft_block_tbl);
		ep_qmap_clear(&ssa_db->p_lft_db->ep_dump_lft_top_tbl);
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
