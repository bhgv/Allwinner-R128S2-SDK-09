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

#ifndef __SPINAND_TYPE_H
#define __SPINAND_TYPE_H

//==============================================================================
//  define some constant variable for the nand flash driver used
//==============================================================================

//define the mask for the nand flash optional operation
/* spi nand flash support dual read operation */
#define SPINAND_DUAL_READ			(1<<0)
/* nand flash support page dual program operation */
#define SPINAND_DUAL_PROGRAM			(1<<1)
/* nand flash support multi-plane page read operation */
#define SPINAND_MULTI_READ			(1<<2)
/* nand flash support multi-plane page program operation */
#define SPINAND_MULTI_PROGRAM			(1<<3)

/* nand flash support external inter-leave operation, it based multi-chip */
#define SPINAND_EXT_INTERLEAVE			(1<<4)
/* nand flash support the maximum block erase cnt */
#define SPINAND_MAX_BLK_ERASE_CNT		(1<<5)
/* nand flash support to read reclaim Operation */
#define SPINAND_READ_RECLAIM			(1<<6)
/* nand flash need plane select for addr */
#define SPINAND_TWO_PLANE_SELECT		(1<<7)

/* nand flash need a dummy Byte after random fast read */
#define SPINAND_ONEDUMMY_AFTER_RANDOMREAD	(1<<8)
/* nand flash support quad read operation */
#define SPINAND_QUAD_READ			(1<<9)
/* nand flash support page quad program operation */
#define SPINAND_QUAD_PROGRAM			(1<<10)
/* nand flash only support 8KB user meta data under ecc protected */
#define SPINAND_8K_METADATA_ECC_PROTECTED	(1<<11)

/* nand flash should not enable QE register bit manually */
#define SPINAND_QUAD_NO_NEED_ENABLE		(1 << 12)

//define the size of the sector
#define SECTOR_SIZE             512                 //the size of a sector, based on byte

#define BAD_BLK_FLAG_MARK 0x03
/* the bad block flag is only in the first page */
#define BAD_BLK_FLAG_FRIST_1_PAGE 0x00
/* the bad block flag is in the first page or second page*/
#define BAD_BLK_FLAG_FIRST_2_PAGE 0x01
/* the bad block flag is in the last page */
#define BAD_BLK_FLAG_LAST_1_PAGE 0x02
/* the bad block flag is in the last 2 pages */
#define BAD_BLK_FLAG_LAST_2_PAGE 0x03

#endif //ifndef __NAND_TYPE_H

