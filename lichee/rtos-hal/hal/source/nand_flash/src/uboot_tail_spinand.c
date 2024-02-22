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

#define _UBOOTT_SPINAND_C_

#include "nand_cfg.h"
#include "nand_osal.h"
#include "nand_struct.h"
#include "nand_type_spinand.h"
#include "nand_physic_interface_spinand.h"
#include "nand_common_info.h"
#include "nand_format.h"
#include "uboot_head.h"
#include "nand_info_init.h"

#include "nand_physic_interface.h"
#include "nand_phy.h"
#include "nand_scan.h"

#define VALID_PAGESIZE_FOR_BOOT0		2048

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int spinand_write_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	__u32 i, j;
	__u8  oob_buf[64];
	__u32 pages_per_block, blocks_per_copy, start_block, count, pagesize;
	struct boot_physical_param  para;

	PHY_DBG("SPINAND burn boot0!\n");

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		PHY_ERR("get flash driver version error!");
		goto error;
	}

	pagesize = NAND_GetPageSize();
	pages_per_block = NAND_GetPageCntPerBlk();
	blocks_per_copy = 1;
	start_block = blocks_per_copy * counter;
	if ((start_block + blocks_per_copy) > aw_nand_info.boot->uboot_start_block) {
		return 0;
	}

	PHY_DBG("boot0 count %d!\n", counter);

	count = 0;
	for (i = start_block; i < (start_block + blocks_per_copy); i++) {
		para.chip  = 0;
		para.block = i;
		if (PHY_SimpleErase(&para) < 0) {
			PHY_ERR("Fail in erasing block %d.\n", i);
			//continue;
		}

		for (j = 0; j < pages_per_block; j++) {
			para.chip  = 0;
			para.block = i;
			para.page = j;
			if (count < len / VALID_PAGESIZE_FOR_BOOT0)
				para.mainbuf = (__u8 *)(buf + VALID_PAGESIZE_FOR_BOOT0 * ((i - start_block) * pages_per_block + j));
			else
				para.mainbuf = (__u8 *)buf;
			para.oobbuf = oob_buf;
			if (pagesize == 2048)
				para.sectorbitmap = 0xf;
			else if (pagesize == 4096)
				para.sectorbitmap = 0xff;
			if (PHY_SimpleWrite(&para) < 0) {
				PHY_ERR("Warning. Fail in writing page %d in block %d.\n", j, i);
			}

			count++;
		}
	}

	return 0;

error:

	return -1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int spinand_read_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	__u32 i, j, m;
	__u8  oob_buf[64];
	__u32 pages_per_block, blocks_per_copy, start_block, count;
	__u32 flag;
	unsigned char *ptr;
	struct boot_physical_param  para;

	PHY_DBG("spinand_read_boot0_one\n");

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	pages_per_block = NAND_GetPageCntPerBlk();

	blocks_per_copy = 1;
	start_block = blocks_per_copy * counter;
	if ((start_block + blocks_per_copy) > aw_nand_info.boot->uboot_start_block) {
		return 0;
	}

	PHY_DBG("boot0 count %d!\n", counter);

	ptr = (__u8 *)MALLOC(512 * SECTOR_CNT_OF_SINGLE_PAGE);

	count = 0;
	flag = 0;
	for (i = start_block; i < (start_block + blocks_per_copy); i++) {
		for (j = 0; j < pages_per_block; j++) {
			para.chip  = 0;
			para.block = i;
			para.page = j;
			para.mainbuf = (void *)ptr;
			para.oobbuf = oob_buf;
			para.sectorbitmap = 0xf;
			for (m = 0; m < 32; m++)
				oob_buf[m] = 0x55;
			if (PHY_SimpleRead(&para) < 0) {
				PHY_ERR("Warning. Fail in read page %d in block %d.\n", j, i);
				goto error;
			}

			if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
				PHY_ERR("get flash driver version error!");
				goto error;
			}

			MEMCPY(buf + count * VALID_PAGESIZE_FOR_BOOT0, ptr, VALID_PAGESIZE_FOR_BOOT0);

			count++;
			if (count == (len / VALID_PAGESIZE_FOR_BOOT0)) {
				flag = 1;
				break;
			}
		}
		if (flag == 1)
			break;
	}
	FREE(ptr, 512 * SECTOR_CNT_OF_SINGLE_PAGE);
	return 0;

