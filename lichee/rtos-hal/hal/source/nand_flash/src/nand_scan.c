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
*                                           Nand flash driver scan module
*
*                             Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
*											       All Rights Reserved
*
* File Name : nand_scan.c
*
* Author : Kevin.z
*
* Version : v0.1
*
* Date : 2008.03.27
*
* Description : This file scan the nand flash storage system, analyze the nand flash type
*               and initiate the physical architecture parameters.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Kevin.z         2008.03.27      0.1          build the file
*
************************************************************************************************************************
*/

#include <sunxi_hal_spi.h>
#include "nand_cfg.h"
#include "nand_osal.h"
#include "nand_struct.h"
#include "nand_scan.h"
#include "nand_ecc_op.h"
#include "nand_id.h"
#include "nand_info_init.h"
#include "nand_common_info.h"
#include "spic_op.h"
#include "nand_type_spinand.h"
#include "nand_physic_interface_spinand.h"

#include "nand_phy.h"

__u32 NandIDNumber = 0xffffff;
__u32 NandSupportTwoPlaneOp = 0;
__u32 CurrentDriverTwoPlaneOPCfg = 0;
__u32 bootflag = 1;

__u32 spinand_get_twoplane_flag(void)
{
	return SUPPORT_MULTI_PROGRAM ? 1 : 0;
}

__u32 SPINAND_GetLsbblksize(void)
{
	return (NandStorageInfo.SectorCntPerPage * 512 * NandStorageInfo.PageCntPerPhyBlk);
}

__u32 SPINAND_GetLsbPages(void)
{
	return NandStorageInfo.PageCntPerPhyBlk;
}

__u32 SPINAND_UsedLsbPages(void)
{
	return 0;
}

__u32 SPINAND_GetPageNo(__u32 lsb_page_no)
{
	return lsb_page_no;
}

__u32 SPINAND_GetPageSize(void)
{
	return (NandStorageInfo.SectorCntPerPage * 512);
}

__u32 SPINAND_GetPhyblksize(void)
{
	return (NandStorageInfo.SectorCntPerPage * 512 * NandStorageInfo.PageCntPerPhyBlk);
}

__u32 SPINAND_GetPageCntPerBlk(void)
{
	return NandStorageInfo.PageCntPerPhyBlk;
}

__u32 SPINAND_GetBlkCntPerChip(void)
{
	return NandStorageInfo.BlkCntPerDie * NandStorageInfo.DieCntPerChip;
}

__u32 SPINAND_GetChipCnt(void)
{
	return NandStorageInfo.ChipCnt;
}

static __s32 _GetOldPhysicArch(void *phy_arch, __u32 *good_blk_no)
{
	__s32 ret, ret2 = 0;
	__u32 b, chip = 0;
	__u32 start_blk = 20, blk_cnt = 30;
	__u8 oob[32];
	struct __NandStorageInfo_t *parch;
	struct boot_physical_param nand_op;

	parch = (struct __NandStorageInfo_t *)MALLOC(32 * 1024);
	if (!parch) {
		PRINT("%s, malloc fail\n", __func__);
		ret2 = -1;
		goto EXIT;
	}

	for (b = start_blk; b < start_blk + blk_cnt; b++) {
		nand_op.chip = chip;
		nand_op.block = b;
		nand_op.page = 0;
		nand_op.sectorbitmap = FULL_BITMAP_OF_SINGLE_PAGE;
		nand_op.mainbuf = (void *)parch;
		nand_op.oobbuf = oob;

		ret = PHY_SimpleRead(&nand_op); //PHY_SimpleRead_CurCH(&nand_op);
		PHY_DBG("_GetOldPhysicArch: chip %d, block %d, page 0, oob: 0x%x, 0x%x, 0x%x, 0x%x\n",
			nand_op.chip, nand_op.block, oob[0], oob[1], oob[2], oob[3]);
		if (ret >= 0) {
			if (oob[0] == 0x00) {
				if ((oob[1] == 0x50) && (oob[2] == 0x48) && (oob[3] == 0x59) && (oob[4] == 0x41) && (oob[5] == 0x52) && (oob[6] == 0x43) && (oob[7] == 0x48)) {
					//*((struct __NandStorageInfo_t *)phy_arch) = *parch;
					if ((parch->PlaneCntPerDie != 1) && (parch->PlaneCntPerDie != 2)) {
						PHY_DBG("_GetOldPhysicArch: get old physic arch ok,but para error: 0x%x 0x%x!\n", parch->OperationOpt, parch->PlaneCntPerDie);
					} else {
						MEMCPY(phy_arch, parch, sizeof(struct __NandStorageInfo_t));
						PHY_DBG("_GetOldPhysicArch: get old physic arch ok, 0x%x 0x%x!\n", parch->OperationOpt, parch->PlaneCntPerDie);
						ret2 = 1;
						break;
					}
				} else {
					PHY_DBG("_GetOldPhysicArch: mark bad block!\n");
				}
			} else if (oob[0] == 0xff) {
				PHY_DBG("_GetOldPhysicArch: find a good block, but no physic arch info.\n");
				ret2 = 2;//blank page
				break;
			} else {
				PHY_DBG("_GetOldPhysicArch: unkonwn1!\n");
			}
		} else {
			if (oob[0] == 0xff) {
				PHY_DBG("_GetOldPhysicArch: blank block!\n");
				ret2 = 2;
				break;
			} else if (oob[0] == 0) {
				PHY_DBG("_GetOldPhysicArch: bad block!\n");
			} else {
				PHY_DBG("_GetOldPhysicArch: unkonwn2!\n");
			}
		}
	}

	if (b == (start_blk + blk_cnt)) {
		ret2 = -1;
		*good_blk_no = 0;
	} else
		*good_blk_no = b;

EXIT:
	FREE(parch, 32 * 1024);

	return ret2;
}

