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

/*
************************************************************************************************************************
*                                                      eNand
*                                     Nand flash driver format module define
*
*                             Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
*											       All Rights Reserved
*
* File Name : nand_format.h
*
* Author : Kevin.z
*
* Version : v0.1
*
* Date : 2008.03.25
*
* Description : This file define the function interface and some data structure export
*               for the format module.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Kevin.z         2008.03.25      0.1          build the file
*
************************************************************************************************************************
*/
#ifndef __NAND_FORMAT_H__
#define __NAND_FORMAT_H__

extern struct __LogicArchitecture_t LogicArchiPar;

//define the page buffer for cache the super page data for read or write
#define FORMAT_PAGE_BUF     (PageCachePool.PageCache0)
#define FORMAT_SPARE_BUF    (PageCachePool.SpareCache)

//==============================================================================
//  define the logical architecture export parameter
//==============================================================================

//define the count of the sectors in the logical page
#define SECTOR_CNT_OF_LOGIC_PAGE    SECTOR_CNT_OF_SUPER_PAGE

//define the full bitmap of sector in the logical page
#define FULL_BITMAP_OF_LOGIC_PAGE   FULL_BITMAP_OF_SUPER_PAGE

//define the count of the pages in a logical block
#define PAGE_CNT_OF_LOGIC_BLK       (LogicArchiPar.PageCntPerLogicBlk) //(NandDriverInfo.LogicalArchitecture->PageCntPerLogicBlk)

//define the count of the pages in a super physical block, size is same as the logical block
#define PAGE_CNT_OF_SUPER_BLK       PAGE_CNT_OF_LOGIC_BLK

//define the total count of inter-leave banks
#define INTERLEAVE_BANK_CNT         (PAGE_CNT_OF_LOGIC_BLK / NandStorageInfo.PageCntPerPhyBlk) //(PAGE_CNT_OF_LOGIC_BLK / NandDriverInfo.NandStorageInfo->PageCntPerPhyBlk)

#define LOGIC_DIE_CNT				(LogicArchiPar.LogicDieCnt)//(NandDriverInfo.LogicalArchitecture->LogicDieCnt)

#define BLK_CNT_OF_LOGIC_DIE		(LogicArchiPar.LogicBlkCntPerLogicDie)//(BLOCK_CNT_OF_ZONE*ZONE_CNT_OF_DIE)

#define LOGIC_PAGE_SIZE				(SECTOR_CNT_OF_SUPER_PAGE*512)

/*
************************************************************************************************************************
*                                   FORMAT NAND FLASH DISK MODULE INIT
*
*Description: Init the nand disk format module, initiate some variables and request resource.
*
*Arguments  : none
*
*Return     : init result;
*               = 0     format module init successful;
*               < 0     format module init failed.
************************************************************************************************************************
*/
__s32 FMT_Init(void);


/*
************************************************************************************************************************
*                                   FORMAT NAND FLASH DISK MODULE EXIT
*
*Description: Exit the nand disk format module, release some resource.
*
*Arguments  : none
*
*Return     : exit result;
*               = 0     format module exit successful;
*               < 0     format module exit failed.
************************************************************************************************************************
*/
__s32 FMT_Exit(void);

__s32 spinand_super_page_read(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare);
__s32 spinand_super_page_write(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare);
__s32 spinand_super_block_erase(__u32 nDieNum, __u32 nBlkNum);
__s32 spinand_super_badblock_check(__u32 nDieNum, __u32 nBlkNum);
__s32 spinand_super_badblock_mark(__u32 nDieNum, __u32 nBlkNum);
int spinand_physic_erase_block(unsigned int chip, unsigned int block);
int spinand_physic_read_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf);
int spinand_physic_write_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf);
int spinand_physic_bad_block_check(unsigned int chip, unsigned int block);
int spinand_physic_bad_block_mark(unsigned int chip, unsigned int block);
int spinand_physic_block_copy(unsigned int chip_s, unsigned int block_s, unsigned int chip_d, unsigned int block_d);


#endif  //ifndef __NAND_FORMAT_H__


