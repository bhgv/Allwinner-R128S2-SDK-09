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
#define _BUILD_PHY_PARTITION_C_
/*****************************************************************************/
#include "nand_nftl.h"
#include "nand_cfg.h"
#include "nand_osal.h"
#include "nand_struct.h"
#include "nand_info_init.h"
#include "build_phy_partition.h"
/*****************************************************************************/

extern int PHY_VirtualPageRead(unsigned int nDieNum, unsigned int nBlkNum, unsigned int nPage, uint64 SectBitmap, void *pBuf, void *pSpare);
extern int PHY_VirtualPageWrite(unsigned int nDieNum, unsigned int nBlkNum, unsigned int nPage, uint64 SectBitmap, void *pBuf, void *pSpare);
extern int PHY_VirtualBlockErase(unsigned int nDieNum, unsigned int nBlkNum);
extern int PHY_VirtualBadBlockCheck(unsigned int nDieNum, unsigned int nBlkNum);
extern int PHY_VirtualBadBlockMark(unsigned int nDieNum, unsigned int nBlkNum);

static void change_partition(struct _partition *partition);

void *nftl_malloc(uint32 size);
void nftl_free(const void *ptr);

/*Interfaces are defined but not used, mainly to eliminate compilation warnings*/
#define __maybe_unused		__attribute__((unused))