static __s32 NAND_ReadPhyArch(void)
{
	__s32 ret = 0;
	struct __NandStorageInfo_t old_storage_info = { 0 };
	__u32 good_blk_no;

    int is_phyinfo_empty(struct _boot_info *info);
	if (is_phyinfo_empty(phyinfo_buf) != 1) {
		NandStorageInfo.FrequencePar = phyinfo_buf->storage_info.config.frequence;

		if (phyinfo_buf->storage_info.config.support_two_plane == 1) {
			NandStorageInfo.OperationOpt |= SPINAND_MULTI_READ;
			NandStorageInfo.OperationOpt |= SPINAND_MULTI_PROGRAM;
			NandStorageInfo.PlaneCntPerDie = 2;
		} else {
			NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_READ;
			NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_PROGRAM;
			NandStorageInfo.PlaneCntPerDie = 1;
		}

		if (phyinfo_buf->storage_info.config.support_dual_read == 1) {
			NandStorageInfo.OperationOpt |= SPINAND_DUAL_READ;
		} else {
			NandStorageInfo.OperationOpt &= ~SPINAND_DUAL_READ;
		}

		if (phyinfo_buf->storage_info.config.support_dual_write == 1) {
			NandStorageInfo.OperationOpt |= SPINAND_DUAL_PROGRAM;
		} else {
			NandStorageInfo.OperationOpt &= ~SPINAND_DUAL_PROGRAM;
		}

		if (phyinfo_buf->storage_info.config.support_quad_write == 1) {
			NandStorageInfo.OperationOpt |= SPINAND_QUAD_PROGRAM;
		} else {
			NandStorageInfo.OperationOpt &= ~SPINAND_QUAD_PROGRAM;
		}

		if (phyinfo_buf->storage_info.config.support_quad_read == 1) {
			NandStorageInfo.OperationOpt |= SPINAND_QUAD_READ;
		} else {
			NandStorageInfo.OperationOpt &= ~SPINAND_QUAD_READ;
		}

		return 0;
	}

	ret = _GetOldPhysicArch(&old_storage_info, &good_blk_no);
	if (ret == 1) {
		PHY_ERR("NAND_ReadPhyArch: get old physic arch ok, use old cfg, now:0x%x 0x%x - old:0x%x 0x%x!\n",
			NandStorageInfo.PlaneCntPerDie, NandStorageInfo.OperationOpt,
			old_storage_info.PlaneCntPerDie, old_storage_info.OperationOpt);
		NandStorageInfo.PlaneCntPerDie = old_storage_info.PlaneCntPerDie;
		if (NandStorageInfo.PlaneCntPerDie == 1) {
			NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_READ;
			NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_PROGRAM;
		}
		if (NandStorageInfo.PlaneCntPerDie == 2) {
			NandStorageInfo.OperationOpt |= SPINAND_MULTI_READ;
			NandStorageInfo.OperationOpt |= SPINAND_MULTI_PROGRAM;
		}
	} else if (ret == 2) {
		PHY_ERR("NAND_ReadPhyArch: blank page!\n");
	} else {
		PHY_ERR("NAND_ReadPhyArch: get para error!\n");
	}
	return ret;
}

