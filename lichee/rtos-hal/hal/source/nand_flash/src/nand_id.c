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
* eNand
* Nand flash driver scan module
*
* Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
* All Rights Reserved
*
* File Name : nand_id.c
* Author : Kevin.z
* Version : v0.1
* Date : 2008.03.27
* Description :
*	This file is a table, that record the physical architecture parameter
*	for every kind of nand flash, and indexed by the nand chip ID.
* Others : None at present.
*
* History :
* <Author>	<time>       <version>      <description>
* Kevin.z	 2008.03.27      0.1	  build the file
* penggang	2009.09.09      0.2	  modify the file
*
************************************************************************************************************************
*/

#include "nand_osal.h"
#include "nand_struct.h"
#include "nand_scan.h"
#include "spi_nand2.h"
#include "nand_type_spinand.h"
#include "nand_ecc_op.h"
#include "nand_id.h"

struct spi_nand_function spi_nand_function2 =
{
	m2_spi_nand_reset,
	m2_spi_nand_read_status,
	m2_spi_nand_setstatus,
	m2_spi_nand_getblocklock,
	m2_spi_nand_setblocklock,
	m2_spi_nand_getotp,
	m2_spi_nand_setotp,
	m2_spi_nand_getoutdriver,
	m2_spi_nand_setoutdriver,
	m2_erase_single_block,
	m2_write_single_page,
	m2_read_single_page,
};


//==============================================================================
// define the physical architecture parameter for all kinds of nand flash
//==============================================================================

