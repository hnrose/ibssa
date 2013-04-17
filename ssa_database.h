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

struct ep_guid_to_lid_rec {
	cl_map_item_t map_item;
	uint16_t lid;
	uint8_t lmc;		/* or just fabric lmc ? */
#if 1
	/* Below is to optimize SP0 (if not in other tables) */
	uint8_t is_switch;	/* ??? */
#else
	uint8_t pad;		/* ??? */
#endif
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
	uint8_t mtu_cap;
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

struct ep_lft_rec {
	cl_map_item_t map_item;
	uint16_t max_lid_ho;
	uint16_t lft_size;
	uint8_t *lft;
};

struct ssa_db {
	/* mutex ??? */
	cl_qmap_t ep_guid_to_lid_tbl;	/* port GUID -> LID */
	cl_qmap_t ep_node_tbl;		/* node GUID based */
	cl_qmap_t ep_port_tbl;		/* LID + port_num based*/
	cl_qmap_t ep_link_tbl;		/* LID + port_num based */
	cl_qmap_t ep_lft_tbl;		/* LID based */

	/* Fabric/SM related */
	uint64_t subnet_prefix;		/* even if full PortInfo used */
	uint8_t sm_state;
	uint8_t lmc;
	uint8_t subnet_timeout;
	uint8_t fabric_mtu;
	uint8_t fabric_rate;
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
};


extern struct ssa_database *ssa_db;

/**********************SSA Database**************************************/
struct ssa_database *ssa_database_init();
void ssa_database_delete(struct ssa_database *p_ssa_db);

/**********************SSA DB********************************************/
struct ssa_db *ssa_db_init();
void ssa_db_copy(struct ssa_db *p_dest_db, struct ssa_db *p_src_db);
void ssa_db_delete(struct ssa_db *p_ssa_db);

/**********************GUID to LID records*******************************/
struct ep_guid_to_lid_rec *ep_guid_to_lid_rec_init(osm_port_t *p_port);
void ep_guid_to_lid_rec_copy(struct ep_guid_to_lid_rec *p_dest_rec, struct ep_guid_to_lid_rec *p_src_rec);
void ep_guid_to_lid_rec_delete(struct ep_guid_to_lid_rec *p_ep_guid_to_lid_rec);
void ep_guid_to_lid_rec_delete_pfn(cl_map_item_t *p_map_item);

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

/**********************LFT records***************************************/
struct ep_lft_rec *ep_lft_rec_init(osm_switch_t *p_switch);
inline uint64_t ep_lft_rec_gen_key(uint16_t lid);
void ep_lft_rec_copy(struct ep_lft_rec *p_dest_rec, struct ep_lft_rec *p_src_rec);
void ep_lft_rec_delete(struct ep_lft_rec *p_ep_lft_rec);
void ep_lft_rec_delete_pfn(cl_map_item_t *p_map_item);

/**********************PORT records**************************************/
struct ep_port_rec *ep_port_rec_init(osm_physp_t *p_physp);
void ep_port_rec_copy(struct ep_port_rec *p_dest_rec, struct ep_port_rec *p_src_rec);
void ep_port_rec_delete(struct ep_port_rec *p_ep_port_rec);
void ep_port_rec_delete_pfn(cl_map_item_t *p_map_item);
/***********************************************************************/

uint64_t ep_rec_gen_key(uint16_t lid, uint8_t port_num);
void ssa_qmap_apply_func(cl_qmap_t *p_qmap, void (*destroy_pfn)(cl_map_item_t *));
END_C_DECLS
#endif				/* _SSA_DATABASE_H_ */