/*
************************************************************************************************************************
*                           SEARCH NAND PHYSICAL ARCHITECTURE PARAMETER
*
*Description: Search the nand flash physical architecture parameter from the parameter table
*             by nand chip ID.
*
*Arguments  : pNandID           the pointer to nand flash chip ID;
*             pNandArchiInfo    the pointer to nand flash physical architecture parameter.
*
*Return     : search result;
*               = 0     search successful, find the parameter in the table;
*               < 0     search failed, can't find the parameter in the table.
************************************************************************************************************************
*/
static __s32 _SearchNandArchi(__u8 *pNandID, struct __NandPhyInfoPar_t *pNandArchInfo)
{
	__s32 i = 0, j = 0, k = 0;
	__u32 id_match_tbl[5] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
	__u32 id_bcnt;
	struct __NandPhyInfoPar_t *tmpNandManu;

	//analyze the manufacture of the nand flash
	switch (pNandID[0]) {
	case GD_NAND:
		tmpNandManu = &GigaDeviceNandTbl[0];
		break;

	case ATO_NAND:
		tmpNandManu = &AtoNandTbl[0];
		break;

	case MICRON_NAND:
		tmpNandManu = &MicronNandTbl[0];
		break;

	case WINBOND_NAND:
		tmpNandManu = &WinbondNandTbl[0];
		break;

	case MXIC_NAND:
		tmpNandManu = &MxicNandTbl[0];
		break;

	case TOSHIBA_NAND:
		tmpNandManu = &ToshibaNandTbl[0];
		break;

	case ETRON_NAND:
		tmpNandManu = &EtronNandTbl[0];
		break;

	case HeYang_NAND:
		tmpNandManu = &HYNandTbl[0];
		break;

	case XTXTech_NAND:
	case XTXTech_0B_NAND:
		tmpNandManu = &XTXNandTbl[0];
		break;

	case DSTech_NAND:
		tmpNandManu = &DSNandTbl[0];
		break;
	case FORESEE_NAND:
		tmpNandManu = &FSNandTbl[0];
		break;
		//manufacture is unknown, search parameter from default nand table
	case ZETTA_NAND:
		tmpNandManu = &ZETTANandTbl[0];
		break;
	default:
		tmpNandManu = &DefaultNandTbl[0];
		break;
	}

	MEMCPY(pNandArchInfo, tmpNandManu, sizeof(struct __NandPhyInfoPar_t));

	//search the nand architecture parameter from the given manufacture nand table by nand ID
	while (tmpNandManu[i].NandID[0] != 0xff) {
		//compare 6 byte id
		id_bcnt = 1;
		for (j = 1; j < 6; j++) {
			//0xff is matching all ID value
			if ((pNandID[j] != tmpNandManu[i].NandID[j]) && (tmpNandManu[i].NandID[j] != 0xff))
				break;

			if (tmpNandManu[i].NandID[j] != 0xff)
				id_bcnt++;
		}

		if (j == 6) {
			/*4 bytes of the nand chip ID are all matching, search parameter successful*/
			if (id_bcnt == 2)
				id_match_tbl[0] = i;
			else if (id_bcnt == 3)
				id_match_tbl[1] = i;
			else if (id_bcnt == 4)
				id_match_tbl[2] = i;
			else if (id_bcnt == 5)
				id_match_tbl[3] = i;
			else if (id_bcnt == 6)
				id_match_tbl[4] = i;
		}

		//prepare to search the next table item
		i++;
	}

	for (k = 4; k >= 0; k--) {
		if (id_match_tbl[k] != 0xffff) {
			i = id_match_tbl[k];
			MEMCPY(pNandArchInfo, tmpNandManu + i, sizeof(struct __NandPhyInfoPar_t));
			return 0;
		}
	}

	//search nand architecture parameter failed
	return -1;
}

