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
#define _HW_BUILD_C_
/*****************************************************************************/

#include "nand_osal.h"
#include "nand_struct.h"
#include "nand_nftl.h"
#include "nand_cfg.h"
#include "uboot_head.h"
#include "build_phy_partition.h"
#include "nand_info_init.h"
#include "nand_physic_interface.h"
#include "nand_common_info.h"
/*****************************************************************************/

static void print_boot_info(struct _nand_info *nand_info);
static unsigned int get_no_use_block_v3(struct _nand_info *nand_info, uchar chip, uint16 start_block);
static int check_mbr_v3(struct _nand_info *nand_info, PARTITION_MBR *mbr);
static void print_factory_block_table_v2(struct _nand_info *nand_info);
static void print_nand_info_v2(struct _nand_info *nand_info);
static int get_partition_v3(struct _nand_info *nand_info);
static int build_all_phy_partition_v2(struct _nand_info *nand_info);
static void print_mbr_data(uchar *mbr_data);
static void debug_read_chip(struct _nand_info *nand_info);

/*Interfaces are defined but not used, mainly to eliminate compilation warnings*/
#define __maybe_unused		__attribute__((unused))

static void print_mbr_data(uchar *mbr_data) __maybe_unused;
static int read_partition_v2(struct _nand_info *nand_info, unsigned int nDieNum, unsigned int nBlkNum) __maybe_unused;
static int read_mbr_v2(struct _nand_info *nand_info, unsigned int nDieNum, unsigned int nBlkNum) __maybe_unused;
static void debug_read_chip(struct _nand_info *nand_info) __maybe_unused;

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
struct _nand_phy_partition *get_head_phy_partition_from_nand_info(struct _nand_info *nand_info)
{
	return nand_info->phy_partition_head;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void set_cache_level(struct _nand_info *nand_info, unsigned short cache_level)
{
	nand_info->cache_level = cache_level;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void set_capacity_level(struct _nand_info *nand_info, unsigned short capacity_level)
{
	if (capacity_level != 0)
		nand_info->capacity_level = 1;
	else
		nand_info->capacity_level = 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static unsigned short debug_read_block(struct _nand_info *nand_info, unsigned int nDieNum, unsigned int nBlkNum)
{
	unsigned int i;

	unsigned char spare[BYTES_OF_USER_PER_PAGE];

	for (i = 0; i < nand_info->PageNumsPerBlk; i++) {
		MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
		PHY_VirtualPageRead(nDieNum, nBlkNum, i, nand_info->FullBitmap, nand_info->temp_page_buf, spare);
		NFTL_DBG("[ND]block:%d page:%d spare: %x %x %x %x %x %x %x %x %x %x\n", nBlkNum, i, spare[0], spare[1], spare[2], spare[3], spare[4], spare[5], spare[6], spare[7], spare[8], spare[9]);
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
static void debug_read_chip(struct _nand_info *nand_info)
{
	unsigned int i, j;

	for (i = 0; i < nand_info->ChipNum; i++) {
		NFTL_DBG("[ND]=============chip %d============================\n", i);
		for (j = 0; j < nand_info->BlkPerChip; j++) {
			debug_read_block(nand_info, i, j);
		}
	}
	NFTL_DBG("[ND]=================end========================\n");

}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int read_partition_v3(struct _nand_info *nand_info, struct _boot_info *boot)
{
	int i;

	MEMCPY(nand_info->partition, boot->partition.ndata, sizeof(nand_info->partition));

	nand_info->partition_nums = 0;
	for (i = 0; i < MAX_PARTITION; i++) {
		if ((nand_info->partition[i].size != 0) && (nand_info->partition[i].size != 0xffffffff)) {
			nand_info->partition_nums++;
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
static int nand_info_init_v3(struct _nand_info *nand_info, uchar chip, uint16 start_block, uchar *mbr_data)
{
	unsigned int nouse;
	
	struct _boot_info *boot;

	boot = nand_info->boot;

	MEMSET(nand_info->mbr_data, 0xff, sizeof(PARTITION_MBR));

	nand_info->mini_free_block_first_reserved = MIN_FREE_BLOCK_NUM_V2;
	nand_info->mini_free_block_reserved = MIN_FREE_BLOCK_REMAIN;
	nand_info->new_bad_page_addr = 0xffff;
	nand_info->FirstBuild = 0;

	if (mbr_data != NULL) {
		NFTL_DBG("[ND]factory FirstBuild %d\n", start_block);
		//print_boot_info(nand_info);
		if (nand_info->boot->mbr.data.PartCount == 0) {
			nouse = get_no_use_block_v3(nand_info, chip, start_block);
			if (nouse == 0xffffffff) {
				// nand3.0;  factory burn ; erase
				NFTL_DBG("[ND]erase factory FirstBuild\n");
				MEMCPY(nand_info->mbr_data, mbr_data, sizeof(PARTITION_MBR));
				nand_info->FirstBuild = 1;
				if (get_partition_v3(nand_info)) {
					NFTL_ERR("[NE]get partition v3 fail!!\n");
					return -1;
				}
				MEMCPY(boot->partition.ndata, nand_info->partition, sizeof(nand_info->partition));
				MEMCPY(boot->mbr.ndata, nand_info->mbr_data, sizeof(PARTITION_MBR));
				nand_info->boot->logic_start_block = start_block;
				nand_info->boot->no_use_block = nand_info->boot->logic_start_block;
			} else {
				// from nand2.0 to nand3.0 ; factory  burn;  not  erase
				NFTL_ERR("[NE]not support nand2.0\n");
				return -1;
			}
		} else {
			// nand3.0 to nand3.0 ; factory  burn ;   not erase
			NFTL_DBG("[ND]new not erase factory FirstBuild\n");
			MEMCPY(nand_info->mbr_data, boot->mbr.ndata, sizeof(PARTITION_MBR));
			read_partition_v3(nand_info, boot);
			MEMCPY(nand_info->factory_bad_block, boot->factory_block.ndata, FACTORY_BAD_BLOCK_SIZE);
			nand_info->boot->no_use_block = nand_info->boot->logic_start_block;
			if (check_mbr_v3(nand_info, (PARTITION_MBR *)mbr_data) != 0) {
				NFTL_ERR("[NE]not erase MP but mbr not the same 2!\n");
				return -1;
			}
		}
	} else {
		NFTL_DBG("[ND]boot start \n");
		print_boot_info(nand_info);
		//print_mbr_data(&boot->mbr.data);
		if (boot->mbr.data.PartCount == 0) {
			//first start nand3.0  from  nand2.0    from OTA
			NFTL_ERR("[NE]not support nand2.0\n");
			return -1;
		} else {
			//nand3.0 boot start
			MEMCPY(nand_info->mbr_data, boot->mbr.ndata, sizeof(PARTITION_MBR));
			read_partition_v3(nand_info, boot);
			MEMCPY(nand_info->factory_bad_block, boot->factory_block.ndata, FACTORY_BAD_BLOCK_SIZE);
		}
	}

	nand_info->no_used_block_addr.Chip_NO = chip;
	nand_info->no_used_block_addr.Block_NO = boot->no_use_block;

	if (build_all_phy_partition_v2(nand_info) != 0) {
		NFTL_ERR("[NE]build all phy partition fail!\n");
		return -1;
	}

	if (nand_info->FirstBuild == 1) {
		MEMCPY(boot->partition.ndata, nand_info->partition, sizeof(nand_info->partition));
		MEMCPY(boot->factory_block.ndata, nand_info->factory_bad_block, FACTORY_BAD_BLOCK_SIZE);
		print_factory_block_table_v2(nand_info);

		//print_mbr_data(&boot->mbr.data);
	}

	print_nand_info_v2(nand_info);

	print_boot_info(nand_info);
	//print_physic_info(nand_info->boot);
	//print_physic_info(phyinfo_buf);

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void print_boot_info(struct _nand_info *nand_info)
{
	

	NFTL_DBG("[ND]boot :0x%x\n", nand_info->boot);
	NFTL_DBG("[ND]boot->magic :0x%x\n", nand_info->boot->magic);
	NFTL_DBG("[ND]boot->len :0x%x\n", nand_info->boot->len);
	NFTL_DBG("[ND]boot->no_use_block :0x%x\n", nand_info->boot->no_use_block);
	NFTL_DBG("[ND]boot->uboot_start_block :0x%x\n", nand_info->boot->uboot_start_block);
	NFTL_DBG("[ND]boot->uboot_next_block :0x%x\n", nand_info->boot->uboot_next_block);
	NFTL_DBG("[ND]boot->logic_start_block :0x%x\n", nand_info->boot->logic_start_block);

	NFTL_DBG("[ND]mbr len :%d\n", sizeof(_MBR));
	NFTL_DBG("[ND]_PARTITION len :%d\n", sizeof(_PARTITION));
	NFTL_DBG("[ND]_NAND_STORAGE_INFO len :%d\n", sizeof(_NAND_STORAGE_INFO));
	NFTL_DBG("[ND]_FACTORY_BLOCK len :%d\n", sizeof(_FACTORY_BLOCK));

//    NFTL_DBG("============mbr===============\n");
//    for(i=-0;i<256;i++)
//    {
//        NFTL_DBG("%08x ",nand_info->boot->mbr.ndata[i]);
//        if(((i+1) % 16) == 0)
//        {
//            NFTL_DBG("\n");
//        }
//    }
//    NFTL_DBG("============partition===============\n");
//
//    for(i=-0;i<128;i++)
//    {
//        NFTL_DBG("%08x ",nand_info->boot->partition.ndata[i]);
//        if(((i+1) % 16) == 0)
//        {
//            NFTL_DBG("\n");
//        }
//    }
//
//    NFTL_DBG("===========storage_info================\n");
//    for(i=-0;i<128;i++)
//    {
//        NFTL_DBG("%08x ",nand_info->boot->storage_info.ndata[i]);
//        if(((i+1) % 16) == 0)
//        {
//            NFTL_DBG("\n");
//        }
//    }
//
//    NFTL_DBG("==============factory_block=============\n");
//    for(i=-0;i<256;i++)
//    {
//        NFTL_DBG("%08x ",nand_info->boot->factory_block.ndata[i]);
//        if(((i+1) % 16) == 0)
//        {
//            NFTL_DBG("\n");
//        }
//    }
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static unsigned int get_no_use_block_v3(struct _nand_info *nand_info, uchar chip, uint16 start_block)
{
	unsigned int i, nDieNum, nBlkNum, nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	


	nDieNum = chip;
	nBlkNum = start_block + 3;
	nPage = 0;
	SectBitmap = nand_info->FullBitmap;

	for (i = 0; i < 50; i++) {
		if (!BlockCheck(nDieNum, nBlkNum)) {
			PHY_VirtualPageRead(nDieNum, nBlkNum, nPage, SectBitmap, nand_info->temp_page_buf, spare);
			if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0xdd)) {
				return nBlkNum;
			}
		}
		nBlkNum++;
		if (nBlkNum == nand_info->BlkPerChip) {
			NFTL_ERR("[NE]1 can not find no use block !!\n");
			return 0xffffffff;
		}
	}
	return 0xffffffff;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_info_init(struct _nand_info *nand_info, uchar chip, uint16 start_block, uchar *mbr_data)
{

	int ret = -1;
	unsigned int bytes_per_page;
	int block_temp, real_start_block;

	bytes_per_page = nand_info->SectorNumsPerPage;
	bytes_per_page <<= 9;

	nand_info->temp_page_buf = MALLOC(bytes_per_page);
	if (nand_info->temp_page_buf == NULL)
		NFTL_ERR("[NE]%s: malloc fail for temp_page_buf!\n", __func__);

	nand_info->factory_bad_block = MALLOC(FACTORY_BAD_BLOCK_SIZE);
	if (nand_info->factory_bad_block == NULL)
		NFTL_ERR("[NE]%s: malloc fail for factory_bad_block!\n", __func__);

	nand_info->new_bad_block = MALLOC(PHY_PARTITION_BAD_BLOCK_SIZE);
	if (nand_info->new_bad_block == NULL)
		NFTL_ERR("[NE]%s: malloc fail for new_bad_block!\n", __func__);

	nand_info->mbr_data = MALLOC(sizeof(PARTITION_MBR));
	if (nand_info->mbr_data == NULL)
		NFTL_ERR("[NE]%s: malloc fail for mbr_data!\n", __func__);

	MEMSET(nand_info->factory_bad_block, 0xff, FACTORY_BAD_BLOCK_SIZE);
	MEMSET(nand_info->new_bad_block, 0xff, PHY_PARTITION_BAD_BLOCK_SIZE);
	MEMSET(nand_info->mbr_data, 0xff, sizeof(PARTITION_MBR));

	block_temp = nand_info->boot->uboot_next_block;

	int nand_secure_storage_first_build(unsigned int start_block);
	block_temp = nand_secure_storage_first_build(block_temp);

	block_temp = block_temp + nand_info->boot->physic_block_reserved;

	real_start_block = block_temp;
	if (nand_get_twoplane_flag() == 1) {
		real_start_block = real_start_block / 2;
		if (block_temp % 2)
			real_start_block++;
	}

	ret = nand_info_init_v3(nand_info, chip, real_start_block, mbr_data);
	if (ret == 0) {
		NFTL_DBG("[ND]nand_info_init_v3 ok!\n");
		return 0;
	} else {
		NFTL_DBG("[ND]nand_info_init_v3 fail!\n");
		return -1;
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
static int read_mbr_v2(struct _nand_info *nand_info, unsigned int nDieNum, unsigned int nBlkNum)
{
	int ret = 1, i;
	unsigned int nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	SectBitmap = nand_info->FullBitmap;

	NFTL_DBG("[NE]mbr read %d\n", nBlkNum);
	for (i = 0; i < 10; i++) {
		nPage = i;
		ret = PHY_VirtualPageRead(nDieNum, nBlkNum, nPage, SectBitmap, nand_info->temp_page_buf, spare);
		if ((spare[1] == 0xaa) && (spare[2] == 0xaa) && (ret >= 0)) {
			NFTL_DBG("[NE]mbr read ok!\n");
			MEMCPY(nand_info->mbr_data, nand_info->temp_page_buf, sizeof(PARTITION_MBR));
			ret = 0;
			break;
		} else if ((spare[1] == 0xaa) && (spare[2] == 0xdd)) {
			ret = -1;
			break;
		} else if (ret > 0) {
			ret = 1;
			break;
		} else {
			ret = 1;
		}
	}
	NFTL_DBG("[NE]mbr read end!\n");
	return ret;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int check_mbr_v3(struct _nand_info *nand_info, PARTITION_MBR *mbr)
{
	int i, m, j, k, ret;
	unsigned  int com_len[ND_MAX_PARTITION_COUNT];

	MEMSET(com_len, 0xff, ND_MAX_PARTITION_COUNT << 2);

	//NFTL_DBG("[NE]=================================mbr1!\n");
	for (i = 0, j = 0; i < ND_MAX_PARTITION_COUNT; i++) {
		if ((mbr->array[i].len == 0xffffffff) || (mbr->array[i].len == 0)) {
			break;
		}
		//NFTL_DBG("len: 0x%x!\n",mbr->array[i].len);
		com_len[j] = mbr->array[i].len;
		j++;
	}

	//NFTL_DBG("[NE]=================================mbr2!\n");
	for (i = 0, k = 0, ret = 0; i < nand_info->partition_nums; i++) {
		for (m = 0; m < MAX_PART_COUNT_PER_FTL; m++) {
			if ((nand_info->partition[i].nand_disk[m].size == 0xffffffff) || (nand_info->partition[i].nand_disk[m].size == 0)) {
				break;
			}
			//NFTL_DBG("[NE]len: 0x%x!\n",nand_info->partition[i].nand_disk[m].size);
			if ((com_len[k] != nand_info->partition[i].nand_disk[m].size) && (com_len[k] != 0xffffffff)) {
				NFTL_DBG("[NE]len1: 0x%x len2: 0x%x!\n", com_len[j], nand_info->partition[i].nand_disk[m].size);
				return 1;
			}
			k++;
		}
	}

	if ((j + 1) != k) {
		NFTL_DBG("j 0x%x k 0x%x!\n", j, k);
		return 1;
	}

	//NFTL_DBG("[NE]=================================!\n");

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int read_partition_v2(struct _nand_info *nand_info, unsigned int nDieNum, unsigned int nBlkNum)
{
	int ret = 1, i, j;
	unsigned int nPage;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	SectBitmap = nand_info->FullBitmap;

	NFTL_DBG("[NE]mbr partition start!\n");
	for (i = 10; i < 20; i++) {
		nPage = i;
		ret = PHY_VirtualPageRead(nDieNum, nBlkNum, nPage, SectBitmap, nand_info->temp_page_buf, spare);
		if ((spare[1] == 0xaa) && (spare[2] == 0xee) && (ret >= 0)) {
			NFTL_DBG("[NE]mbr partition ok!\n");
			MEMCPY(nand_info->partition, nand_info->temp_page_buf, sizeof(nand_info->partition));

			nand_info->partition_nums = 0;

			for (j = 0; j < MAX_PARTITION; j++) {
				if (nand_info->partition[j].size != 0) {
					nand_info->partition_nums++;
				}
			}
			ret = 0;
			break;
		} else if ((spare[1] == 0xaa) && (spare[2] == 0xdd)) {
			ret = -1;
			break;
		} else {
			ret = 1;
		}
	}
	NFTL_DBG("[NE]mbr partition end!\n");
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void print_factory_block_table_v2(struct _nand_info *nand_info)
{
	int i, nDieNum;

	nDieNum = FACTORY_BAD_BLOCK_SIZE >> 2;
	for (i = 0; i < nDieNum; i++) {
		if (nand_info->factory_bad_block[i].Chip_NO != 0xffff) {
			NFTL_DBG("[ND]factory bad block:%d %d!\n", nand_info->factory_bad_block[i].Chip_NO, nand_info->factory_bad_block[i].Block_NO);
		} else {
			break;
		}
	}

}

int write_new_block_table_v2_new(struct _nand_info *nand_info)
{
	int ret;
	unsigned int nDieNum, nBlkNum, i;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];
	uint64 SectBitmap;
	unsigned char *buf;

	MEMSET(spare, 0xff, BYTES_OF_USER_PER_PAGE);
	spare[1] = 0xaa;
	spare[2] = 0xcc;
	spare[3] = 0x01;
	nDieNum = nand_info->new_bad_block_addr.Chip_NO;
	nBlkNum = nand_info->new_bad_block_addr.Block_NO;
	SectBitmap = nand_info->FullBitmap;
	buf = nand_info->temp_page_buf;

	NFTL_DBG("[NE]write_new_bad_block_table new format %d!\n", nBlkNum);

	ret = PHY_VirtualBlockErase(nDieNum, nBlkNum);
	if (ret != 0) {
		NFTL_ERR("[NE]new_bad_block_addr erase error?!\n");
	}

	MEMCPY(buf, nand_info->new_bad_block, PHY_PARTITION_BAD_BLOCK_SIZE);

	for (i = 0; i < nand_info->PageNumsPerBlk; i++) {
		ret = PHY_VirtualPageWrite(nDieNum, nBlkNum, i, SectBitmap, buf, spare);
		if (ret != 0) {
			NFTL_ERR("[NE]bad_block_addr write error,page:%d!\n", i);
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
static void print_nand_info_v2(struct _nand_info *nand_info)
{
	int i;
	NFTL_DBG("[ND]nand_info->type :%d\n", nand_info->type);
	NFTL_DBG("[ND]nand_info->SectorNumsPerPage :%d\n", nand_info->SectorNumsPerPage);
	NFTL_DBG("[ND]nand_info->BytesUserData :%d\n", nand_info->BytesUserData);
	NFTL_DBG("[ND]nand_info->PageNumsPerBlk :%d\n", nand_info->PageNumsPerBlk);
	NFTL_DBG("[ND]nand_info->BlkPerChip :%d\n", nand_info->BlkPerChip);
	NFTL_DBG("[ND]nand_info->FirstBuild :%d\n", nand_info->FirstBuild);
	NFTL_DBG("[ND]nand_info->FullBitmap :%d\n", nand_info->FullBitmap);
	NFTL_DBG("[ND]nand_info->bad_block_addr.Chip_NO :%d\n", nand_info->bad_block_addr.Chip_NO);
	NFTL_DBG("[ND]nand_info->bad_block_addr.Block_NO :%d\n", nand_info->bad_block_addr.Block_NO);
	NFTL_DBG("[ND]nand_info->mbr_block_addr.Chip_NO :%d\n", nand_info->mbr_block_addr.Chip_NO);
	NFTL_DBG("[ND]nand_info->mbr_block_addr.Block_NO :%d\n", nand_info->mbr_block_addr.Block_NO);
	NFTL_DBG("[ND]nand_info->no_used_block_addr.Chip_NO :%d\n", nand_info->no_used_block_addr.Chip_NO);
	NFTL_DBG("[ND]nand_info->no_used_block_addr.Block_NO :%d\n", nand_info->no_used_block_addr.Block_NO);
	NFTL_DBG("[ND]nand_info->new_bad_block_addr.Chip_NO :%d\n", nand_info->new_bad_block_addr.Chip_NO);
	NFTL_DBG("[ND]nand_info->new_bad_block_addr.Block_NO :%d\n", nand_info->new_bad_block_addr.Block_NO);
	NFTL_DBG("[ND]nand_info->new_bad_page_addr :%d\n", nand_info->new_bad_page_addr);
	NFTL_DBG("[ND]nand_info->partition_nums :%d\n", nand_info->partition_nums);

	NFTL_DBG("[ND]sizeof partition:%d\n", sizeof(nand_info->partition));

	for (i = 0; i < nand_info->partition_nums; i++) {
		NFTL_DBG("[ND]nand_info->partition:%d:\n", i);
		NFTL_DBG("[ND]size:0x%x\n", nand_info->partition[i].size);
		NFTL_DBG("[ND]cross_talk:0x%x\n", nand_info->partition[i].cross_talk);
		NFTL_DBG("[ND]attribute:0x%x\n", nand_info->partition[i].attribute);
		NFTL_DBG("[ND]start: chip:%d block:%d\n", nand_info->partition[i].start.Chip_NO, nand_info->partition[i].start.Block_NO);
		NFTL_DBG("[ND]end  : chip:%d block:%d\n", nand_info->partition[i].end.Chip_NO, nand_info->partition[i].end.Block_NO);
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int get_partition_v3(struct _nand_info *nand_info)
{
	int last_num = 0, nums = 0, new_partition;
	PARTITION_MBR *mbr;
	unsigned int part_cnt, i, m;

	mbr = (PARTITION_MBR *)nand_info->mbr_data;

	MEMSET(nand_info->partition, 0xff, sizeof(nand_info->partition));

	NFTL_DBG("[ND]nand_info->SectorNumsPerPage :0x%x\n", nand_info->SectorNumsPerPage);
	NFTL_DBG("[ND]nand_info->PageNumsPerBlk :0x%x\n", nand_info->PageNumsPerBlk);
	NFTL_DBG("[ND]nand_info->BlkPerChip :0x%x\n", nand_info->BlkPerChip);
	NFTL_DBG("[ND]nand_info->ChipNum :0x%x\n", nand_info->ChipNum);


	nand_info->partition[0].cross_talk = 0;
	nand_info->partition[0].attribute = 0;
	nand_info->partition_nums = 1;

	if ((mbr->array[0].user_type & 0xf000) != 0xf000) {
		new_partition = 0;
		nand_info->partition[0].size = 0xffffffff;
	} else {
		new_partition = 1;
	}

	//print_mbr_data(mbr);

	for (part_cnt = 0, m = 0; part_cnt < mbr->PartCount && part_cnt < ND_MAX_PARTITION_COUNT; part_cnt++) {
		if (new_partition == 0) {
			nand_info->partition[0].nand_disk[m].size = mbr->array[part_cnt].len;
			nand_info->partition[0].nand_disk[m].type = 0;
			MEMCPY(nand_info->partition[0].nand_disk[m].name, mbr->array[part_cnt].classname, PARTITION_NAME_SIZE);
			m++;
		} else {
			nums = mbr->array[part_cnt].user_type & 0x0f00;
			nums >>= 8;
			if ((last_num != nums) && ((last_num + 1) != nums)) {
				return -1;
			}
			if (last_num != nums) {
				m = 0;
				nand_info->partition[nums].size = 0;
				nand_info->partition_nums = nums + 1;
			}
			last_num = nums;
			nand_info->partition[nums].nand_disk[m].size = mbr->array[part_cnt].len;
			nand_info->partition[nums].nand_disk[m].type = 0;
			MEMCPY(nand_info->partition[nums].nand_disk[m].name, mbr->array[part_cnt].classname, PARTITION_NAME_SIZE);
			nand_info->partition[nums].size += nand_info->partition[nums].nand_disk[m].size;
			nand_info->partition[nums].cross_talk = 0;
			nand_info->partition[nums].attribute = 0;
			if (mbr->array[part_cnt].len == 0) {
				nand_info->partition[nums].size = 0xffffffff;
			}
			m++;
		}
	}

	for (i = 0; i < nand_info->partition_nums; i++) {
		NFTL_DBG("[NE]print partition %d !\n", i);
		print_partition(&nand_info->partition[i]);
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
static void new_print_all_partition(struct _nand_info *nand_info)
{
	int i, m;
	for (i = 0; i < nand_info->partition_nums; i++) {
		NFTL_DBG("[NE]partition_num: %x,size :0x%x,cross_talk %x\n", i, nand_info->partition[i].size, nand_info->partition[i].cross_talk);

		for (m = 0; m < MAX_PART_COUNT_PER_FTL; m++) {
			if (nand_info->partition[i].nand_disk[m].type == 0xffffffff) {
				break;
			}
			NFTL_DBG("[NE]part %s size: 0x%x type: %x\n", nand_info->partition[i].nand_disk[m].name, nand_info->partition[i].nand_disk[m].size, nand_info->partition[i].nand_disk[m].type);
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
static int build_all_phy_partition_v2(struct _nand_info *nand_info)
{
	int i;
	new_print_all_partition(nand_info);

	nand_info->phy_partition_head = NULL;

	for (i = 0; i < nand_info->partition_nums; i++) {
		nand_info->partition[i].cross_talk = 0;
		if (build_phy_partition_v2(nand_info, &nand_info->partition[i]) == NULL) {
			NFTL_ERR("[NE]build phy partition %d error!\n", i);
			return -1;
		}
	}

	NFTL_DBG("[ND]build %d phy_partition !\n", nand_info->partition_nums);
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void print_mbr_data(uchar *mbr_data)
{
	int i;
	PARTITION_MBR *mbr = (PARTITION_MBR *)mbr_data;
	NAND_PARTITION *part __attribute__((unused));
	NFTL_DBG("[NE]mbr->PartCount: %d!\n", mbr->PartCount);

	for (i = 0; i < ND_MAX_PARTITION_COUNT; i++) {
		part = (NAND_PARTITION *)(&mbr->array[i]);

		NFTL_DBG("part: %d!\n", i);
		NFTL_DBG("[NE]part->name: %s !\n", part->classname);
		NFTL_DBG("[NE]part->addr: 0x%x !\n", part->addr);
		NFTL_DBG("[NE]part->len: 0x%x !\n", part->len);
		NFTL_DBG("[NE]part->user_type: 0x%x !\n", part->user_type);
		NFTL_DBG("[NE]part->keydata: %d !\n", part->keydata);
		NFTL_DBG("[NE]part->ro: %d !\n", part->ro);
	}
}

__u32 nand_get_nand_version(void)
{
       __u32 nand_version;

       nand_version = 0;
       nand_version |= 0xff;
       nand_version |= 0x00 << 8;
       nand_version |= NAND_VERSION_0 << 16;
       nand_version |= NAND_VERSION_1 << 24;

       return nand_version;
}

int nand_get_version(__u8 *nand_version)
{
       __u32 version;
       version = nand_get_nand_version();

       nand_version[0] = (u8)version;
       nand_version[1] = (u8)(version >> 8);
       nand_version[2] = (u8)(version >> 16);
       nand_version[3] = (u8)(version >> 24);

       return version;
}

