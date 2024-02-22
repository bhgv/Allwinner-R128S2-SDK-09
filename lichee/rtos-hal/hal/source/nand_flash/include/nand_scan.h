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

#ifndef __NAND_SCAN_H__
#define __NAND_SCAN_H__

//==============================================================================
//  define nand flash manufacture ID number
//==============================================================================

#define MICRON_NAND             0x2c                //Micron nand flash manufacture number
#define GD_NAND                 0xc8                //GD and MIRA nand flash manufacture number
#define ATO_NAND                0x9b                //ATO nand flash manufacture number
#define WINBOND_NAND            0xef                //winbond nand flash manufacture number
#define MXIC_NAND               0xc2                //mxic nand flash manufacture number
#define TOSHIBA_NAND            0x98                //toshiba nand flash manufacture number
#define ETRON_NAND              0xd5                //etron nand flash manufacture number
#define HeYang_NAND             0xc9                //HeYang nand flash manufacture number
#define XTXTech_NAND            0xa1                //xtx tech nand flash manufacture number
#define XTXTech_0B_NAND         0x0b                //xtx tech nand flash manufacture number
#define DSTech_NAND             0xe5                //Dosilicon nand flash manufacture number
#define FORESEE_NAND            0xcd                //foresee nand flash manufacture number
#define ZETTA_NAND              0xba                //zetta tech nand flash manufacture number

//==============================================================================
//  define the function __s32erface for nand storage scan module
//==============================================================================

/*
************************************************************************************************************************
*                           ANALYZE NAND FLASH STORAGE SYSTEM
*
*Description: Analyze nand flash storage system, generate the nand flash physical
*             architecture parameter and connect information.
*
*Arguments  : none
*
*Return     : analyze result;
*               = 0     analyze successful;
*               < 0     analyze failed, can't recognize or some other error.
************************************************************************************************************************
*/
__u32 spinand_get_twoplane_flag(void);
__u32 SPINAND_GetLsbblksize(void);
__u32 SPINAND_GetLsbPages(void);
__u32 SPINAND_UsedLsbPages(void);
__u32 SPINAND_GetPageNo(__u32 lsb_page_no);
__u32 SPINAND_GetPageSize(void);
__u32 SPINAND_GetPhyblksize(void);
__u32 SPINAND_GetPageCntPerBlk(void);
__u32 SPINAND_GetBlkCntPerChip(void);
__u32 SPINAND_GetChipCnt(void);
__s32 SCN_AnalyzeNandSystem(void);

#endif  //ifndef __NAND_SCAN_H__