error:
	FREE(ptr, 512 * SECTOR_CNT_OF_SINGLE_PAGE);
	return -1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int spinand_write_uboot_one_in_block(unsigned char *buf, unsigned int len, struct _boot_info *info_buf, unsigned int info_len, unsigned int counter)
{
	unsigned int j, boot_len, page_index, t_len;
	unsigned char  oob_buf[64];
	int uboot_flag, info_flag;
	unsigned char *kernel_buf;
	unsigned char *ptr;
	struct boot_physical_param  para;
	int pagesize, pages_per_block;

	PHY_DBG("spinand burn uboot in one block!\n");

	pagesize = NAND_GetPageSize();
	pages_per_block = NAND_GetPageCntPerBlk();

	uboot_flag = 0;
	info_flag = 0;
	boot_len = 0;
	t_len = 0;

	kernel_buf = MALLOC(pagesize);

	MEMSET(oob_buf, 0xff, 64);

	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		PHY_ERR("get flash driver version error!");
		FREE(kernel_buf, pagesize);
		return ERR_NO_105;
	}

	para.chip  = 0;
	para.block = info_buf->uboot_start_block + counter;
	if (PHY_SimpleErase(&para) < 0) {
		PHY_ERR("Fail in erasing block %d.\n", para.block);
		//continue;
	}

	page_index = 0;
	for (j = 0; j < pages_per_block; j++) {
		para.chip  = 0;
		para.block = info_buf->uboot_start_block + counter;

		para.page = j;
		if (pagesize == 2048)
			para.sectorbitmap = 0xf;
		else if (pagesize == 4096)
			para.sectorbitmap = 0xff;
		para.oobbuf = oob_buf;

		if (uboot_flag == 0) {
			boot_len = page_index * pagesize;
			MEMCPY(kernel_buf, buf + boot_len, pagesize);
			ptr = kernel_buf;
			if ((len - boot_len) == (pagesize)) {
				uboot_flag = page_index + 1;
			}
		} else if (info_flag == 0) {
			PHY_DBG("uboot info: page %d in block %d.\n", para.page, para.block);
			t_len = (page_index - uboot_flag) * pagesize;
			ptr = (unsigned char *)info_buf;
			ptr += t_len;
			if ((info_len - t_len) == pagesize) {
				info_flag = page_index;
			}
		} else {
			ptr = kernel_buf;
		}

		para.mainbuf = ptr;

		if (PHY_SimpleWrite(&para) < 0) {
			PHY_ERR("Warning. Fail in writing page %d in block %d.\n", j, para.block);
		}
		page_index++;
	}

	FREE(kernel_buf, pagesize);
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int spinand_write_uboot_one_in_many_block(unsigned char *buf, unsigned int len, struct _boot_info *info_buf, unsigned int info_len, unsigned int counter)
{
	int ret;
	unsigned int j, boot_len, page_index, total_len, t_len, total_pages, pages_per_block, good_block_offset, blocks_one_uboot, uboot_block_offset, m;
	unsigned int write_blocks;
	unsigned char  oob_buf[64];
	int uboot_flag, info_flag;
	unsigned char *kernel_buf;
	unsigned char *ptr;
	struct boot_physical_param  para;
	int pagesize;

	pagesize = NAND_GetPageSize();
	pages_per_block = NAND_GetPageCntPerBlk();

	PHY_DBG("spinand burn uboot in many block %d!\n", counter);

	uboot_flag = 0;
	info_flag = 0;
	boot_len = 0;
	t_len = 0;

	kernel_buf = MALLOC(pagesize);

	MEMSET(oob_buf, 0xff, 64);

	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		PHY_ERR("get flash driver version error!");
		FREE(kernel_buf, pagesize);
		return ERR_NO_105;
	}

	if (len % pagesize) {
		PHY_ERR("uboot length check error!\n");
		FREE(kernel_buf, pagesize);
		return ERR_NO_104;
	}

	total_len = len + info_len;
	if (total_len % pagesize) {
		PHY_ERR("uboot length check error!\n");
		FREE(kernel_buf, pagesize);
		return ERR_NO_104;
	}
	total_pages = total_len / pagesize;
	blocks_one_uboot = total_pages / pages_per_block;

	if (total_pages % pages_per_block) {
		blocks_one_uboot++;
	}

	good_block_offset = blocks_one_uboot * counter;

	for (m = 0, j = info_buf->uboot_start_block; j < info_buf->uboot_next_block; j++) {
		if (m == good_block_offset) {
			break;
		}
		ret = spinand_physic_bad_block_check(0, j);
		if (ret == 0) {
			m++;
		}
	}

	uboot_block_offset = j;

	if ((uboot_block_offset + blocks_one_uboot) > info_buf->uboot_next_block) {
		FREE(kernel_buf, pagesize);
		return 0;
	}

