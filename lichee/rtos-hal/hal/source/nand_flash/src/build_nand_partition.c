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

/*****************************************************************************/
#define _BUILD_PARTITION_C_
/*****************************************************************************/

#include "nand_nftl.h"
#include "nand_osal.h"
#include "nand_struct.h"
#include "build_phy_partition.h"
#include "nand_physic_interface.h"

void nftl_free(const void *ptr);

/*****************************************************************************/

extern unsigned short cur_w_lb_no, cur_w_pb_no, cur_w_p_no, cur_e_lb_no;


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void change_block_addr(struct _nand_partition *nand, struct _nand_super_block *super_block, unsigned short nBlkNum)
{
	super_block->Chip_NO = nand->phy_partition->StartBlock.Chip_NO;

	super_block->Block_NO = nand->phy_partition->StartBlock.Block_NO + nBlkNum;

	while (super_block->Block_NO >= nand->phy_partition->nand_info->BlkPerChip) {
		super_block->Chip_NO++;
		super_block->Block_NO  -= nand->phy_partition->nand_info->BlkPerChip;
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_erase_superblk(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	change_block_addr(nand, &super_block, p->phy_page.Block_NO);
	cur_e_lb_no = p->phy_page.Block_NO;
	ret = nand->phy_partition->block_erase(super_block.Chip_NO, super_block.Block_NO);

	return ret;

}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_read_page(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	ret = nand->phy_partition->page_read(super_block.Chip_NO, super_block.Block_NO, p->phy_page.Page_NO, p->page_bitmap, p->main_data_addr, p->spare_data_addr);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/

static int nand_write_page(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	change_block_addr(nand, &super_block, p->phy_page.Block_NO);
	cur_w_lb_no = p->phy_page.Block_NO;
	cur_w_pb_no = super_block.Block_NO;
	cur_w_p_no = p->phy_page.Page_NO;
	ret = nand->phy_partition->page_write(super_block.Chip_NO, super_block.Block_NO, p->phy_page.Page_NO, p->page_bitmap, p->main_data_addr, p->spare_data_addr);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_is_blk_good(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	ret = 0;
	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	ret = is_factory_bad_block(nand->phy_partition->nand_info, super_block.Chip_NO, super_block.Block_NO);
	if (ret != 0) {
		return 0;
	}
	ret = is_new_bad_block(nand->phy_partition->nand_info, super_block.Chip_NO, super_block.Block_NO);
	if (ret != 0) {
		return 0;
	}

	return 1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_mark_bad_blk(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];

	MEMSET(spare, 0, BYTES_OF_USER_PER_PAGE);
	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	NAND_PhysicLock();
	ret = PHY_VirtualBadBlockMark(super_block.Chip_NO, super_block.Block_NO);
	NAND_PhysicUnLock();

	add_new_bad_block(nand->phy_partition->nand_info, super_block.Chip_NO, super_block.Block_NO);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
struct _nand_partition *build_nand_partition(struct _nand_phy_partition *phy_partition)
{
	struct _nand_partition  *partition;

	partition = MALLOC(sizeof(struct _nand_partition));
	if (partition == NULL) {
		NFTL_ERR("[NE]%s:malloc fail for partition\n", __func__);
		return NULL;
	}
	partition->phy_partition       = phy_partition;
	partition->sectors_per_page    = phy_partition->SectorNumsPerPage;
	partition->spare_bytes         = phy_partition->BytesUserData;
	partition->pages_per_block     = phy_partition->PageNumsPerBlk;
	partition->bytes_per_page      = partition->sectors_per_page << 9;
	partition->bytes_per_block     = partition->bytes_per_page * partition->pages_per_block;
	partition->full_bitmap         = phy_partition->FullBitmapPerPage;

	partition->cap_by_sectors      = phy_partition->TotalSectors;
	partition->cap_by_bytes        = partition->cap_by_sectors << 9;
	partition->total_blocks        = phy_partition->TotalBlkNum - phy_partition->FreeBlock;

	partition->total_by_bytes      = partition->total_blocks * partition->bytes_per_block;

	partition->nand_erase_superblk = nand_erase_superblk;
	partition->nand_read_page      = nand_read_page;
	partition->nand_write_page     = nand_write_page;
	partition->nand_is_blk_good    = nand_is_blk_good;
	partition->nand_mark_bad_blk   = nand_mark_bad_blk;

	MEMCPY(partition->name, "nand_partition0", 31);
	partition->name[14] += phy_partition->PartitionNO;
	NFTL_DBG("[ND]%s\n", partition->name);

	return partition;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int free_nand_partition(struct _nand_partition *nand_partition)
{
	if (!nand_partition)
		return -1;

	if (nand_partition->phy_partition) {
		free_phy_partition(nand_partition->phy_partition);
		nand_partition->phy_partition = NULL;
	}
	nftl_free(nand_partition);

	return 0;
}

