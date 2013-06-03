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

#ifndef _SSA_DATABASE_H_
#define _SSA_DATABASE_H_

#include <iba/ib_types.h>
#include <complib/cl_ptr_vector.h>
#include <complib/cl_qmap.h>
#include <opensm/osm_node.h>
#include <opensm/osm_port.h>
#include <opensm/osm_switch.h>

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else                           /* !__cplusplus */
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif                          /* __cplusplus */

BEGIN_C_DECLS

struct ep_guid_to_lid_tbl_rec {
	uint64_t	guid;
	uint16_t	lid;
	uint8_t		lmc;
	uint8_t		is_switch;
	uint8_t		pad[4];
};

struct ep_map_rec {
	cl_map_item_t	map_item;
	uint64_t	offset;
};

struct ep_node_rec {
	cl_map_item_t map_item;
#if 1
	/* or just device_id, vendor_id, enh SP0 ? */
	ib_node_info_t node_info;
#else
	/* or just is_tavor ? */
	uint32_t vendor_id;	/* needed for Tavor MTU */
	uint16_t device_id;	/* needed for Tavor MTU */
	uint16_t pad;
#endif
	ib_node_desc_t node_desc;
	uint8_t is_enhanced_sp0;
	uint8_t pad[3];
};

struct ep_pkey_rec {
	/* port number only needed for switch external ports, not if only end ports */
	/* actual pkey table blocks or pkeys map ? */
#if 1
	uint16_t max_pkeys;     /* from NodeInfo.PartitionCap */
	uint16_t used_blocks;
	ib_pkey_table_t pkey_tbl[0];
#else
	cl_map_t pkeys;
#endif
};

struct ep_port_rec {
	cl_map_item_t map_item;
	/* or just (subnet prefix), cap mask, port state ?, active speeds, active width, and mtu cap ? */
	/*** PORT INFO ****/
	uint8_t neighbor_mtu;
	uint8_t link_speed_ext;
	uint8_t link_speed;
	uint8_t link_width_active;
	uint8_t vl_enforce;
	/******************/
	uint8_t is_fdr10_active;
	uint8_t pad[3];
	cl_ptr_vector_t slvl_by_port;	/* the length is different for switch or host port */
	struct ep_pkey_rec ep_pkey_rec;
};

struct ep_link_rec {
	cl_map_item_t map_item;
	ib_link_record_t link_rec;
};

struct ep_lft_block_rec {
	cl_map_item_t map_item;
	uint16_t lid;
	uint16_t block_num;
	uint8_t block[IB_SMP_DATA_SIZE];
};

struct ep_lft_top_rec {
	cl_map_item_t map_item;
	uint16_t lid;
	uint16_t lft_top;
};

struct ssa_db_lft {
	cl_qmap_t ep_db_lft_block_tbl;		/* LID + block_num based */
	cl_qmap_t ep_db_lft_top_tbl;		/* LID based */
	cl_qmap_t ep_dump_lft_block_tbl;	/* LID + block_num based */
	cl_qmap_t ep_dump_lft_top_tbl;		/* LID based */
};

struct ssa_db {
	/* mutex ??? */
	struct ep_guid_to_lid_tbl_rec	*p_guid_to_lid_tbl;

	cl_qmap_t ep_guid_to_lid_tbl;	/* port GUID -> offset */
	cl_qmap_t ep_node_tbl;		/* node GUID based */
	cl_qmap_t ep_port_tbl;		/* LID + port_num based*/
	cl_qmap_t ep_link_tbl;		/* LID + port_num based */

	/* Fabric/SM related */
	uint64_t subnet_prefix;		/* even if full PortInfo used */
	uint8_t sm_state;
	uint8_t lmc;
	uint8_t subnet_timeout;
	boolean_t enable_quirks;
	boolean_t allow_both_pkeys;
	/* boolean_t qos; */
	/* prefix_routes */
	uint8_t initialized;
};