/////////////////////////////////////////////////////

	uboot_flag = 0;
	info_flag = 0;
	boot_len = 0;
	page_index = 0;
	write_blocks = 0;

	for (j = uboot_block_offset; j < info_buf->uboot_next_block; j++) {
		para.chip  = 0;
		para.block = j;

		ret = spinand_physic_bad_block_check(0, j);
		if (ret != 0) {
			continue;
		}

		if (PHY_SimpleErase(&para) < 0) {
			PHY_ERR("Fail in erasing block %d.\n", para.block);
			//continue;
		}

		write_blocks++;

		PHY_DBG("write uboot many block %d!\n", para.block);

		for (m = 0; m < pages_per_block; m++) {
			para.chip  = 0;
			para.block = j;
			para.page = m;
			if (pagesize == 2048)
				para.sectorbitmap = 0xf;
			else if (pagesize == 4096)
				para.sectorbitmap = 0xff;
			para.oobbuf = oob_buf;

			if (uboot_flag == 0) {
				boot_len = page_index * pagesize;
				MEMCPY(kernel_buf, buf + boot_len, pagesize);
				ptr = kernel_buf;
				if ((len - boot_len) == pagesize) {
					uboot_flag = page_index + 1;
				}
			} else if (info_flag == 0) {
				PHY_DBG("uboot info: page %d in block %d.\n", para.page, para.block);
				t_len = (page_index - uboot_flag) * pagesize;
				ptr = (unsigned char *)info_buf;
				ptr += t_len;
				if ((info_len - t_len) == pagesize) {
					info_flag = page_index;
				}
			} else {
				ptr = kernel_buf;
			}

			para.mainbuf = ptr;

			if (PHY_SimpleWrite(&para) < 0) {
				PHY_ERR("Warning. Fail in writing page %d in block %d.\n", para.page, para.block);
			}
			page_index++;
		}

		if (blocks_one_uboot == write_blocks) {
			break;
		}


	}

	FREE(kernel_buf, pagesize);
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int spinand_write_uboot_one(unsigned char *buf, unsigned int len, struct _boot_info *info_buf, unsigned int info_len, unsigned int counter)
{
	int ret;
	int real_len;

	PHY_DBG("spinand burn uboot one!\n");

    int spinand_add_len_to_uboot_tail(unsigned int  uboot_size);
	real_len = spinand_add_len_to_uboot_tail(len);

	//print_physic_info(phyinfo_buf);

	if (real_len <= SPINAND_GetPhyblksize()) {
		ret = spinand_write_uboot_one_in_block(buf, len, info_buf, info_len, counter);
	} else {
		ret = spinand_write_uboot_one_in_many_block(buf, len, info_buf, info_len, counter);
	}
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail 1: ecc limit
*Note         :
*****************************************************************************/
int spinand_read_uboot_one_in_block(unsigned char *buf, unsigned int len, unsigned int counter)
{
	int ret = 0, data_error = 0, ecc_limit = 0;
	unsigned int j, page_index, total_pages;
	unsigned char  oob_buf[64];
	unsigned char *ptr;
	struct boot_physical_param  para;
	int pagesize, pages_per_block;

	pagesize = NAND_GetPageSize();
	pages_per_block = NAND_GetPageCntPerBlk();

	PHY_DBG("spinand read uboot in one block %d!\n", counter);

	ptr = MALLOC(pagesize);

	MEMSET(oob_buf, 0xff, 64);

	if (len % pagesize) {
		PHY_ERR("uboot length check error!\n");
		FREE(ptr, pagesize);
		return 0;
	}

	total_pages = len / pagesize;

	for (j = 0, page_index = 0; j < pages_per_block; j++) {
		para.chip  = 0;
		para.block = phyinfo_buf->uboot_start_block + counter;

		para.page = j;
		if (pagesize == 2048)
			para.sectorbitmap = 0xf;
		else if (pagesize == 4096)
			para.sectorbitmap = 0xff;
		para.oobbuf = oob_buf;
		para.mainbuf = ptr;

		ret = PHY_SimpleRead(&para);
		if (ret == 0) {
			;
		} else if (ret == ECC_LIMIT) {
			ecc_limit = 1;
			PHY_ERR("Warning. Fail in read page %d in block %d \n", para.page, para.block);
		} else {
			data_error = 1;
			break;
		}

		if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
			PHY_ERR("get uboot flash driver version error!\n");
			data_error = 1;
			break;
		}

		MEMCPY(buf + page_index * pagesize, ptr, pagesize);

		page_index++;

		if (total_pages == page_index) {
			break;
		}
	}

	FREE(ptr, pagesize);

	if (data_error == 1) {
		ret = -1;
	} else if (ecc_limit == 1) {
		ret = 1;
	} else {
		ret = 0;
	}
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail  1: ecc limit
*Note         :
*****************************************************************************/
int spinand_read_uboot_one_in_many_block(unsigned char *buf, unsigned int len, unsigned int counter)
{
	int ret = 0, data_error = 0, ecc_limit = 0;
	unsigned int j, page_index, total_len, total_pages, pages_per_block, good_block_offset, blocks_one_uboot, uboot_block_offset, m;
	unsigned char  oob_buf[64];
	
	unsigned char *kernel_buf;

	struct boot_physical_param  para;
	int pagesize;

	pagesize = NAND_GetPageSize();
	pages_per_block = NAND_GetPageCntPerBlk();

	PHY_DBG("spinand read uboot in many block %d!\n", counter);

	

	kernel_buf = MALLOC(pagesize);

	MEMSET(oob_buf, 0xff, 64);

	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		PHY_ERR("get flash driver version error!");
		FREE(kernel_buf, pagesize);
		return ERR_NO_105;
	}

	if (len % pagesize) {
		PHY_ERR("uboot length check error!\n");
		FREE(kernel_buf, pagesize);
		return ERR_NO_104;
	}

	total_len = len;
	total_pages = total_len / pagesize;
	blocks_one_uboot = total_pages / pages_per_block;

	if (total_pages % pages_per_block) {
		blocks_one_uboot++;
	}

	good_block_offset = blocks_one_uboot * counter;

	for (m = 0, j = phyinfo_buf->uboot_start_block; j < phyinfo_buf->uboot_next_block; j++) {
		if (m == good_block_offset) {
			break;
		}
		ret = spinand_physic_bad_block_check(0, j);
		if (ret == 0) {
			m++;
		}
	}

	uboot_block_offset = j;
	if ((uboot_block_offset + blocks_one_uboot) > phyinfo_buf->uboot_next_block) {
		FREE(kernel_buf, pagesize);
		return 0;
	}

/////////////////////////////////////////////////////

	page_index = 0;

	for (j = uboot_block_offset; j < phyinfo_buf->uboot_next_block; j++) {
		ret = spinand_physic_bad_block_check(0, j);
		if (ret != 0) {
			continue;
		}

		PHY_DBG("read uboot many block %d!\n", para.block);

		for (m = 0; m < pages_per_block; m++) {
			para.chip  = 0;
			para.block = j;
			para.page = m;
			if (pagesize == 2048)
				para.sectorbitmap = 0xf;
			else if (pagesize == 4096)
				para.sectorbitmap = 0xff;
			para.oobbuf = oob_buf;
			para.mainbuf = kernel_buf;

			ret = PHY_SimpleRead(&para);
			if (ret == 0) {
				;
			} else if (ret == ECC_LIMIT) {
				ecc_limit = 1;
				PHY_ERR("Warning. Fail in read page %d in block %d \n", para.page, para.block);
			} else {
				PHY_ERR("error read page: %d in block %d \n", para.page, para.block);
				data_error = 1;
				break;
			}

			if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
				PHY_ERR("get uboot flash driver version error!\n");
				data_error = 1;
				break;
			}

			MEMCPY(buf + page_index * pagesize, para.mainbuf, pagesize);
			page_index++;
			if (total_pages == page_index) {
				break;
			}
		}

		if (total_pages == page_index) {
			break;
		}
		if (data_error == 1) {
			break;
		}

	}

	FREE(kernel_buf, pagesize);

	if (data_error == 1) {
		ret = -1;
	} else if (ecc_limit == 1) {
		ret = 1;
	} else {
		ret = 0;
	}
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  <0:fail 1: ecc limit
*Note         :
*****************************************************************************/
int spinand_read_uboot_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	int ret = 0;

	PHY_DBG("spinand read uboot one %d!\n", counter);

	if (len <= SPINAND_GetPhyblksize()) {
		ret = spinand_read_uboot_one_in_block(buf, len, counter);
	} else {
		ret = spinand_read_uboot_one_in_many_block(buf, len, counter);
	}
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :NAND_GetParam
*****************************************************************************/
int spinand_get_param(void *nand_param)
{
	__u32 i;
	boot_spinand_para_t *nand_param_temp;

	nand_param_temp = (boot_spinand_para_t *)nand_param;

	nand_param_temp->ChipCnt               = NandStorageInfo.ChipCnt;
	nand_param_temp->ChipConnectInfo       = NandStorageInfo.ChipConnectInfo;
	nand_param_temp->ConnectMode           = NandStorageInfo.ConnectMode;
	nand_param_temp->BankCntPerChip        = NandStorageInfo.BankCntPerChip;
	nand_param_temp->DieCntPerChip         = NandStorageInfo.DieCntPerChip;
	nand_param_temp->PlaneCntPerDie        = NandStorageInfo.PlaneCntPerDie;
	nand_param_temp->SectorCntPerPage      = NandStorageInfo.SectorCntPerPage;
	nand_param_temp->PageCntPerPhyBlk      = NandStorageInfo.PageCntPerPhyBlk;
	nand_param_temp->BlkCntPerDie          = NandStorageInfo.BlkCntPerDie;
	nand_param_temp->OperationOpt          = NandStorageInfo.OperationOpt;
	nand_param_temp->FrequencePar          = NandStorageInfo.FrequencePar;
	nand_param_temp->SpiMode               = NandStorageInfo.SpiMode;
	nand_param_temp->MaxEraseTimes         = NandStorageInfo.MaxEraseTimes;
	nand_param_temp->MultiPlaneBlockOffset = NandStorageInfo.MultiPlaneBlockOffset;
	nand_param_temp->pagewithbadflag       = NandStorageInfo.pagewithbadflag;
	nand_param_temp->EccLimitBits          = NandStorageInfo.EccLimitBits;
	nand_param_temp->MaxEccBits            = NandStorageInfo.MaxEccBits;
//	nand_param->spi_nand_function          = NandStorageInfo.spi_nand_function;

//	spic_set_trans_mode(0, NandStorageInfo.SpiMode);

	for (i = 0; i < 8; i++)
		nand_param_temp->NandChipId[i]	=	NandStorageInfo.NandChipId[i];

	return 0;

}

