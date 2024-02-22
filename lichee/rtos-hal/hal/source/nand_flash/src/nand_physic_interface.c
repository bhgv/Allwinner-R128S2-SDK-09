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

#define _NPHYI_COMMON_C_

#include "nand_osal.h"
#include "nand_physic_interface_spinand.h"
#include "nand_format.h"
#include "nand_scan.h"

//#define POWER_OFF_DBG

#ifndef POWER_OFF_DBG
unsigned int power_off_dbg_enable = 0;
#else
unsigned int power_off_dbg_enable = 1;
#endif
/* current logical write block */
unsigned short cur_w_lb_no = 0;
/* current physical write block */
unsigned short cur_w_pb_no = 0;
/* current physical write page */
unsigned short cur_w_p_no = 0;
/* current logical erase block */
unsigned short cur_e_lb_no = 0;

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
struct _nand_info *NandHwInit(void)
{
	NAND_PhysicLockInit();

	return SpiNandHwInit();
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int NandHwExit(void)
{
	return SpiNandHwExit();
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_physic_erase_block(unsigned int chip, unsigned int block)
{
	return spinand_physic_erase_block(chip, block);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_physic_read_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf)
{
	return spinand_physic_read_page(chip, block, page, bitmap, mbuf, sbuf);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_physic_write_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf)
{
	return spinand_physic_write_page(chip, block, page, bitmap, mbuf, sbuf);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_physic_bad_block_check(unsigned int chip, unsigned int block)
{
	return spinand_physic_bad_block_check(chip, block);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_physic_bad_block_mark(unsigned int chip, unsigned int block)
{
	return spinand_physic_bad_block_mark(chip, block);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_physic_block_copy(unsigned int chip_s, unsigned int block_s, unsigned int chip_d, unsigned int block_d)
{
	return spinand_physic_block_copy(chip_s, block_s, chip_d, block_d);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int PHY_VirtualPageRead(unsigned int nDieNum, unsigned int nBlkNum, unsigned int nPage, uint64 SectBitmap, void *pBuf, void *pSpare)
{
	return spinand_super_page_read(nDieNum, nBlkNum, nPage, SectBitmap, pBuf, pSpare);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int PHY_VirtualPageWrite(unsigned int nDieNum, unsigned int nBlkNum, unsigned int nPage, uint64 SectBitmap, void *pBuf, void *pSpare)
{
	return spinand_super_page_write(nDieNum, nBlkNum, nPage, SectBitmap, pBuf, pSpare);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int PHY_VirtualBlockErase(unsigned int nDieNum, unsigned int nBlkNum)
{
	return spinand_super_block_erase(nDieNum, nBlkNum);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int PHY_VirtualBadBlockCheck(unsigned int nDieNum, unsigned int nBlkNum)
{
	return spinand_super_badblock_check(nDieNum, nBlkNum);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int PHY_VirtualBadBlockMark(unsigned int nDieNum, unsigned int nBlkNum)
{
	return spinand_super_badblock_mark(nDieNum, nBlkNum);
}

__u32 NAND_GetLsbblksize(void)
{
	return SPINAND_GetLsbblksize();
}

__u32 NAND_GetLsbPages(void)
{
	return SPINAND_GetLsbPages();
}

__u32 NAND_GetPhyblksize(void)
{
	return SPINAND_GetPhyblksize();
}

__u32 NAND_UsedLsbPages(void)
{
	return SPINAND_UsedLsbPages();
}

__u32 NAND_GetPageNo(__u32 lsb_page_no)
{
	return SPINAND_GetPageNo(lsb_page_no);
}

__u32 NAND_GetPageCntPerBlk(void)
{
	return SPINAND_GetPageCntPerBlk();
}

__u32 NAND_GetPageSize(void)
{
	return SPINAND_GetPageSize();
}

__u32 nand_get_twoplane_flag(void)
{
	return spinand_get_twoplane_flag();
}

