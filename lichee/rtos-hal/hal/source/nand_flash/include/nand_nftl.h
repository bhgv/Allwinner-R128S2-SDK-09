/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __NAND_NFTL_H__
#define __NAND_NFTL_H__

#include "nand_cfg.h"

struct _nand_partition;
struct _nftl_zone;
struct _nftl_cfg;

struct _nftl_blk {
	struct _nand_partition	*nand;
	struct _nftl_blk *nftl_blk_next;
	struct _nftl_zone *nftl_zone;
	struct _nftl_cfg *nftl_cfg;
	unsigned int logic_sects;
	unsigned int logic_page_sects;
	unsigned int ver_main:8;
	unsigned int ver_mid:8;
	unsigned int ver_sub:8;

	int (*read_data) (struct _nftl_blk *nftl_blk, unsigned int block,
			unsigned int nblk, unsigned char *buf);
	int (*write_data) (struct _nftl_blk *nftl_blk, unsigned int block,
			unsigned int nblk, unsigned char *buf);
	int (*flush_write_cache) (struct _nftl_blk *nftl_blk,unsigned int num);
	int (*discard) (struct _nftl_blk *nftl_blk, unsigned int block,
			unsigned int nblk);
	int (*reclaim) (struct _nftl_blk *start_blk);
	int (*garbage_collect) (struct _nftl_blk *start_blk);
	int (*dynamic_gc)(struct _nftl_blk *nftl_blk, int is_deep);
	int (*gc_stat)(struct _nftl_blk *nftl_blk, char *buf, int len);
	int (*badblk)(struct _nftl_blk *nftl_blk);
	int (*static_wear_leveling)(struct _nftl_blk *nftl_blk);
	int (*shutdown_op) (struct _nftl_blk *nftl_blk);
};

int nftl_init(struct _nftl_blk *nftl_blk, int _nftl_blk_no);
int nftl_exit(struct _nftl_blk *nftl_blk);

#endif