int spinand_get_param_for_uboottail(void *nand_param)
{
	boot_spinand_para_t *nand_param_temp;

	nand_param_temp = (boot_spinand_para_t *)nand_param;

	nand_param_temp->uboot_start_block       = aw_nand_info.boot->uboot_start_block;
	nand_param_temp->uboot_next_block        = aw_nand_info.boot->uboot_next_block;
	nand_param_temp->logic_start_block       = aw_nand_info.boot->logic_start_block;
	nand_param_temp->nand_specialinfo_page   = aw_nand_info.boot->nand_specialinfo_page;
	nand_param_temp->nand_specialinfo_offset = aw_nand_info.boot->nand_specialinfo_offset;
	nand_param_temp->physic_block_reserved   = aw_nand_info.boot->physic_block_reserved;

	PHY_DBG("uboot_start_block:       %x\n", nand_param_temp->uboot_start_block);
	PHY_DBG("uboot_next_block:        %x\n", nand_param_temp->uboot_next_block);
	PHY_DBG("logic_start_block:       %x\n", nand_param_temp->logic_start_block);
	PHY_DBG("nand_specialinfo_page:   %x\n", nand_param_temp->nand_specialinfo_page);
	PHY_DBG("nand_specialinfo_offset: %x\n", nand_param_temp->nand_specialinfo_offset);
	PHY_DBG("physic_block_reserved:   %x\n", nand_param_temp->physic_block_reserved);

	return 0;
}


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
__s32 SPINAND_SetPhyArch_V3(struct _boot_info *ram_arch, void *phy_arch)
{
	struct _spinand_config_para_info *config_para;

	config_para = (struct _spinand_config_para_info *)phy_arch;

	ram_arch->storage_info.config.support_two_plane    = config_para->support_two_plane;
	ram_arch->storage_info.config.support_v_interleave = config_para->support_v_interleave;
	ram_arch->storage_info.config.support_dual_channel = config_para->support_dual_channel;

	ram_arch->storage_info.config.support_dual_read    = config_para->support_dual_read;
	ram_arch->storage_info.config.support_dual_write   = config_para->support_dual_write;
	ram_arch->storage_info.config.support_quad_write   = config_para->support_quad_write;
	ram_arch->storage_info.config.support_quad_read    = config_para->support_quad_read;
	ram_arch->storage_info.config.frequence            = config_para->frequence;

	if (config_para->support_two_plane == 0) {
		ram_arch->storage_info.config.plane_cnt = 1;
	} else {
		ram_arch->storage_info.config.plane_cnt = 2;
	}

	return 0;
}

