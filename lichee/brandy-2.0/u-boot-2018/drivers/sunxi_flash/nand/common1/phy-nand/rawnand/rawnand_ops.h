/*
 * rawnand_ops.c for sunxi rawnand ops
 *
 * Copyright (C) 2019 Allwinner.
 * SPDX-License-Identifier: GPL-2.0
 */
#ifndef __RAWNAND_OPS_H
#define __RAWNAND_OPS_H
//#include "rawnand.h"
#include <sunxi_nand.h>

#define NAND_OPEN_BLOCK_CNT (8)

struct nand_phy_write_lsb_cache {
	unsigned int cache_use_status;
	struct _nand_physic_op_par tmp_npo;
};

struct df_read_page_end {
	int (*read_page_end)(struct _nand_physic_op_par *npo);
};

struct rawnand_ops {
	int (*erase_single_block)(struct _nand_physic_op_par *npo);
	int (*write_single_page)(struct _nand_physic_op_par *npo);
	int (*read_single_page)(struct _nand_physic_op_par *npo);
	int (*single_bad_block_check)(struct _nand_physic_op_par *npo);
	int (*single_bad_block_mark)(struct _nand_physic_op_par *npo);
	int (*erase_super_block)(struct _nand_physic_op_par *npo);
	int (*write_super_page)(struct _nand_physic_op_par *npo);
	int (*read_super_page)(struct _nand_physic_op_par *npo);
	int (*super_bad_block_check)(struct _nand_physic_op_par *npo);
	int (*super_bad_block_mark)(struct _nand_physic_op_par *npo);
};

extern struct df_read_page_end df_read_page_end;
extern struct rawnand_ops rawnand_ops;
extern struct nand_phy_write_lsb_cache nand_phy_w_cache[NAND_OPEN_BLOCK_CNT];
#endif