static __s32 SPINAND_SetCfgregFeature(__u32 spi_no, __u32 chip_no)
{
	__u8 status_lock = 0;
	__u8 status_otp = 0;
	__s32 ret1 = 0, ret2 = 0, ret3 = 0, ret4 = 0;
	struct __NandStorageInfo_t  *info = &NandStorageInfo;

	ret1 = info->spi_nand_function->spi_nand_setblocklock(
		       spi_no, chip_no, 0);
	if (0xef == info->NandChipId[0])
		ret2 = info->spi_nand_function->spi_nand_setotp(spi_no, chip_no,
				SPI_NAND_ECC_ENABLE | SPI_NAND_BUF_MODE);
	else if (info->OperationOpt & SPINAND_QUAD_NO_NEED_ENABLE)
		ret2 = info->spi_nand_function->spi_nand_setotp(spi_no, chip_no,
				SPI_NAND_ECC_ENABLE);
	else
		ret2 = info->spi_nand_function->spi_nand_setotp(spi_no, chip_no,
				SPI_NAND_ECC_ENABLE | SPI_NAND_QE);

	ret3 = info->spi_nand_function->spi_nand_getblocklock(
		       spi_no, chip_no, &status_lock);
	ret4 = info->spi_nand_function->spi_nand_getotp(
		       spi_no, chip_no, &status_otp);
	if (ret1 || ret2 || ret3 || ret4) {
		PHY_ERR("set cfgreg feature fail: %d %d %d %d\n", ret1, ret2,
			ret3, ret4);
		return NAND_OP_FALSE;
	}
	PHY_DBG("status_lock: 0x%02x, status_otp: 0x%02x\n", status_lock, status_otp);
	return NAND_OP_TRUE;
}

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
__s32 SCN_AnalyzeNandSystem(void)
{
	__s32 i, result;
	__u8 tmpChipID[8];
	struct __NandPhyInfoPar_t tmpNandPhyInfo;

	// init nand flash storage information to default value
	NandStorageInfo.ChipCnt               = 1;
	NandStorageInfo.ChipConnectInfo       = 1;
	NandStorageInfo.ConnectMode           = 1;
	NandStorageInfo.BankCntPerChip        = 1;
	NandStorageInfo.DieCntPerChip         = 1;
	NandStorageInfo.PlaneCntPerDie        = 1;
	NandStorageInfo.SectorCntPerPage      = 4;
	NandStorageInfo.PageCntPerPhyBlk      = 64;
	NandStorageInfo.BlkCntPerDie          = 1024;
	NandStorageInfo.OperationOpt          = 0;
	NandStorageInfo.FrequencePar          = 10;
	NandStorageInfo.SpiMode               = 0;
	NandStorageInfo.pagewithbadflag       = 0;
	NandStorageInfo.MultiPlaneBlockOffset = 1;
	NandStorageInfo.MaxEraseTimes         = 50000;
	NandStorageInfo.MaxEccBits            = 1;
	NandStorageInfo.EccLimitBits          = 1;
	NandStorageInfo.EccType               = ECC_TYPE_ERR;
	NandStorageInfo.EccProtectedType      = ECC_PROTECTED_TYPE;
	NandStorageInfo.Model                 = NULL;

	// read nand flash chip ID from boot chip
	result = PHY_ReadNandId_0(BOOT_CHIP_SELECT_NUM, tmpChipID);
	if (result) {
		PHY_ERR("read id fail 0\n");
		return -1;
	}

	PHY_DBG("SPI nand ID: %x %x\n", *((__u32 *)tmpChipID),
		*((__u32 *)tmpChipID + 1));

	// search the nand flash physical architecture parameter by nand ID
	result = _SearchNandArchi(tmpChipID, &tmpNandPhyInfo);
	if (result) {
		// read nand flash chip ID from boot chip
		result = PHY_ReadNandId_1(BOOT_CHIP_SELECT_NUM, tmpChipID);
		if (result) {
			PHY_ERR("read id fail 1\n");
			return -1;
		}
		PHY_DBG("SPI nand ID: %x %x\n", *((__u32 *)tmpChipID),
			*((__u32 *)tmpChipID + 1));

		// search the nand flash physical architecture parameter by nand
		// ID
		result = _SearchNandArchi(tmpChipID, &tmpNandPhyInfo);
		if (result) {
			PHY_ERR("_SearchNandArchi fail\n");
			return -1;
		}
	}

	/* storage_type = 2; */

	// set the nand flash physical architecture parameter
	NandStorageInfo.BankCntPerChip                = tmpNandPhyInfo.DieCntPerChip;
	NandStorageInfo.DieCntPerChip                 = tmpNandPhyInfo.DieCntPerChip;
	NandStorageInfo.PlaneCntPerDie                = 2;
	NandStorageInfo.SectorCntPerPage              = tmpNandPhyInfo.SectCntPerPage;
	NandStorageInfo.PageCntPerPhyBlk              = tmpNandPhyInfo.PageCntPerBlk;
	NandStorageInfo.BlkCntPerDie                  = tmpNandPhyInfo.BlkCntPerDie;
	NandStorageInfo.OperationOpt                  = tmpNandPhyInfo.OperationOpt;
	NandStorageInfo.FrequencePar                  = tmpNandPhyInfo.AccessFreq;
	NandStorageInfo.NandChipId[0]                 = tmpNandPhyInfo.NandID[0];
	NandStorageInfo.NandChipId[1]                 = tmpNandPhyInfo.NandID[1];
	NandStorageInfo.NandChipId[2]                 = tmpNandPhyInfo.NandID[2];
	NandStorageInfo.NandChipId[3]                 = tmpNandPhyInfo.NandID[3];
	NandStorageInfo.NandChipId[4]                 = tmpNandPhyInfo.NandID[4];
	NandStorageInfo.NandChipId[5]                 = tmpNandPhyInfo.NandID[5];
	NandStorageInfo.NandChipId[6]                 = tmpNandPhyInfo.NandID[6];
	NandStorageInfo.NandChipId[7]                 = tmpNandPhyInfo.NandID[7];
	NandStorageInfo.SpiMode                       = tmpNandPhyInfo.SpiMode;
	NandStorageInfo.pagewithbadflag               = tmpNandPhyInfo.pagewithbadflag;
	NandStorageInfo.MaxEraseTimes                 = tmpNandPhyInfo.MaxEraseTimes;
	NandStorageInfo.MaxEccBits                    = tmpNandPhyInfo.MaxEccBits;
	NandStorageInfo.EccLimitBits                  = tmpNandPhyInfo.EccLimitBits;
	NandStorageInfo.MultiPlaneBlockOffset         = tmpNandPhyInfo.MultiPlaneBlockOffset;
	NandStorageInfo.spi_nand_function             = tmpNandPhyInfo.spi_nand_function;
	NandStorageInfo.Idnumber                      = tmpNandPhyInfo.Idnumber;
	NandStorageInfo.EccType                       = tmpNandPhyInfo.EccType;
	NandStorageInfo.EccProtectedType              = tmpNandPhyInfo.EccProtectedType;
	NandStorageInfo.Model                         = tmpNandPhyInfo.Model;

	// reset the nand flash chip on boot chip select
	result = PHY_ResetChip(BOOT_CHIP_SELECT_NUM);
	if (result) {
		return -1;
	}

	/* set max block erase cnt and enable read reclaim flag */
	//   MaxBlkEraseTimes = tmpNandPhyInfo.MaxEraseTimes;
	/* in order to support to parse external script, record id ctl number */
	NandIDNumber = tmpNandPhyInfo.Idnumber;

	/* record current nand flash whether support two plane program */
	if (NandStorageInfo.OperationOpt & SPINAND_MULTI_PROGRAM)
		NandSupportTwoPlaneOp = 1;
	else
		NandSupportTwoPlaneOp = 0;

	/* record current driver cfg for two plane operation */
	if (CFG_SUPPORT_MULTI_PLANE_PROGRAM == 0)
		CurrentDriverTwoPlaneOPCfg = 0;
	else {
		if (NandSupportTwoPlaneOp)
			CurrentDriverTwoPlaneOPCfg = 1;
		else
			CurrentDriverTwoPlaneOPCfg = 0;
	}
	PHY_DBG(
		"[SCAN_DBG] NandTwoPlaneOp: %d, DriverTwoPlaneOPCfg: %d, 0x%x \n",
		NandSupportTwoPlaneOp, CurrentDriverTwoPlaneOPCfg,
		((NandIDNumber << 4) ^ 0xffffffff));

	if (!CFG_SUPPORT_READ_RECLAIM) {
		NandStorageInfo.OperationOpt &= ~SPINAND_READ_RECLAIM;
	}

	if (!CFG_SUPPORT_DUAL_PROGRAM) {
		NandStorageInfo.OperationOpt &= ~SPINAND_DUAL_PROGRAM;
	}

	if (!CFG_SUPPORT_DUAL_READ) {
		NandStorageInfo.OperationOpt &= ~SPINAND_DUAL_READ;
	}
	for (i = 1; i < MAX_CHIP_SELECT_CNT; i++) {
		// read the nand chip ID from current nand flash chip
		PHY_ReadNandId_0((__u32)i, tmpChipID);
		// check if the nand flash id same as the boot chip
		if ((tmpChipID[0] == NandStorageInfo.NandChipId[0]) &&
		    (tmpChipID[1] == NandStorageInfo.NandChipId[1]) &&
		    ((tmpChipID[2] == NandStorageInfo.NandChipId[2]) ||
		     (NandStorageInfo.NandChipId[2] == 0xff)) &&
		    ((tmpChipID[4] == NandStorageInfo.NandChipId[3]) ||
		     (NandStorageInfo.NandChipId[3] == 0xff)) &&
		    ((tmpChipID[4] == NandStorageInfo.NandChipId[4]) ||
		     (NandStorageInfo.NandChipId[4] == 0xff)) &&
		    ((tmpChipID[5] == NandStorageInfo.NandChipId[5]) ||
		     (NandStorageInfo.NandChipId[5] == 0xff))) {
			NandStorageInfo.ChipCnt++;
			NandStorageInfo.ChipConnectInfo |= (1 << i);
		} else {
			// reset current nand flash chip
			PHY_ResetChip((__u32)i);

			PHY_ReadNandId_1((__u32)i, tmpChipID);
			if ((tmpChipID[0] == NandStorageInfo.NandChipId[0]) &&
			    (tmpChipID[1] == NandStorageInfo.NandChipId[1]) &&
			    ((tmpChipID[2] == NandStorageInfo.NandChipId[2]) ||
			     (NandStorageInfo.NandChipId[2] == 0xff)) &&
			    ((tmpChipID[4] == NandStorageInfo.NandChipId[3]) ||
			     (NandStorageInfo.NandChipId[3] == 0xff)) &&
			    ((tmpChipID[4] == NandStorageInfo.NandChipId[4]) ||
			     (NandStorageInfo.NandChipId[4] == 0xff)) &&
			    ((tmpChipID[5] == NandStorageInfo.NandChipId[5]) ||
			     (NandStorageInfo.NandChipId[5] == 0xff))) {
				NandStorageInfo.ChipCnt++;
				NandStorageInfo.ChipConnectInfo |= (1 << i);
			}
		}

		// reset current nand flash chip
		PHY_ResetChip((__u32)i);
	}

	// process the rb connect infomation
	{
		NandStorageInfo.ConnectMode = 0xff;

		if ((NandStorageInfo.ChipCnt == 1) &&
		    (NandStorageInfo.ChipConnectInfo & (1 << 0))) {
			NandStorageInfo.ConnectMode = 1;
		} else if (NandStorageInfo.ChipCnt == 2) {
			if ((NandStorageInfo.ChipConnectInfo & (1 << 0)) &&
			    (NandStorageInfo.ChipConnectInfo & (1 << 1)))
				NandStorageInfo.ConnectMode = 2;
			else if ((NandStorageInfo.ChipConnectInfo & (1 << 0)) &&
				 (NandStorageInfo.ChipConnectInfo & (1 << 2)))
				NandStorageInfo.ConnectMode = 3;
			else if ((NandStorageInfo.ChipConnectInfo & (1 << 0)) &&
				 (NandStorageInfo.ChipConnectInfo & (1 << 3)))
				NandStorageInfo.ConnectMode = 4;
			else if ((NandStorageInfo.ChipConnectInfo & (1 << 0)) &&
				 (NandStorageInfo.ChipConnectInfo & (1 << 4)))
				NandStorageInfo.ConnectMode = 5;

		}

		else if (NandStorageInfo.ChipCnt == 4) {
			NandStorageInfo.ConnectMode = 6;
		}

		if (NandStorageInfo.ConnectMode == 0xff) {
			PHY_ERR("%s : check spi nand connect fail, ChipCnt =  "
				"%x, ChipConnectInfo = %x \n",
				__FUNCTION__, NandStorageInfo.ChipCnt,
				NandStorageInfo.ChipConnectInfo);
			return -1;
		}
	}

	if (!CFG_SUPPORT_MULTI_PLANE_PROGRAM) {
		NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_READ;
		NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_PROGRAM;
	}
	// process the plane count of a die and the bank count of a chip
	if (!SUPPORT_MULTI_PROGRAM) {
		NandStorageInfo.PlaneCntPerDie = 1;
	}

	for (i = 0; i < NandStorageInfo.ChipCnt; i++)
		SPINAND_SetCfgregFeature(0, i);

	// process the external inter-leave operation
	if (CFG_SUPPORT_EXT_INTERLEAVE) {
		if (NandStorageInfo.ChipCnt > 1) {
			NandStorageInfo.OperationOpt |= SPINAND_EXT_INTERLEAVE;
		} else {
			NandStorageInfo.OperationOpt &=
				(~SPINAND_EXT_INTERLEAVE);
		}
	} else {
		NandStorageInfo.OperationOpt &= (~SPINAND_EXT_INTERLEAVE);
	}
	PHY_ChangeMode();

	//reinit spi with new frequence
	int ret;
	struct hal_spi_master *spim = &nand_spim;

	hal_spi_deinit(spim->port);

	spim->cfg.clock_frequency = NAND_ACCESS_FREQUENCE;
	spim->cfg.clock_frequency *= 1000 * 1000;
	ret = hal_spi_init(spim->port, &spim->cfg);
	if (ret != HAL_SPI_MASTER_OK) {
		printf("init spi master failed - %d\n", ret);
		return ret;
	}
    int physic_info_read(void);
	physic_info_read();
	SCAN_DBG("\n[SCAN_DBG] crc_enable: 0x%x\n", phyinfo_buf->enable_crc);
	if (SUPPORT_UPDATE_WITH_OLD_PHYSIC_ARCH) {
		NAND_ReadPhyArch();
	}

	if ((!CFG_SUPPORT_MULTI_PLANE_PROGRAM) ||
	    (SECTOR_CNT_OF_SINGLE_PAGE > 8)) {
		NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_READ;
		NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_PROGRAM;
	}
	// process the plane count of a die and the bank count of a chip
	if (!SUPPORT_MULTI_PROGRAM) {
		NandStorageInfo.PlaneCntPerDie = 1;
	}

	// print nand flash physical architecture parameter
	SCAN_DBG("\n\n");
	SCAN_DBG("[SCAN_DBG] ============== DAIZEBIN Nand Architecture Parameter==============\n");
	SCAN_DBG("[SCAN_DBG]    Nand Model:           %s\n", NandStorageInfo.Model);
	SCAN_DBG("[SCAN_DBG]    Nand Chip ID:         0x%x 0x%x\n",
					 (NandStorageInfo.NandChipId[0] << 0) |
					 (NandStorageInfo.NandChipId[1] << 8) |
					 (NandStorageInfo.NandChipId[2] << 16) |
					 (NandStorageInfo.NandChipId[3] << 24),
					 (NandStorageInfo.NandChipId[4] << 0) |
					 (NandStorageInfo.NandChipId[5] << 8) |
					 (NandStorageInfo.NandChipId[6] << 16) |
					 (NandStorageInfo.NandChipId[7] << 24));
	SCAN_DBG("[SCAN_DBG]    Nand Chip Count:      0x%x\n", NandStorageInfo.ChipCnt);
	SCAN_DBG("[SCAN_DBG]    Nand Chip Connect:    0x%x\n", NandStorageInfo.ChipConnectInfo);
	SCAN_DBG("[SCAN_DBG]    Sector Count Of Page: 0x%x\n", NandStorageInfo.SectorCntPerPage);
	SCAN_DBG("[SCAN_DBG]    Page Count Of Block:  0x%x\n", NandStorageInfo.PageCntPerPhyBlk);
	SCAN_DBG("[SCAN_DBG]    Block Count Of Die:   0x%x\n", NandStorageInfo.BlkCntPerDie);
	SCAN_DBG("[SCAN_DBG]    Plane Count Of Die:   0x%x\n", NandStorageInfo.PlaneCntPerDie);
	SCAN_DBG("[SCAN_DBG]    Die Count Of Chip:    0x%x\n", NandStorageInfo.DieCntPerChip);
	SCAN_DBG("[SCAN_DBG]    Bank Count Of Chip:   0x%x\n", NandStorageInfo.BankCntPerChip);
	SCAN_DBG("[SCAN_DBG]    Optional Operation:   0x%x\n", NandStorageInfo.OperationOpt);
	SCAN_DBG("[SCAN_DBG]    Access Frequence:     0x%x\n", NandStorageInfo.FrequencePar);
	SCAN_DBG("[SCAN_DBG] =======================================================\n\n");

	return 0;
}