static uint32 get_phy_partition_num(struct _nand_info *nand_info) __maybe_unused;
static struct _nand_phy_partition *get_phy_partition_from_num(struct _nand_info *nand_info, uint32 num) __maybe_unused;
static int erase_phy_partition(struct _nand_info *nand_info, struct _nand_phy_partition *phy_partition) __maybe_unused;

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static uint64 bitmap_change(unsigned short SectBitmap)
{
	uint64 bitmap;
	int i;
	for (i = 0, bitmap = 0; i < SectBitmap; i++) {
		bitmap <<= 1;
		bitmap |= 0x01;
	}

	return bitmap;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int PageRead(unsigned short nDieNum, unsigned short nBlkNum, unsigned short nPage, unsigned short SectBitmap, void *pBuf, void *pSpare)
{
	uint64 bitmap;
	int ret;
	NAND_PhysicLock();
	bitmap = bitmap_change(SectBitmap);
	ret = PHY_VirtualPageRead(nDieNum, nBlkNum, nPage, bitmap, pBuf, pSpare);
	NAND_PhysicUnLock();

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int PageWrite(unsigned short nDieNum, unsigned short nBlkNum, unsigned short nPage, unsigned short SectBitmap, void *pBuf, void *pSpare)
{
	uint64 bitmap;
	int ret;
	NAND_PhysicLock();
	bitmap = bitmap_change(SectBitmap);
	ret = PHY_VirtualPageWrite(nDieNum, nBlkNum, nPage, bitmap, pBuf, pSpare);
	NAND_PhysicUnLock();

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int BlockErase(unsigned short nDieNum, unsigned short nBlkNum)
{
	int ret;
	NAND_PhysicLock();
	ret = PHY_VirtualBlockErase(nDieNum, nBlkNum);
	NAND_PhysicUnLock();

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int BlockCheck(unsigned short nDieNum, unsigned short nBlkNum)
{
	int ret = 0;

	NAND_PhysicLock();
	ret = PHY_VirtualBadBlockCheck(nDieNum, nBlkNum);
	NAND_PhysicUnLock();

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_factory_bad_block(struct _nand_info *nand_info, unsigned short nDieNum, unsigned short nBlkNum)
{
	uint32 num, i;

	num = FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	for (i = 0; i < num; i++) {
		if ((nand_info->factory_bad_block[i].Chip_NO == nDieNum) && (nand_info->factory_bad_block[i].Block_NO == nBlkNum)) {
			return 1;
		}
		if (nand_info->factory_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_new_bad_block(struct _nand_info *nand_info, unsigned short nDieNum, unsigned short nBlkNum)
{
	uint32 num, i;

	num = PHY_PARTITION_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	for (i = 0; i < num; i++) {
		if ((nand_info->new_bad_block[i].Chip_NO == nDieNum) && (nand_info->new_bad_block[i].Block_NO == nBlkNum)) {
			return 1;
		}
		if (nand_info->new_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int add_new_bad_block(struct _nand_info *nand_info, unsigned short nDieNum, unsigned short nBlkNum)
{
	uint32 num, i;

	num = PHY_PARTITION_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	for (i = 0; i < num; i++) {
		if (nand_info->new_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}
	if (i < num) {
		nand_info->new_bad_block[i].Chip_NO = nDieNum;
		nand_info->new_bad_block[i].Block_NO = nBlkNum;

	} else {
		NFTL_ERR("[NE]Too much new bad block\n");
	}
	NAND_PhysicLock();
	write_new_block_table_v2_new(nand_info);
	NAND_PhysicUnLock();
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int put_factory_bad_block(struct _nand_info *nand_info, struct _nand_phy_partition *phy_partition)
{
	int count, i, j;

	count = FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);

	for (i = 0; i < count; i++) {
		if (nand_info->factory_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}

	if (i == count) {
		NFTL_ERR("[NE]too much bad block\n");
		return -1;
	}

	for (j = 0; i < count; i++, j++) {
		if (phy_partition->factory_bad_block[j].Chip_NO != 0xffff) {
			nand_info->factory_bad_block[i].Chip_NO = phy_partition->factory_bad_block[j].Chip_NO;
			nand_info->factory_bad_block[i].Block_NO = phy_partition->factory_bad_block[j].Block_NO;
		} else {
			break;
		}
	}
	return 0;
}


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int free_phy_partition(struct _nand_phy_partition *phy_partition)
{
	if (phy_partition->factory_bad_block) {
		nftl_free(phy_partition->factory_bad_block);
		phy_partition->factory_bad_block = NULL;
	}
	if (phy_partition->new_bad_block) {
		nftl_free(phy_partition->new_bad_block);
		phy_partition->new_bad_block = NULL;
	}
	nftl_free(phy_partition);
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void print_phy_partition(struct _nand_phy_partition *phy_partition)
{
	NFTL_DBG("[ND]phy_partition->PartitionNO :%d\n", phy_partition->PartitionNO);
	NFTL_DBG("[ND]phy_partition->SectorNumsPerPage :%d\n", phy_partition->SectorNumsPerPage);
	NFTL_DBG("[ND]phy_partition->PageNumsPerBlk :%d\n", phy_partition->PageNumsPerBlk);
	NFTL_DBG("[ND]phy_partition->TotalBlkNum :%d\n", phy_partition->TotalBlkNum);

	NFTL_DBG("[ND]phy_partition->FullBitmapPerPage :%d\n", phy_partition->FullBitmapPerPage);
	NFTL_DBG("[ND]phy_partition->FreeBlock :%d\n", phy_partition->FreeBlock);
	NFTL_DBG("[ND]phy_partition->TotalSectors :%d\n", phy_partition->TotalSectors);
	NFTL_DBG("[ND]phy_partition->StartBlock.Chip_NO :%d\n", phy_partition->StartBlock.Chip_NO);
	NFTL_DBG("[ND]phy_partition->StartBlock.Block_NO :%d\n", phy_partition->StartBlock.Block_NO);
	NFTL_DBG("[ND]phy_partition->EndBlock.Chip_NO :%d\n", phy_partition->EndBlock.Chip_NO);
	NFTL_DBG("[ND]phy_partition->EndBlock.Block_NO :%d\n", phy_partition->EndBlock.Block_NO);
	NFTL_DBG("[ND]phy_partition->next_phy_partition :%d\n", phy_partition->next_phy_partition);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void print_all_bad_block(struct _nand_phy_partition *phy_partition)
{
	int i, num;
	num = FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	NFTL_DBG("[ND]phy_partition->PartitionNO %d  FACTORY BAD BLOCK:\n", phy_partition->PartitionNO);
	for (i = 0; i < num; i++) {
		if (phy_partition->factory_bad_block[i].Chip_NO != 0xffff) {
			NFTL_DBG("[ND]BAD Chip:%d;Block:%d.\n", phy_partition->factory_bad_block[i].Chip_NO, phy_partition->factory_bad_block[i].Block_NO);
		} else {
			break;
		}
	}

	num = PHY_PARTITION_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	NFTL_DBG("[ND]phy_partition->PartitionNO %d  NEW BAD BLOCK:\n", phy_partition->PartitionNO);
	for (i = 0; i < num; i++) {
		if (phy_partition->new_bad_block[i].Chip_NO != 0xffff) {
			NFTL_DBG("[ND]BAD Chip:%d;Block:%d.\n", phy_partition->new_bad_block[i].Chip_NO, phy_partition->new_bad_block[i].Block_NO);
		} else {
			break;
		}
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
struct _nand_disk *get_disk_from_phy_partition(struct _nand_phy_partition *phy_partition)
{
	return phy_partition->disk;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint16 get_partitionNO(struct _nand_phy_partition *phy_partition)
{
	return phy_partition->PartitionNO;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static uint32 get_phy_partition_num(struct _nand_info *nand_info)
{
	uint32 num = 0;

	struct _nand_phy_partition *p;

	p = nand_info->phy_partition_head;
	while (p != NULL) {
		num++;
		p = p->next_phy_partition;
	}
	return num;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static struct _nand_phy_partition *get_phy_partition_from_num(struct _nand_info *nand_info, uint32 num)
{
	uint32 i = 0;
	struct _nand_phy_partition *p;

	p = nand_info->phy_partition_head;
	for (i = 0; i < num; i++) {
		p = p->next_phy_partition;
	}
	return p;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int erase_phy_partition(struct _nand_info *nand_info, struct _nand_phy_partition *phy_partition)
{
	unsigned short nDieNum, nBlkNum;
	int i, ret;
	uint32 total_blocks = phy_partition->TotalBlkNum;

	nDieNum = phy_partition->StartBlock.Chip_NO;
	nBlkNum = phy_partition->StartBlock.Block_NO;

	NFTL_DBG("[NE]erase_whole_phy_partition start! %d %d %d\n", total_blocks, nDieNum, nBlkNum);

	for (i = 0; i < total_blocks; i++) {
		if (BlockCheck(nDieNum, nBlkNum) == 0) {
			ret = phy_partition->block_erase(nDieNum, nBlkNum);
			if (ret != 0) {
				NFTL_ERR("[NE]erase_whole_phy_partition error %d %d\n", nDieNum, nBlkNum);
			}
		}

		nBlkNum++;
		if (nBlkNum == nand_info->BlkPerChip) {
			nBlkNum = 0;
			nDieNum++;
			if (nDieNum == nand_info->ChipNum) {
				break;
			}
		}
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int get_max_free_block_num(struct _nand_info *nand_info, struct _nand_phy_partition *phy_partition)
{
	int max_free_block_num, sector_per_block;
	uint32 total_sector;

	sector_per_block = phy_partition->PageNumsPerBlk * phy_partition->SectorNumsPerPage;
	total_sector = sector_per_block * nand_info->BlkPerChip;
	total_sector *= nand_info->ChipNum;

	if (total_sector <= 0x64000) { //less than 200MB
		max_free_block_num = 40;
	} else if (total_sector <= 0x96000) { //less than 300MB
		max_free_block_num = 85;
	} else if (total_sector <= 0x12c000) { //less than 600MB
		if (sector_per_block >= 2048) {
			max_free_block_num = 40;
		} else if (sector_per_block >= 1024) {
			max_free_block_num = 80;
		} else {
			max_free_block_num = 180;
		}
	} else if (total_sector <= 0x258000) { //less than 1200MB
		if (sector_per_block >= 1024) {
			max_free_block_num = 170;
		} else {
			max_free_block_num = 320;
		}
	} else if (total_sector <= 0xa00000) { //less than 5GB
		if (sector_per_block >= 16384) {
			max_free_block_num = 50;
		} else if (sector_per_block >= 8182) {
			max_free_block_num = 75;
		} else if (sector_per_block >= 4096) {
			max_free_block_num = 170;
		} else {
			max_free_block_num = 300;
		}
	} else if (total_sector <= 0x1400000) { //less than 10GB
		if (sector_per_block >= 16384) {
			max_free_block_num = 75;
		} else if (sector_per_block >= 8182) {
			max_free_block_num = 160;
		} else if (sector_per_block >= 4096) {
			max_free_block_num = 300;
		} else {
			max_free_block_num = 600;
		}
	} else if (total_sector <= 0x2800000) { //less than 20GB
		if (sector_per_block >= 32768) {
			max_free_block_num = 70;
		} else if (sector_per_block >= 16384) {
			max_free_block_num = 150;
		} else if (sector_per_block >= 8182) {
			max_free_block_num = 160;
		} else {
			max_free_block_num = 240;
		}
	} else {         //32G
		if (sector_per_block >= 32768) {
			max_free_block_num = 140;
		} else if (sector_per_block >= 16384) {
			max_free_block_num = 250;
		} else {
			max_free_block_num = 500;
		}
	}

	return max_free_block_num;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static struct _nand_phy_partition *mp_build_phy_partition(struct _nand_info *nand_info, struct _partition *part)
{
	struct _nand_phy_partition *phy_partition;
	struct _nand_phy_partition *p;
	uint32 total_blocks, temp1, temp2, factory_bad_count, new_bad_count;
	unsigned short nDieNum, nBlkNum, PartitionNO, last_phy_partition;
	int max_free_block_num;

	phy_partition = (struct _nand_phy_partition *)nftl_malloc(sizeof(struct _nand_phy_partition));
	if (phy_partition == NULL) {
		NFTL_ERR("[NE]%s:malloc fail for phy_partition\n", __func__);
		return NULL;
	}

	phy_partition->next_phy_partition = NULL;
	if (nand_info->phy_partition_head == NULL) {
		nand_info->phy_partition_head = phy_partition;
		phy_partition->PartitionNO = 0;
		phy_partition->StartBlock.Chip_NO = nand_info->no_used_block_addr.Chip_NO;
		phy_partition->StartBlock.Block_NO = nand_info->no_used_block_addr.Block_NO;
	} else {
		PartitionNO = 1;
		p = nand_info->phy_partition_head;
		while (p->next_phy_partition != NULL) {
			p = p->next_phy_partition;
			PartitionNO ++;
		}
		p->next_phy_partition = phy_partition;
		phy_partition->PartitionNO = PartitionNO;

		phy_partition->StartBlock.Chip_NO = p->EndBlock.Chip_NO;
		phy_partition->StartBlock.Block_NO = p->EndBlock.Block_NO + 1;

		while (phy_partition->StartBlock.Block_NO >= nand_info->BlkPerChip) {
			phy_partition->StartBlock.Chip_NO ++;
			phy_partition->StartBlock.Block_NO -= nand_info->BlkPerChip;
			if (phy_partition->StartBlock.Chip_NO >= nand_info->ChipNum) {
				NFTL_ERR("[NE]no block2 %d\n", phy_partition->StartBlock.Chip_NO);
				return NULL;
			}
		}
	}

	phy_partition->Attribute = part->attribute;
	phy_partition->CrossTalk = part->cross_talk;
	phy_partition->page_read = PageRead;
	phy_partition->page_write = PageWrite;
	phy_partition->block_erase = BlockErase;

	phy_partition->nand_info = nand_info;
	phy_partition->SectorNumsPerPage = nand_info->SectorNumsPerPage;
	phy_partition->BytesUserData = nand_info->BytesUserData;
	phy_partition->PageNumsPerBlk = nand_info->PageNumsPerBlk;
	phy_partition->FullBitmapPerPage = phy_partition->SectorNumsPerPage;

	phy_partition->EndBlock.Chip_NO = phy_partition->StartBlock.Chip_NO;
	phy_partition->EndBlock.Block_NO = phy_partition->StartBlock.Block_NO;

	if (part->size != 0xffffffff) {
		phy_partition->TotalSectors = part->size;
		temp1 = part->size / phy_partition->SectorNumsPerPage;
		temp1 /= phy_partition->PageNumsPerBlk;
		total_blocks = temp1 + temp1 / phy_partition->PageNumsPerBlk; //logic block num

		NFTL_DBG("[NE]phy_partition Attribute %d\n", phy_partition->Attribute);
		if (phy_partition->Attribute == 0) {
			temp2 = temp1 / (NORM_RESERVED_BLOCK_RATIO + nand_info->capacity_level * 3);
		} else {
			temp2 = temp1 / (SYS_RESERVED_BLOCK_RATIO + nand_info->capacity_level * 3);
		}

		if (temp2 < nand_info->mini_free_block_first_reserved) {
			temp2 = nand_info->mini_free_block_first_reserved;
		}

		max_free_block_num = get_max_free_block_num(nand_info, phy_partition);
		if (temp2 > max_free_block_num) {
			temp2 = max_free_block_num;
		}
		total_blocks += temp2;     // add nftl free block

		phy_partition->FreeBlock = MIN_PHY_RESERVED_BLOCK_V2;

		total_blocks += phy_partition->FreeBlock;   // add phy free block
		last_phy_partition = 0;
	} else {
		total_blocks = 0xffffffff;
		last_phy_partition = 1;
	}

	phy_partition->TotalBlkNum = 0;
	phy_partition->factory_bad_block = nftl_malloc(FACTORY_BAD_BLOCK_SIZE);
	if (phy_partition->factory_bad_block == NULL) {
		NFTL_ERR("[NE]%s:malloc fail for factory_bad_block\n", __func__);
		return NULL;
	}

	MEMSET(phy_partition->factory_bad_block, 0xff, FACTORY_BAD_BLOCK_SIZE);

	phy_partition->new_bad_block = nftl_malloc(PHY_PARTITION_BAD_BLOCK_SIZE);
	if (phy_partition->new_bad_block == NULL) {
		NFTL_ERR("[NE]%s:malloc fail for new_bad_block\n", __func__);
		return NULL;
	}

	MEMSET(phy_partition->new_bad_block, 0xff, PHY_PARTITION_BAD_BLOCK_SIZE);

	factory_bad_count = 0;
	new_bad_count = 0;
	nDieNum = phy_partition->StartBlock.Chip_NO;
	nBlkNum = phy_partition->StartBlock.Block_NO;
	while (total_blocks != 0) {
		if (nand_info->FirstBuild == 1) {
			if (BlockCheck(nDieNum, nBlkNum) == 0) {
				total_blocks--;
			} else {
				phy_partition->factory_bad_block[factory_bad_count].Chip_NO = nDieNum;
				phy_partition->factory_bad_block[factory_bad_count].Block_NO = nBlkNum;
				factory_bad_count++;
				if (factory_bad_count == FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block)) {
					NFTL_ERR("[NE]too much bad block %d\n", factory_bad_count);
					nftl_free(phy_partition);
					return NULL;
				}
			}
		} else {
			if (is_factory_bad_block(nand_info, nDieNum, nBlkNum) == 1) {
				NFTL_DBG("[ND]factory bad block:%d,%d PartitionNO:%d\n", nDieNum, nBlkNum, phy_partition->PartitionNO);
				phy_partition->factory_bad_block[factory_bad_count].Chip_NO = nDieNum;
				phy_partition->factory_bad_block[factory_bad_count].Block_NO = nBlkNum;
				factory_bad_count++;
				if (factory_bad_count == FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block)) {
					NFTL_ERR("[NE]too much factory bad block %d\n", factory_bad_count);
					nftl_free(phy_partition);
					return NULL;
				}
			} else if (is_new_bad_block(nand_info, nDieNum, nBlkNum) == 1) {
				NFTL_DBG("[ND]new bad block:%d,%d PartitionNO:%d\n", nDieNum, nBlkNum, phy_partition->PartitionNO);
				phy_partition->new_bad_block[new_bad_count].Chip_NO = nDieNum;
				phy_partition->new_bad_block[new_bad_count].Block_NO = nBlkNum;
				new_bad_count++;
				if (new_bad_count == PHY_PARTITION_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block)) {
					NFTL_ERR("[NE]too much new bad block %d\n", factory_bad_count);
					nftl_free(phy_partition);
					return NULL;
				}
			} else {
				total_blocks--;
			}
		}
		phy_partition->EndBlock.Chip_NO = nDieNum;
		phy_partition->EndBlock.Block_NO = nBlkNum;
		phy_partition->TotalBlkNum++;
		nBlkNum++;
		if (nBlkNum == nand_info->BlkPerChip) {
			nBlkNum = 0;
			nDieNum++;
			if (nDieNum == nand_info->ChipNum) {
				break;
			}
		}
	}

	if (last_phy_partition == 1) {
		/*  Don't need to reduce factory_bad_count, otherwise the part size of UDISK can not match witch the filesystem size of littlefs superblock. */
		/*  The fatory bad block will use reserved block directly. */
		/*  total_blocks = phy_partition->TotalBlkNum - factory_bad_count; */
		total_blocks = phy_partition->TotalBlkNum;

		phy_partition->FreeBlock = MIN_PHY_RESERVED_BLOCK_V2;
		total_blocks -= phy_partition->FreeBlock;

		if (phy_partition->Attribute == 0) {
			temp2 = total_blocks / (NORM_RESERVED_BLOCK_RATIO + nand_info->capacity_level * 3);
		} else {
			temp2 = total_blocks / (SYS_RESERVED_BLOCK_RATIO + nand_info->capacity_level * 3);
		}

		if (temp2 < nand_info->mini_free_block_first_reserved) {
			temp2 = nand_info->mini_free_block_first_reserved;
		}

		max_free_block_num = get_max_free_block_num(nand_info, phy_partition);
		if (temp2 > max_free_block_num) {
			temp2 = max_free_block_num;
		}
		total_blocks -= temp2;

		temp1 = total_blocks * phy_partition->PageNumsPerBlk;
		temp1 *= phy_partition->SectorNumsPerPage;
		phy_partition->TotalSectors = temp1;
		part->size = phy_partition->TotalSectors;
	}

	if (nand_info->FirstBuild == 1) {
		put_factory_bad_block(nand_info, phy_partition);
	}

	phy_partition->disk = nand_info->partition[phy_partition->PartitionNO].nand_disk;


	part->start.Chip_NO = phy_partition->StartBlock.Chip_NO;
	part->start.Block_NO = phy_partition->StartBlock.Block_NO;

	part->end.Chip_NO = phy_partition->EndBlock.Chip_NO;
	part->end.Block_NO = phy_partition->EndBlock.Block_NO;

	change_partition(part);
	print_partition(part);

	print_phy_partition(phy_partition);
	print_all_bad_block(phy_partition);

	return phy_partition;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static struct _nand_phy_partition *launch_build_phy_partition(struct _nand_info *nand_info, struct _partition *part)
{
	struct _nand_phy_partition *phy_partition;
	struct _nand_phy_partition *p;
	uint32 temp1, factory_bad_count, new_bad_count;
	unsigned short nDieNum, nBlkNum, PartitionNO;
	

	phy_partition = (struct _nand_phy_partition *)nftl_malloc(sizeof(struct _nand_phy_partition));
	if (phy_partition == NULL) {
		NFTL_ERR("[NE]%s:malloc fail for phy_partition\n", __func__);
		return NULL;
	}

	phy_partition->next_phy_partition = NULL;
	if (nand_info->phy_partition_head == NULL) {
		nand_info->phy_partition_head = phy_partition;
		phy_partition->PartitionNO = 0;
	} else {
		PartitionNO = 1;
		p = nand_info->phy_partition_head;
		while (p->next_phy_partition != NULL) {
			p = p->next_phy_partition;
			PartitionNO ++;
		}
		p->next_phy_partition = phy_partition;
		phy_partition->PartitionNO = PartitionNO;
	}

	phy_partition->StartBlock.Chip_NO = part->start.Chip_NO;
	phy_partition->StartBlock.Block_NO = part->start.Block_NO;
	phy_partition->EndBlock.Chip_NO = part->end.Chip_NO;
	phy_partition->EndBlock.Block_NO = part->end.Block_NO;
	phy_partition->TotalSectors = part->size;
	phy_partition->Attribute = part->attribute;
	phy_partition->CrossTalk = part->cross_talk;
	phy_partition->page_read = PageRead;
	phy_partition->page_write = PageWrite;
	phy_partition->block_erase = BlockErase;
	phy_partition->disk = part->nand_disk;

	phy_partition->nand_info = nand_info;
	phy_partition->SectorNumsPerPage = nand_info->SectorNumsPerPage;
	phy_partition->BytesUserData = nand_info->BytesUserData;
	phy_partition->PageNumsPerBlk = nand_info->PageNumsPerBlk;
	phy_partition->FullBitmapPerPage = phy_partition->SectorNumsPerPage;
	phy_partition->FreeBlock = MIN_PHY_RESERVED_BLOCK_V2;
	phy_partition->TotalBlkNum = 0;
	phy_partition->GoodBlockNum = 0;

	phy_partition->factory_bad_block = nftl_malloc(FACTORY_BAD_BLOCK_SIZE);
	if (phy_partition->factory_bad_block == NULL) {
		NFTL_ERR("[NE]%s:malloc fail for factory_bad_block\n", __func__);
		return NULL;
	}

	MEMSET(phy_partition->factory_bad_block, 0xff, FACTORY_BAD_BLOCK_SIZE);

	phy_partition->new_bad_block = nftl_malloc(PHY_PARTITION_BAD_BLOCK_SIZE);
	if (phy_partition->new_bad_block == NULL) {
		NFTL_ERR("[NE]%s:malloc fail for new_bad_block\n", __func__);
		return NULL;
	}

	MEMSET(phy_partition->new_bad_block, 0xff, PHY_PARTITION_BAD_BLOCK_SIZE);

	factory_bad_count = 0;
	new_bad_count = 0;
	nDieNum = phy_partition->StartBlock.Chip_NO;
	nBlkNum = phy_partition->StartBlock.Block_NO;
	while (1) {
		if (is_factory_bad_block(nand_info, nDieNum, nBlkNum) == 1) {
			NFTL_DBG("[ND]factory bad block:%d,%d PartitionNO:%d\n", nDieNum, nBlkNum, phy_partition->PartitionNO);
			phy_partition->factory_bad_block[factory_bad_count].Chip_NO = nDieNum;
			phy_partition->factory_bad_block[factory_bad_count].Block_NO = nBlkNum;
			factory_bad_count++;
			if (factory_bad_count == FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block)) {
				NFTL_ERR("[NE]too much factory bad block %d\n", factory_bad_count);
				nftl_free(phy_partition);
				return NULL;
			}
		} else if (is_new_bad_block(nand_info, nDieNum, nBlkNum) == 1) {
			NFTL_DBG("[ND]new bad block:%d,%d PartitionNO:%d\n", nDieNum, nBlkNum, phy_partition->PartitionNO);
			phy_partition->new_bad_block[new_bad_count].Chip_NO = nDieNum;
			phy_partition->new_bad_block[new_bad_count].Block_NO = nBlkNum;
			new_bad_count++;
			if (new_bad_count == PHY_PARTITION_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block)) {
				NFTL_ERR("[NE]too much new bad block %d\n", factory_bad_count);
				nftl_free(phy_partition);
				return NULL;
			}
		} else {
			phy_partition->GoodBlockNum++;
		}
		phy_partition->TotalBlkNum++;

		if ((nDieNum == phy_partition->EndBlock.Chip_NO) && (nBlkNum == phy_partition->EndBlock.Block_NO)) {
			break;
		}
		nBlkNum++;
		if (nBlkNum == nand_info->BlkPerChip) {
			nBlkNum = 0;
			nDieNum++;
			if (nDieNum == nand_info->ChipNum) {
				break;
			}
		}
	}

	//check
	temp1 = phy_partition->TotalSectors / phy_partition->SectorNumsPerPage;
	temp1 /= phy_partition->PageNumsPerBlk;
	temp1 /= PLANE_CNT_OF_DIE;

	if (phy_partition->GoodBlockNum > temp1) {
		if ((phy_partition->GoodBlockNum - temp1) < MIN_FREE_BLOCK_NUM_RUNNING) {
			NFTL_ERR("[ND]not enough block:%d,%d!\n", phy_partition->GoodBlockNum, temp1);
			nftl_free(phy_partition);
			return NULL;
		}
	} else {
		NFTL_ERR("[ND]not enough block:%d,%d!!\n", phy_partition->GoodBlockNum, temp1);
		nftl_free(phy_partition);
		return NULL;
	}
	print_partition(part);

	print_phy_partition(phy_partition);
	print_all_bad_block(phy_partition);

	return phy_partition;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
struct _nand_phy_partition *build_phy_partition_v2(struct _nand_info *nand_info, struct _partition *part)
{
	if (nand_info->FirstBuild == 1) {
		return mp_build_phy_partition(nand_info, part);
	} else {
		return launch_build_phy_partition(nand_info, part);
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void print_partition(struct _partition *partition)
{
	int i;

	NFTL_DBG("[ND]partition->size :%d\n", partition->size);
	NFTL_DBG("[ND]partition->cross_talk :%d\n", partition->cross_talk);
	NFTL_DBG("[ND]partition->attribute :%d\n", partition->attribute);

	NFTL_DBG("[ND]partition->start.Chip_NO :%d\n", partition->start.Chip_NO);
	NFTL_DBG("[ND]partition->start.Block_NO :%d\n", partition->start.Block_NO);
	NFTL_DBG("[ND]partition->end.Chip_NO :%d\n", partition->end.Chip_NO);
	NFTL_DBG("[ND]partition->end.Block_NO :%d\n", partition->end.Block_NO);

	for (i = 0; i < MAX_PART_COUNT_PER_FTL; i++) {
		if (partition->nand_disk[i].size == -1)
			break;
		NFTL_DBG("[ND]partition->nand_disk[%d].name :%s\n", i,
				partition->nand_disk[i].name);
		NFTL_DBG("[ND]partition->nand_disk[%d].size :%d\n", i,
				partition->nand_disk[i].size);
		NFTL_DBG("[ND]partition->nand_disk[%d].type :%d\n", i,
				partition->nand_disk[i].type);
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void change_partition(struct _partition *partition)
{
	uint32 size = 0, i;

	for (i = 0; i < MAX_PART_COUNT_PER_FTL; i++) {
		if ((partition->nand_disk[i].size != 0) && (partition->nand_disk[i].type != 0xffffffff)) {
			size += partition->nand_disk[i].size;
		}
		if ((partition->nand_disk[i].size == 0) && (partition->nand_disk[i].type != 0xffffffff)) {
			partition->nand_disk[i].size = partition->size - size;
			break;
		}
	}
}
