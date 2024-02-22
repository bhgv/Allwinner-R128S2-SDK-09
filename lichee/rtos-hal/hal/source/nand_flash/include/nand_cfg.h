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

#ifndef __NAND_CFG_H__
#define __NAND_CFG_H__

#define NAND_SUPPORT_RAWNAND 0
#define NAND_CACHE_LEVEL 0
#define NAND_CAPACITY_LEVEL 0

//define the max value of the count of chip select
#define MAX_CHIP_SELECT_CNT                 (1)

#define NAND_MAX_PART_CNT                   (20)

//define the number of the chip select which connecting the boot chip
#define BOOT_CHIP_SELECT_NUM                (0)

//define the switch that if need support multi-plane program
#define CFG_SUPPORT_MULTI_PLANE_PROGRAM         (1)

//define the switch that if need support multi-plane read
#define CFG_SUPPORT_MULTI_PLANE_READ            (1)

//define the switch that if need support external inter-leave
#define CFG_SUPPORT_EXT_INTERLEAVE              (1)

//define the switch that if need support dual program
#define CFG_SUPPORT_DUAL_PROGRAM               (0)

//define the switch that if need support dual read
#define CFG_SUPPORT_DUAL_READ               (1)

//define the switch that if need support quad program
#define CFG_SUPPORT_QUAD_PROGRAM               (1)

//define the switch that if need support quad read
#define CFG_SUPPORT_QUAD_READ               (1)

//define the switch that if need support doing page copyback by send command
#define CFG_SUPPORT_PAGE_COPYBACK               (1)

//define the switch that if need support wear-levelling
#define CFG_SUPPORT_WEAR_LEVELLING              (0)

//define the switch that if need support read-reclaim
#define CFG_SUPPORT_READ_RECLAIM                (1)

//define if need check the page program status after page program immediately
#define CFG_SUPPORT_CHECK_WRITE_SYNCH           (1)

//define if need support align bank when allocating the log page
#define CFG_SUPPORT_ALIGN_NAND_BNK              (1)

#define FORMAT_WRITE_SPARE_DEBUG_ON		(0)
#define FORMAT_READ_SPARE_DEBUG_ON		(0)
#define PHY_PAGE_READ_ECC_ERR_DEBUG_ON		(0)
#define SUPPORT_READ_CHECK_SPARE		(0)
#define SUPPORT_SCAN_EDO_FOR_SDR_NAND		(0)
#define SUPPORT_UPDATE_WITH_OLD_PHYSIC_ARCH	(0)
#define SUPPORT_UPDATE_EXTERNAL_ACCESS_FREQ	(1)

/* TODO: keep them in nftl lib */
#define MIN_FREE_BLOCK_NUM_V2                     25
#define MIN_FREE_BLOCK_REMAIN                     4

#define SYS_RESERVED_BLOCK_RATIO                   5
#define NORM_RESERVED_BLOCK_RATIO                 10
#define MIN_FREE_BLOCK_NUM_RUNNING                15

#endif