//==============================================================================
//============================ GIGADEVICE & MIRA NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t GigaDeviceNandTbl[] =
{
	{
		.Model		= "GD5F1GQ4UCYIG",
		.NandID		= {0xc8, 0xb1, 0x48, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_EXT_INTERLEAVE | SPINAND_MAX_BLK_ERASE_CNT |
				SPINAND_READ_RECLAIM | SPINAND_ONEDUMMY_AFTER_RANDOMREAD |
				SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 50000,
		.MaxEccBits	= 8,
		.EccLimitBits	= 4,
		.Idnumber	= 0x000000,
		.EccType	= BIT3_LIMIT3_TO_6_ERR7,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	{
		.Model		= "GD5F1GQ4UBYIG",
		.NandID		= {0xc8, 0xd1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT | SPINAND_READ_RECLAIM |
				SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 50000,
		.MaxEccBits	= 8,
		.EccLimitBits	= 4,
		.Idnumber	= 0x000001,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12 | HAS_EXT_ECC_SE01,
		.EccProtectedType = SIZE16_OFF4_LEN8_OFF4,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/* GD5F2GQ4UBYIG did not check yet */
	{
		.Model		= "GD5F2GQ4UBYIG",
		.NandID		= {0xc8, 0xd2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT | SPINAND_READ_RECLAIM |
				SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 50000,
		.MaxEccBits	= 8,
		.EccLimitBits	= 4,
		.Idnumber	= 0x000003,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12 | HAS_EXT_ECC_SE01,
		.EccProtectedType = SIZE16_OFF4_LEN12,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	{
		.Model		= "F50L1G41LB(2M)",
		.NandID		= {0xc8, 0x01, 0x7f, 0x7f, 0x7f, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT | SPINAND_READ_RECLAIM |
				SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
				SPINAND_QUAD_NO_NEED_ENABLE,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 65000,
		.MaxEccBits	=  1,
		.EccLimitBits	= 1,
		.Idnumber	= 0x000002,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.pagewithbadflag = BAD_BLK_FLAG_FIRST_2_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/* GD5F4GQ4UCYIG not test, just for Baidu demo test*/
	{
		.Model		= "GD5F4GQ4UCYIG",
		.NandID		= {0xc8, 0xb4, 0x68, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 8,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OperationOpt	= SPINAND_MAX_BLK_ERASE_CNT | SPINAND_READ_RECLAIM |
				SPINAND_ONEDUMMY_AFTER_RANDOMREAD |
				SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 50000,
		.MaxEccBits	= 8,
		.EccLimitBits	= 4,
		.Idnumber	= 0x000004,
		.EccType	= BIT3_LIMIT3_TO_6_ERR7,
		.EccProtectedType = SIZE16_OFF4_LEN8_OFF4,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/*  test later */
	{
		.Model      = "GD5F1GQ5UEYIG",
		.NandID     = {0xc8, 0x51, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie   = 1024,
		.OperationOpt   = SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
						SPINAND_MAX_BLK_ERASE_CNT | SPINAND_READ_RECLAIM |
						SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM,
		.AccessFreq = 100,
		.SpiMode    = 0,
		.MaxEraseTimes  = 50000,
		.MaxEccBits = 8,
		.EccLimitBits   = 4,
		.Idnumber   = 0x000005,
		.EccType    = BIT4_LIMIT5_TO_7_ERR8 | HAS_EXT_ECC_SE01,
		.EccProtectedType = SIZE16_OFF4_LEN12,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, },
};

//==============================================================================
//============================ ATO NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t AtoNandTbl[] =
{
    //		   NAND_CHIP_ID		 DieCnt SecCnt  PagCnt   BlkCnt    OpOpt   Freq     mode   pagewithbadflag   function		     offset		maxerasetime	maxecc	ecclimit	idnumber
    //------------------------------------------------------------------------------------------------------------------------
    //no support because of no testing
	//{ {0x9b, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x007c,   100,    0, 	   0,	     &spi_nand_function0, 1,		100000,     		0x010000 },//ATO25D1GA

	//------------------------------------------------------------------------------------------------------------------------
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0, 	0,		 0, 	   0,	0x0000, 	0,	   0,    0,	0,       	  0},	 // NULL
};

//==============================================================================
//============================ Micron NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t MicronNandTbl[] =
{
    //		   NAND_CHIP_ID		 DieCnt SecCnt  PagCnt   BlkCnt    OpOpt   Freq     mode   pagewithbadflag   function		     offset		maxerasetime	maxecc	ecclimit	idnumber
    //------------------------------------------------------------------------------------------------------------------------
    //no support because of no testing
	//{ {0x2c, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00fd,   40,    0, 	   0,	     &spi_nand_function1, 1,		 50000,     	4,			1,     	0x020000 },//MT29F1G01AAADD
   // { {0x2c, 0x22, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00fd,   40,    0, 	   0,	     &spi_nand_function1, 1,		 50000,     	4,			1,     	0x020001 },//MT29F2G01AAAED
    //{ {0x2c, 0x32, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00fd,   40,    0, 	   0,	     &spi_nand_function1, 1,		 50000,     	4,			1,     	0x020002 },//MT29F4G01AAADD
	{
		.Model		= "MT29F1G01ABAGDWB",
		.NandID		= {0x2c, 0x14, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT |SPINAND_READ_RECLAIM |
				SPINAND_QUAD_READ |SPINAND_QUAD_PROGRAM|
				SPINAND_QUAD_NO_NEED_ENABLE,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 65000,
		.MaxEccBits	= 8,
		.EccLimitBits	= 4,
		.Idnumber	= 0x020000,
		.EccType	= BIT3_LIMIT5_ERR2,
		.EccProtectedType = SIZE16_OFF32_LEN16,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	{
		.Model		= "MT29F2G01ABAGDWB",
		.NandID		= {0x2c, 0x24, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT |SPINAND_READ_RECLAIM |
				SPINAND_TWO_PLANE_SELECT|
				SPINAND_QUAD_READ |SPINAND_QUAD_PROGRAM|
				SPINAND_QUAD_NO_NEED_ENABLE,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 65000,
		.MaxEccBits	= 8,
		.EccLimitBits	= 4,
		.Idnumber	= 0x020001,
		.EccType	= BIT3_LIMIT5_ERR2 ,
		.EccProtectedType = SIZE16_OFF32_LEN16,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	//------------------------------------------------------------------------------------------------------------------------
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0, 	0,		 0, 	   0,	0x0000, 	0,	   0,    0,	0,       	  0},	 // NULL
};

//==============================================================================
//============================ Mxic NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t MxicNandTbl[] =
{
	{
		.Model		= "MX35LF1GE4AB",
		.NandID		= {0xc2, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
			SPINAND_EXT_INTERLEAVE | SPINAND_MAX_BLK_ERASE_CNT |
			SPINAND_READ_RECLAIM | SPINAND_QUAD_READ |
			SPINAND_QUAD_PROGRAM,
		.AccessFreq	= 96,
		.SpiMode	= 0,
		.MaxEraseTimes  = 65000,
		.MaxEccBits	= 4,
		.EccLimitBits	= 1,
		.Idnumber	= 0x030000,
		.EccType	= BIT4_LIMIT3_TO_4_ERR15 | HAS_EXT_ECC_STATUS,
		/**
		 * MX35LF1GE4AB should use SIZE16_OFF4_LEN12, however, in order
		 * to compatibility with versions already sent to customers,
		 * which do not use general physical layout, we used
		 * SIZE16_OFF4_LEN4_OFF8 instead.
		 */
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.pagewithbadflag = BAD_BLK_FLAG_FIRST_2_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/*  test later */
	{
		.Model		= "MX35LF2GE4AD",
		.NandID		= {0xc2, 0x26, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
			SPINAND_MAX_BLK_ERASE_CNT |
			SPINAND_READ_RECLAIM | SPINAND_QUAD_READ |
			SPINAND_QUAD_PROGRAM,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 65000,
		.MaxEccBits	= 8,
		.EccLimitBits	= 4,
		.Idnumber	= 0x030001,
		.EccType	= BIT2_LIMIT1_ERR2_LIMIT3,
		.EccProtectedType = SIZE16_OFF4_LEN12,
		.pagewithbadflag = BAD_BLK_FLAG_FIRST_2_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, },
};
//==============================================================================
//============================ Winbond NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t WinbondNandTbl[] =
{
	{
		.Model		= "W25N01GVZEIG",
		.NandID		= {0xef, 0xaa, 0x21, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT |SPINAND_READ_RECLAIM |
				SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 65000,
		.MaxEccBits	= 4,
		.EccLimitBits	= 2,
		.Idnumber	= 0x040000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, },
};
//==============================================================================
//============================ Toshiba NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t ToshibaNandTbl[] =
{
    //		   NAND_CHIP_ID		 DieCnt SecCnt  PagCnt   BlkCnt    OpOpt   Freq     mode   pagewithbadflag   function		     offset		maxerasetime	maxecc	ecclimit	idnumber
    //------------------------------------------------------------------------------------------------------------------------
    //no support because of no testing
    //{ {0x98, 0xcd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     8,      64,     2048,   0x7c/*0x007d*/,   50,    0, 	   0,	     &spi_nand_function4, 1,		65000,     	8,			4,     	0x050000 },//TC58CVG2S0H
	{
		.Model		= "TC58CVG0S3HRAIG",
		.NandID		= {0x98, 0xc2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT |SPINAND_READ_RECLAIM |
				SPINAND_QUAD_READ |SPINAND_QUAD_NO_NEED_ENABLE ,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 65000,
		.MaxEccBits	= 8,
		.EccLimitBits	= 5,
		.Idnumber	= 0x050000,
		.EccType	= BIT2_LIMIT3_ERR2,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, },
};

//==============================================================================
//============================ Etron NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t EtronNandTbl[] =
{
    //		   NAND_CHIP_ID		 DieCnt SecCnt  PagCnt   BlkCnt    OpOpt   Freq     mode   pagewithbadflag   function		     offset		maxerasetime	maxecc	ecclimit	idnumber
    //------------------------------------------------------------------------------------------------------------------------
    //no support because of no testing
    //{ {0xd5, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     8,      64,     2048,   0x007d,   30,    0, 	   0,	     &spi_nand_function0, 1,		65000,     	8,			4,     	0x060000 },//EM73E044SNA

	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, },
};

//==============================================================================
//============================ HeYangTek NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t HYNandTbl[] =
{
    //		   NAND_CHIP_ID		 DieCnt SecCnt  PagCnt   BlkCnt    OpOpt   Freq     mode   pagewithbadflag   function		     offset		maxerasetime	maxecc	ecclimit	idnumber
    //no support because of no testing
    //------------------------------------------------------------------------------------------------------------------------
    //{ {0xc9, 0xd4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     8,      64,     2048,   0x007d,   30,    0, 	   0,	     &spi_nand_function0, 1,		50000,     	8,			4,     	0x070000 },//HYF4GQ4UAACBE

	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, },
};

//==============================================================================
//============================ XTX Tech NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t XTXNandTbl[] =
{
    //		   NAND_CHIP_ID		 DieCnt SecCnt  PagCnt   BlkCnt    OpOpt   Freq     mode   pagewithbadflag   function		     offset		maxerasetime	maxecc	ecclimit	idnumber
    //no support because of no testing
    //------------------------------------------------------------------------------------------------------------------------
	{
		.Model		= "XT26G01AWSEGA",
		.NandID		= {0x0b, 0xe1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT |
				SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM ,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 65000,
		.MaxEccBits	= 4,
		.EccLimitBits	= 2,
		.Idnumber	= 0x080000,
		.EccType	= BIT2_LIMIT1_ERR2_LIMIT3,
		.EccProtectedType = SIZE64_OFF8_LEN40_OFF16,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/* test later */
	{
		.Model          = "XT26G01CWSIGA",
		.NandID         = {0x0b, 0x11, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie   = 1024,
		.OperationOpt   = SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT |  SPINAND_READ_RECLAIM |
				SPINAND_DUAL_READ | SPINAND_QUAD_READ ,
		.AccessFreq     = 96,
		.SpiMode        = 0,
		.MaxEraseTimes  = 65000,
		.MaxEccBits     = 8,
		.EccLimitBits   = 4,
		.Idnumber       = 0x080001,
		.EccType        = BIT4_LIMIT4_TO_8_ECC15,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	//{ {0xa1, 0xe1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x087d,   30,    0, 	   0,	     &spi_nand_function5, 1,		65000,     	4,			2,     	0x080000 },//PN26G01AWSIUG

	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, },
};

//==============================================================================
//============================ Dosilicon NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t DSNandTbl[] =
{
	{
		.Model		= "DS35X1GAXXX",
		.NandID		= {0xe5, 0x71, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT | SPINAND_READ_RECLAIM |
				SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 65000,
		.MaxEccBits	= 4,
		.EccLimitBits	= 2,
		.Idnumber	= 0x090000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.pagewithbadflag = BAD_BLK_FLAG_FIRST_2_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, },
};

/*==============================================================================
*============================ FORESEE NAND FLASH ==============================
*==============================================================================
*/

struct __NandPhyInfoPar_t FSNandTbl[] =
{
	{
		.Model		= "FS35ND01G-S1F1QWFI000",
		.NandID		= {0xcd, 0xb1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT |SPINAND_READ_RECLAIM |
				SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 50000,
		.MaxEccBits	= 4,
		.EccLimitBits	= 2,
		.Idnumber	= 0x0a0000,
		.EccType	= BIT3_LIMIT3_TO_4_ERR7,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	{
		.Model		= "F35SQA001G-WWT",
		.NandID		= {0xcd, 0x71, 0x71, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM | SPINAND_DUAL_READ,
		.AccessFreq	= 96,
		.SpiMode	= 0,
		.MaxEraseTimes  = 50000,
		.MaxEccBits	= 4,
		.EccLimitBits	= 2,
		.Idnumber	= 0x0a0001,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.pagewithbadflag = BAD_BLK_FLAG_FRIST_1_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	//------------------------------------------------------------------------------------------------------------------------
	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, },
};

/*==============================================================================
*============================ ZETTA NAND FLASH ==============================
*==============================================================================
*/
struct __NandPhyInfoPar_t ZETTANandTbl[] =
{
	{
		.Model		= "ZD35Q1GAIB",
		.NandID		= {0xba, 0x71, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OperationOpt	= SPINAND_MULTI_READ | SPINAND_MULTI_PROGRAM |
				SPINAND_MAX_BLK_ERASE_CNT |SPINAND_READ_RECLAIM |
				SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM,
		.AccessFreq	= 100,
		.SpiMode	= 0,
		.MaxEraseTimes  = 50000,
		.MaxEccBits	= 4,
		.EccLimitBits	= 2,
		.Idnumber	= 0x0b0000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.pagewithbadflag = BAD_BLK_FLAG_FIRST_2_PAGE,
		.spi_nand_function = &spi_nand_function2,
		.MultiPlaneBlockOffset = 1,
	},
	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, },
};

//==============================================================================
//============================= DEFAULT NAND FLASH =============================
//==============================================================================
struct __NandPhyInfoPar_t DefaultNandTbl[] =
{
	/* NULL */
	{ {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, },
};