/* TODO: wrong function  */
__u32 NAND_GetNandIDNumCtrl(void)
{
	return 0;
}

/* TODO: wrong function  */
__u32 NAND_GetNandExtPara(__u32 para_num)
{
	/*unsigned int nand0_offset = 0;*/
	char str[9] __attribute__((unused));

	str[0] = 'n';
	str[1] = 'a';
	str[2] = 'n';
	str[3] = 'd';
	str[4] = '0';
	str[5] = '_';
	str[6] = 'p';
	str[7] = '0';
	str[8] = '\0';

	if (para_num == 0) {
		/* frequency */
		str[7] = '0';
	} else if (para_num == 1) {

		/* SUPPORT_TWO_PLANE */
		str[7] = '1';
	} else if (para_num == 2) {
		/* SUPPORT_VERTICAL_INTERLEAVE */
		str[7] = '2';
	} else if (para_num == 3) {
		/* SUPPORT_DUAL_CHANNEL */
		str[7] = '3';
	} else if (para_num == 4) {
		/*frequency change */
		str[7] = '4';
	} else if (para_num == 5) {
		str[7] = '5';
	} else {
		NAND_Print("NAND GetNandExtPara: wrong para num: %d\n", para_num);
		return 0xffffffff;
	}

	return 0;
}


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int SPINAND_UpdatePhyArch(void)
{
	int ret;
	unsigned int id_number_ctl, para;
	struct _spinand_config_para_info config_para;

	config_para.support_two_plane    = 0;
	config_para.support_v_interleave = 1;
	config_para.support_dual_channel = 1;
	config_para.support_dual_read    = 0;
	config_para.support_dual_write   = 0;
	config_para.support_quad_write   = 1;
	config_para.support_quad_read    = 1;
	config_para.frequence            = 75;

	PHY_DBG("SPINAND UpdatePhyArch\n");

	config_para.support_two_plane    = (SPINAND_MULTI_PROGRAM & NandStorageInfo.OperationOpt) ? 1 : 0;
	config_para.support_v_interleave = (SPINAND_EXT_INTERLEAVE & NandStorageInfo.OperationOpt) ? 1 : 0;
	config_para.support_dual_read    = (SPINAND_DUAL_READ & NandStorageInfo.OperationOpt) ? 1 : 0;
	config_para.support_dual_write   = (SPINAND_DUAL_PROGRAM & NandStorageInfo.OperationOpt) ? 1 : 0;
	config_para.support_quad_write   = (SPINAND_QUAD_PROGRAM & NandStorageInfo.OperationOpt) ? 1 : 0;
	config_para.support_quad_read    = (SPINAND_QUAD_READ & NandStorageInfo.OperationOpt) ? 1 : 0;
	config_para.frequence            = NandStorageInfo.FrequencePar;

	__u32 NAND_GetNandIDNumCtrl(void);
	__u32 NAND_GetNandExtPara(__u32 para_num);

	id_number_ctl = NAND_GetNandIDNumCtrl();
	if ((id_number_ctl & 0x0e) != 0) {
		para = NAND_GetNandExtPara(1);
		if ((para != 0xffffffff) && (id_number_ctl & 0x02)) { //get script success
			if (((para & 0xffffff) == NandStorageInfo.Idnumber) || ((para & 0xffffff) == 0xeeeeee)) {
				PHY_DBG("script support_two_plane %d\n", para);
				config_para.support_two_plane = (para >> 24) & 0xff;
				if (config_para.support_two_plane == 1) {
					config_para.support_two_plane = 1;
				} else {
					config_para.support_two_plane = 0;
				}
			}
		}
	}

	//ret = set_nand_structure((void*)&nand_permanent_data);
	ret = SPINAND_SetPhyArch_V3(aw_nand_info.boot, &config_para);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int spinand_erase_chip(unsigned int chip, unsigned int start_block, unsigned int end_block, unsigned int force_flag)
{
	int ret, i;
	struct boot_physical_param  para;
	int blocks_per_chip;

	blocks_per_chip = SPINAND_GetBlkCntPerChip();

	if ((end_block >= blocks_per_chip) || (end_block == 0))
		end_block = blocks_per_chip;

	if (start_block > end_block)
		return 0;

	for (i = start_block; i < end_block; i++) {
		para.chip = 0;
		para.block = i;
		para.page = 0;

		ret = spinand_physic_bad_block_check(para.chip, i);
		if (force_flag == 1)
			ret = 0;

		if (ret == 0) {
			ret = spinand_physic_erase_block(para.chip, i);
			if (i % 128 == 0)
				PHY_ERR("erase block%d\n", i);
			if (ret != 0) {
				PHY_ERR("erase blk%d failed, mark bad block\n", i);
				spinand_physic_bad_block_mark(para.chip, i);
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
void spinand_erase_special_block(void)
{
	return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int spinand_physic_info_get_one_copy(unsigned int  start_block, unsigned int  pages_offset, unsigned int  *block_per_copy, unsigned int  *buf)
{
	unsigned int  page, block;
	unsigned int  flag;
	unsigned int  size_per_page, pages_per_block, lsbblock_size;
	unsigned char sdata[64];
	int ret;
	unsigned int  phyinfo_page_cnt, page_cnt;
	unsigned int  badblk_num = 0;
	unsigned int  pages_per_phyinfo;
	void *tempbuf = NULL;
	struct boot_physical_param	para;

	PHY_DBG("physic_info_get_one_copy start!!\n");
	size_per_page = NAND_GetPageSize();
	pages_per_block = NAND_GetPageCntPerBlk();

	pages_per_phyinfo = PHY_INFO_SIZE / size_per_page;
	if (PHY_INFO_SIZE % size_per_page)
		pages_per_phyinfo++;

//	PHY_DBG("pages_per_phyinfo %d\n",pages_per_phyinfo);

	tempbuf = (void *)MALLOC(32 * 1024);
	if (tempbuf == NULL) {
		PHY_ERR("tempbuf malloc fail\n");
		return -1;
	}

	page_cnt = 0;
	phyinfo_page_cnt = 0;
	flag = 0;
	for (block = start_block;; block++) {
		para.chip  = 0;
		para.block = block;
		para.page = 0;
		para.oobbuf = sdata;
		para.sectorbitmap = 0;
		para.mainbuf = NULL;

		ret = PHY_SimpleRead(&para);
		if ((sdata[0] == 0x0)) {
			badblk_num++;
			PHY_DBG("bad block:chip %d block %d\n", para.chip, para.block);
			continue;
		}
		for (page = 0; page < pages_per_block; page++) {
			if (page_cnt >= pages_offset) {
				para.chip  = 0;
				para.block = block;
				para.page = page;
				para.oobbuf = sdata;
				para.sectorbitmap = FULL_BITMAP_OF_SINGLE_PAGE;
				para.mainbuf = (void *)((char *)tempbuf + phyinfo_page_cnt * size_per_page);
				PHY_DBG("block %d page %d\n", para.block, para.page);
				ret = PHY_SimpleRead(&para);
				if (ret == ERR_ECC) {
					PHY_ERR("ecc err:chip %d block %d page %d\n", para.chip, para.block, para.page);
					break;
				}

				phyinfo_page_cnt++;
				if (phyinfo_page_cnt == pages_per_phyinfo) {
					flag = 1;
					break;
				}
			}
			page_cnt++;
		}

		if (ret == ERR_ECC)
			break;

		if (flag == 1)
			break;

	}

	MEMCPY(buf, tempbuf, PHY_INFO_SIZE);

	lsbblock_size = NAND_GetLsbblksize();

	*block_per_copy = (pages_offset + pages_per_phyinfo) * size_per_page / lsbblock_size + badblk_num;
	if (((pages_offset + pages_per_phyinfo)*size_per_page) % lsbblock_size)
		*block_per_copy = (*block_per_copy) + 1;
//	PHY_DBG("block_per_copy %d pages_offset+pages_per_phyinfo %d\n",*block_per_copy,(pages_offset+pages_per_phyinfo));

	if (tempbuf) {
		FREE(tempbuf, 32 * 1024);
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
int spinand_add_len_to_uboot_tail(unsigned int  uboot_size)
{
	unsigned int size_per_page, pages_per_uboot, pages_per_block;
	unsigned int page_in_lsb_block __attribute__((unused));
	unsigned int  jump_size;

	size_per_page = NAND_GetPageSize();

	pages_per_uboot = uboot_size / size_per_page;
	if (uboot_size % size_per_page)
		pages_per_uboot++;

	jump_size = size_per_page * pages_per_uboot - uboot_size;

	pages_per_block = NAND_GetPageCntPerBlk();

	page_in_lsb_block = pages_per_uboot % pages_per_block;

	return (uboot_size + jump_size + PHY_INFO_SIZE);
}


