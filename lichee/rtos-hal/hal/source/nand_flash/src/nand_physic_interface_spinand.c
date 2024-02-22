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

#include "nand_cfg.h"
#include "nand_osal.h"
#include "nand_struct.h"
#include "nand_scan.h"
#include "nand_format.h"
#include "nand_scan.h"
#include "nand_info_init.h"
#include "nand_secure_storage.h"

__s32 PHY_Init(void);
__s32 PHY_Exit(void);
int set_uboot_start_and_end_block(void);

struct _nand_info *SpiNandHwInit(void)
{
	__s32 ret = 0;

	FORMAT_ERR("%s: Start Nand Hardware initializing %s %s.....\n", __func__, __DATE__, __TIME__);

	NAND_Print_Version();

	if (PHY_Init()) {
		ret = -1;
		FORMAT_ERR("[ERR]%s: PHY_Init() failed!\n", __func__);
		goto ERR_RET;
	}

	if (SCN_AnalyzeNandSystem()) {
		ret = -1;
		FORMAT_ERR("[ERR]%s: SCN_AnalyzeNandSystem() failed!\n", __func__);
		goto ERR_RET;
	}

	FMT_Init();

	MEMSET(&aw_nand_info, 0x0, sizeof(struct _nand_info));

	aw_nand_info.type = 0;
	aw_nand_info.SectorNumsPerPage = LogicArchiPar.SectCntPerLogicPage;
	aw_nand_info.BytesUserData     = 16;
	aw_nand_info.BlkPerChip        = LogicArchiPar.LogicBlkCntPerLogicDie;
	aw_nand_info.ChipNum           = LogicArchiPar.LogicDieCnt;
	aw_nand_info.PageNumsPerBlk    = LogicArchiPar.PageCntPerLogicBlk;
	aw_nand_info.FullBitmap        = FULL_BITMAP_OF_SUPER_PAGE;

	aw_nand_info.MaxBlkEraseTimes  = NandStorageInfo.MaxEraseTimes;

	if (1)
		aw_nand_info.EnableReadReclaim = 1;
	else
		aw_nand_info.EnableReadReclaim = 0;

	aw_nand_info.boot = phyinfo_buf;

	set_uboot_start_and_end_block();

	nand_secure_storage_init(1);

ERR_RET:
	if (ret < 0)
		FORMAT_ERR("%s: End Nand Hardware initializing ..... FAIL!\n", __func__);
	else
		FORMAT_DBG("%s: End Nand Hardware initializing ..... OK!\n", __func__);

	return (ret < 0) ? (struct _nand_info *)NULL : &aw_nand_info;
}

__s32  SpiNandHwExit(void)
{
	FMT_Exit();
	PHY_Exit();

	return 0;
}