struct ssa_database {
	/* mutex ??? */
	struct ssa_db *p_current_db;
	struct ssa_db *p_previous_db;
	struct ssa_db *p_dump_db;
	struct ssa_db_lft *p_lft_db;
};

extern struct ssa_database *ssa_db;

/**********************SSA Database**************************************/
struct ssa_database *ssa_database_init();
void ssa_database_delete(struct ssa_database *p_ssa_db);

/**********************SSA DB********************************************/
struct ssa_db *ssa_db_init();
void ssa_db_delete(struct ssa_db *p_ssa_db);

/**********************GUID to LID records*******************************/
void ep_guid_to_lid_tbl_rec_init(osm_port_t *p_port,
				 struct ep_guid_to_lid_tbl_rec * p_rec);

/**********************NODE records**************************************/
struct ep_node_rec *ep_node_rec_init(osm_node_t *p_osm_node);
void ep_node_rec_copy(struct ep_node_rec *p_dest_rec, struct ep_node_rec *p_src_rec);
void ep_node_rec_delete(struct ep_node_rec *p_ep_node_rec);
void ep_node_rec_delete_pfn(cl_map_item_t *p_map_item);

/**********************LINK records**************************************/
struct ep_link_rec *ep_link_rec_init(osm_physp_t *p_physp);
void ep_link_rec_copy(struct ep_link_rec *p_dest_rec, struct ep_link_rec *p_src_rec);
void ep_link_rec_delete(struct ep_link_rec *p_ep_link_rec);
void ep_link_rec_delete_pfn(cl_map_item_t *p_map_item);

/********************** LFT Block records*******************************/
struct ep_lft_block_rec *ep_lft_block_rec_init(osm_switch_t *p_sw,
					       uint16_t lid, uint16_t block);
void ep_lft_block_rec_copy(struct ep_lft_block_rec * p_dest_rec,
			   struct ep_lft_block_rec * p_src_rec);
inline uint64_t ep_lft_block_rec_gen_key(uint16_t lid, uint16_t block_num);
void ep_lft_block_rec_delete(struct ep_lft_block_rec *p_lft_block_rec);
void ep_lft_block_rec_delete_pfn(cl_map_item_t * p_map_item);
void ep_lft_block_rec_qmap_clear(cl_qmap_t *p_map);

/********************** LFT Top records*********************************/
struct ep_lft_top_rec *ep_lft_top_rec_init(uint16_t lid, uint16_t lft_top);
void ep_lft_top_rec_copy(struct ep_lft_top_rec * p_dest_rec,
			 struct ep_lft_top_rec * p_src_rec);
inline uint64_t ep_lft_top_rec_gen_key(uint16_t lid);
void ep_lft_top_rec_delete(struct ep_lft_top_rec *p_lft_top_rec);
void ep_lft_top_rec_delete_pfn(cl_map_item_t * p_map_item);
void ep_lft_top_rec_qmap_clear(cl_qmap_t *p_map);

/**********************PORT records**************************************/
struct ep_port_rec *ep_port_rec_init(osm_physp_t *p_physp);
void ep_port_rec_copy(struct ep_port_rec *p_dest_rec, struct ep_port_rec *p_src_rec);
void ep_port_rec_delete(struct ep_port_rec *p_ep_port_rec);
void ep_port_rec_delete_pfn(cl_map_item_t *p_map_item);
/***********************************************************************/

uint64_t ep_rec_gen_key(uint16_t lid, uint8_t port_num);
inline void ep_map_rec_copy(struct ep_map_rec *p_src_rec, struct ep_map_rec *p_dest_rec);
struct ep_map_rec *ep_map_rec_init(uint64_t offset);
void ep_map_rec_delete(struct ep_map_rec *p_map_rec);
void ep_map_rec_delete_pfn(cl_map_item_t *p_map_item);
void ssa_qmap_apply_func(cl_qmap_t *p_qmap, void (*destroy_pfn)(cl_map_item_t *));
END_C_DECLS
#endif				/* _SSA_DATABASE_H_ */
