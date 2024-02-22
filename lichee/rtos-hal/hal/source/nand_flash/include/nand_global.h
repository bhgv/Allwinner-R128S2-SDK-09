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

#ifndef __NAND_GLOBAL_H
#define __NAND_GLOBAL_H


extern struct _nand_info aw_nand_info;
extern struct _uboot_info uboot_info;
extern struct _boot_info *phyinfo_buf;
extern struct __NandStorageInfo_t NandStorageInfo;
extern struct __NandPageCachePool_t PageCachePool;

//==============================================================================
//  define the physical archictecture export parameter
//==============================================================================

//define the Ecc Mode
#define ECC_MODE           (NandStorageInfo.EccMode)

//define the DDR tyep
//#define DDR_TYPE           (NandStorageInfo.DDRType)

//define the sector count of a single physical page
#define SECTOR_CNT_OF_SINGLE_PAGE           (NandStorageInfo.SectorCntPerPage)

//define the sector count of a super physical page, the super page may be based on multi-plane
#define SECTOR_CNT_OF_SUPER_PAGE            (NandStorageInfo.SectorCntPerPage * NandStorageInfo.PlaneCntPerDie)

//define the sector bitmap for a single page
#define FULL_BITMAP_OF_SINGLE_PAGE           ((__u32)(((__u32)1<<(SECTOR_CNT_OF_SINGLE_PAGE - 1)) | (((__u32)1<<(SECTOR_CNT_OF_SINGLE_PAGE - 1)) - 1)))

//define the sector bitmap for a super page, the sector count of a super page may be equal to 32
#define FULL_BITMAP_OF_SUPER_PAGE           ((__u64)(((__u64)1<<(SECTOR_CNT_OF_SUPER_PAGE - 1)) | (((__u64)1<<(SECTOR_CNT_OF_SUPER_PAGE - 1)) - 1)))

//define the block number offset for the multi-plane operation
#define MULTI_PLANE_BLOCK_OFFSET            (NandStorageInfo.MultiPlaneBlockOffset)

//define the position of the bad block flag in a physical block
#define BAD_BLK_FLAG_PST                    (NandStorageInfo.pagewithbadflag)

//define if the nand flash can support multi-plane read operation
#define SUPPORT_MULTI_READ                  (SPINAND_MULTI_READ & NandStorageInfo.OperationOpt)

//define if the nand flash can support multi-plane program operation
#define SUPPORT_MULTI_PROGRAM               (SPINAND_MULTI_PROGRAM & NandStorageInfo.OperationOpt)

//define if the nand flash can support page copy-back with command operation
//#define SUPPORT_PAGE_COPYBACK               (NAND_PAGE_COPYBACK & NandStorageInfo.OperationOpt)

//define if the nand flash system can support external __s32er-leave operation
#define SUPPORT_EXT_INTERLEAVE              (SPINAND_EXT_INTERLEAVE & NandStorageInfo.OperationOpt)

//define if the nand flash system can support user data write separately
#define SUPPORT_OOB_SEPARATE                (SPINAND_8K_METADATA_ECC_PROTECTED & NandStorageInfo.OperationOpt)

#if 0
//define if the nand flash require to skip die addr
#define SUPPORT_LOG_BLOCK_MANAGE            (NAND_LOG_BLOCK_MANAGE & NandStorageInfo.OperationOpt)
#define LOG_BLOCK_LSB_PAGE_TYPE             ((NAND_LOG_BLOCK_LSB_TYPE & NandStorageInfo.OperationOpt)>>16)
#endif

//define if the nand flash require to skip die addr
//#define SUPPORT_WRITE_CHECK_SYNC            (NAND_FORCE_WRITE_SYNC & NandStorageInfo.OperationOpt)

//define if the nand flash requre to user max block erase cnt
#define SUPPORT_USE_MAX_BLK_ERASE_CNT		(SPINAND_MAX_BLK_ERASE_CNT & NandStorageInfo.OperationOpt)

//define if the nand flash requre to do read reclaim operation
#define SUPPORT_READ_RECLAIM				(SPINAND_READ_RECLAIM & NandStorageInfo.OperationOpt)

//define the count of the nand flash DIE in a nand flash chip
#define DIE_CNT_OF_CHIP                     (NandStorageInfo.DieCntPerChip)

//define the count of the nand flash bank in a nand flas hchip
#define BNK_CNT_OF_CHIP                     (NandStorageInfo.BankCntPerChip)

//define the Rb connect Mode
#define CONNECT_MODE                    	(NandStorageInfo.ConnectMode)

//define the count of the total nand flash bank in the nand flash storage system
#define TOTAL_BANK_CNT                      (NandStorageInfo.BankCntPerChip * NandStorageInfo.ChipCnt)

//define the count of the physical block in a nand flash DIE
#define BLOCK_CNT_OF_DIE                    (NandStorageInfo.BlkCntPerDie)

//define the count of the nand flash plane in a nand flash DIE
#define PLANE_CNT_OF_DIE                    (NandStorageInfo.PlaneCntPerDie)

//define the count of the physical page in a physical block
#define PAGE_CNT_OF_PHY_BLK                 (NandStorageInfo.PageCntPerPhyBlk)

//define the information of the nand chip connect in the nand storage system
#define CHIP_CONNECT_INFO                   (NandStorageInfo.ChipConnectInfo)

//define the nand flash access frequence parameter
#define NAND_ACCESS_FREQUENCE               (NandStorageInfo.FrequencePar)

//define the page cache for physical module processing page data
#define PHY_TMP_PAGE_CACHE                  (PageCachePool.PageCache0)

//define the spare data cache for physical module processing spare area data
#define PHY_TMP_SPARE_CACHE                 (PageCachePool.SpareCache)


#endif
