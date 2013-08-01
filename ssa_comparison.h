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

#ifndef _SSA_COMPARISON_H_
#define _SSA_COMPARISON_H_

#include <iba/ib_types.h>
#include <complib/cl_qmap.h>
#include <ssa_plugin.h>
#include <ssa_database.h>
#include <ssa_db.h>

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else                           /* !__cplusplus */
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif                          /* __cplusplus */

BEGIN_C_DECLS
#define SSA_DB_CHANGEMASK_SUBNET_PREFIX 	(((uint16_t)1)<<0)
#define SSA_DB_CHANGEMASK_SM_STATE 		(((uint16_t)1)<<1)
#define SSA_DB_CHANGEMASK_LMC 			(((uint16_t)1)<<2)
#define SSA_DB_CHANGEMASK_SUBNET_TIMEOUT 	(((uint16_t)1)<<3)
#define SSA_DB_CHANGEMASK_ALLOW_BOTH_PKEYS	(((uint16_t)1)<<5)

enum ssa_db_diff_table_id {
	SSA_TABLE_ID_TABLE_DEF = -2,
	SSA_TABLE_ID_GUID_TO_LID = 0,
	SSA_TABLE_ID_GUID_TO_LID_FIELD_DEF,
	SSA_TABLE_ID_NODE,
	SSA_TABLE_ID_NODE_FIELD_DEF,
	SSA_TABLE_ID_LINK,
	SSA_TABLE_ID_LINK_FIELD_DEF,
	SSA_TABLE_ID_PORT,
	SSA_TABLE_ID_PORT_FIELD_DEF,
	SSA_TABLE_ID_PKEY,
	SSA_TABLE_ID_LFT_TOP,
	SSA_TABLE_ID_LFT_TOP_FIELD_DEF,
	SSA_TABLE_ID_LFT_BLOCK,
	SSA_TABLE_ID_LFT_BLOCK_FIELD_DEF,
	SSA_TABLE_ID_MAX
};

enum ssa_db_diff_guid_to_lid_fields {
	SSA_FIELD_ID_GUID_TO_LID_GUID,
	SSA_FIELD_ID_GUID_TO_LID_LID,
	SSA_FIELD_ID_GUID_TO_LID_LMC,
	SSA_FIELD_ID_GUID_TO_LID_IS_SWITCH,
	SSA_FIELD_ID_GUID_TO_LID_MAX
};

enum ssa_db_diff_node_fields {
	SSA_FIELD_ID_NODE_NODE_GUID,
	SSA_FIELD_ID_NODE_IS_ENHANCED_SP0,
	SSA_FIELD_ID_NODE_NODE_TYPE,
	SSA_FIELD_ID_NODE_DESCRIPTION,
	SSA_FIELD_ID_NODE_MAX
};

enum ssa_db_diff_link_fields {
	SSA_FIELD_ID_LINK_FROM_LID,
	SSA_FIELD_ID_LINK_TO_LID,
	SSA_FIELD_ID_LINK_FROM_PORT_NUM,
	SSA_FIELD_ID_LINK_TO_PORT_NUM,
	SSA_FIELD_ID_LINK_MAX
};

enum ssa_db_diff_port_fields {
	SSA_FIELD_ID_PORT_PKEY_TBL_OFFSET,
	SSA_FIELD_ID_PORT_PKEY_TBL_SIZE,
	SSA_FIELD_ID_PORT_PORT_LID,
	SSA_FIELD_ID_PORT_PORT_NUM,
	SSA_FIELD_ID_PORT_NEIGHBOR_MTU,
	SSA_FIELD_ID_PORT_RATE,
	SSA_FIELD_ID_PORT_VL_ENFORCE,
	SSA_FIELD_ID_PORT_IS_FDR10_ACTIVE,
	SSA_FIELD_ID_PORT_MAX
};

enum ssa_db_diff_lft_top_fields {
	SSA_FIELD_ID_LFT_TOP_LID,
	SSA_FIELD_ID_LFT_TOP_LFT_TOP,
	SSA_FIELD_ID_LFT_TOP_MAX
};

enum ssa_db_diff_lft_block_fields {
	SSA_FIELD_ID_LFT_BLOCK_LID,
	SSA_FIELD_ID_LFT_BLOCK_BLOCK_NUM,
	SSA_FIELD_ID_LFT_BLOCK_BLOCK,
	SSA_FIELD_ID_LFT_BLOCK_MAX
};

#define SSA_TABLE_BLOCK_SIZE			1024

/* used for making comparison between two ssa databases */
struct ssa_db_diff {
	struct db_def			db_def;

	struct db_dataset		db_table_def;
	struct db_table_def		*p_def_tbl;

	/* data tables */
	struct db_dataset		db_tables[SSA_TABLE_ID_MAX];
	void				*p_tables[SSA_TABLE_ID_MAX];

	/***** guid_to_lid_tbl changes tracking **********/
	cl_qmap_t ep_guid_to_lid_tbl_added;
	cl_qmap_t ep_guid_to_lid_tbl_removed;
	/*************************************************/
	/********* node_tbl  changes tracking ************/
	cl_qmap_t ep_node_tbl_added;
	cl_qmap_t ep_node_tbl_removed;
	/*************************************************/
	/********** port_tbl changes tracking ************/
	cl_qmap_t ep_port_tbl_added;
	cl_qmap_t ep_port_tbl_removed;
	/*************************************************/
	/********** LFT changes tracking *****************/
	cl_qmap_t ep_lft_block_tbl;
	cl_qmap_t ep_lft_top_tbl;
	/*************************************************/
	/********** link_tbl changes tracking ************/
	cl_qmap_t ep_link_tbl_added;
	cl_qmap_t ep_link_tbl_removed;
	/*************************************************/

	/* change_mask bits point to the changed data fields */
	uint64_t change_mask;
	uint64_t subnet_prefix;
	uint8_t sm_state;
	uint8_t lmc;
	uint8_t subnet_timeout;
	uint8_t enable_quirks;
	uint8_t allow_both_pkeys;

	/* TODO: add support for changes in SLVL and in future for QoS and LFTs */
	uint8_t dirty;
};

struct ssa_db_diff *ssa_db_diff_init(uint64_t guid_to_lid_num_recs, uint64_t node_num_recs,
				     uint64_t link_num_recs, uint64_t port_num_recs,
				     uint64_t pkey_num_recs, uint64_t lft_top_num_recs,
				     uint64_t lft_block_num_recs);
void ssa_db_diff_destroy(struct ssa_db_diff * p_ssa_db_diff);
struct ssa_db_diff *ssa_db_compare(struct ssa_events * ssa,
				   struct ssa_database * ssa_db);
END_C_DECLS
#endif				/* _SSA_COMPARISON_H_ */
