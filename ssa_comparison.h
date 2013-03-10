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
#define SSA_DB_CHANGEMASK_FABRIC_MTU		(((uint16_t)1)<<4)
#define SSA_DB_CHANGEMASK_FABRIC_RATE		(((uint16_t)1)<<5)
#define SSA_DB_CHANGEMASK_ENABLE_QUIRKS		(((uint16_t)1)<<6)
#define SSA_DB_CHANGEMASK_ALLOW_BOTH_PKEYS	(((uint16_t)1)<<7)

/* used for making comparison between two ssa databases */
struct ssa_db_diff {
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

	/* change_mask bits point to the changed data fields
	 * (in case of more than 16 fields the type of change_mask
	 * has to be changed accordingly)
	 */
	uint64_t change_mask;
	uint64_t subnet_prefix;
	uint8_t sm_state;
	uint8_t lmc;
	uint8_t subnet_timeout;
	uint8_t fabric_mtu;
	uint8_t fabric_rate;
	uint8_t enable_quirks;
	uint8_t allow_both_pkeys;

	/* TODO: add support for changes in SLVL, PKEYs and in future for QoS and LFTs */
	uint8_t dirty;
};

struct ssa_db_diff *ssa_db_diff_init();
void ssa_db_diff_destroy(struct ssa_db_diff * p_ssa_db_diff);
struct ssa_db_diff *ssa_db_compare(struct ssa_events * ssa,
				   struct ssa_db * p_previous_db,
				   struct ssa_db * p_current_db);
void ep_guid_to_lid_qmap_copy(cl_qmap_t * p_dest_qmap, cl_qmap_t * p_src_qmap);
void ep_node_qmap_copy(cl_qmap_t * p_dest_qmap, cl_qmap_t * p_src_qmap);

END_C_DECLS
#endif				/* _SSA_COMPARISON_H_ */
