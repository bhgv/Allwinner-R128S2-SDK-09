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
#include "nand_type_spinand.h"
#include "nand_format.h"
#include "nand_info_init.h"
#include "nand_physic_interface_spinand.h"

#include "nand_phy.h"
#include "nand_scan.h"

struct __LogicArchitecture_t            LogicArchiPar = {0};

//define some local variable
__u32 DieCntOfNand = 0;               //the count of dies in a nand chip
__u32 SuperBlkCntOfDie = 0;    //the count of the super blocks in a Die


static __s32 _CalculatePhyOpPar(struct __PhysicOpPara_t *pPhyPar, __u32 nDie, __u32 nBlock, __u32 nPage)
{
	__u32   tmpDieNum, tmpBnkNum, tmpBlkNum, tmpPageNum;

	//FORMAT_DBG("[FORMAT_DBG] Calculate the physical operation parameters.\n"
	//               "             ZoneNum:0x%x, BlockNum:0x%x, PageNum: 0x%x\n", nZone, nBlock, nPage);

	//calcualte the Die number by the zone number
	//tmpDieNum = nZone / ZONE_CNT_OF_DIE;
	tmpDieNum = nDie;


	if (SUPPORT_EXT_INTERLEAVE) {
		//nand flash support external inter-leave but don't support internal inter-leave, the block
		//number is virtual block number add the die block base, the bank number is the page number
		//model the inter-leave bank count, the page number is vitual page number divide the inter-leave
		//bank count
		//tmpBnkNum =  (nPage % INTERLEAVE_BANK_CNT) + (tmpDieNum * INTERLEAVE_BANK_CNT);
		tmpBnkNum = (nPage % INTERLEAVE_BANK_CNT) + (tmpDieNum / DIE_CNT_OF_CHIP) * INTERLEAVE_BANK_CNT;
		tmpBlkNum = nBlock + ((tmpDieNum % DIE_CNT_OF_CHIP) * (BLOCK_CNT_OF_DIE / PLANE_CNT_OF_DIE));
		tmpPageNum = nPage / INTERLEAVE_BANK_CNT;
	}

	else { //if(!SUPPORT_INT_INTERLEAVE && !SUPPORT_EXT_INTERLEAVE)
		//nand flash don't internal inter-leave and extern inter-leave either, the bank number is the
		//die number divide the die count of chip, the block number is the virtual block number add
		//the die block base in the chip, the page number is same as the virtual page number
		tmpBnkNum = tmpDieNum / DIE_CNT_OF_CHIP;
		tmpBlkNum = nBlock + (tmpDieNum % DIE_CNT_OF_CHIP) * (BLOCK_CNT_OF_DIE / PLANE_CNT_OF_DIE);
		tmpPageNum = nPage;
	}

	//set the physical operation paramter by the bank number, block number and page number
	pPhyPar->BankNum = tmpBnkNum;
	pPhyPar->PageNum = tmpPageNum;
	pPhyPar->BlkNum = tmpBlkNum;

	//FORMAT_DBG("         Calculate Result: BankNum 0x%x, BlkNum 0x%x, PageNum 0x%x\n", tmpBnkNum, tmpBlkNum, tmpPageNum);

	//calculate physical operation parameter successful
	return 0;
}

static __s32 _ReadCheckSpare(__u8 *spare, __u32 die_no, __u32 blk_no, __u32 page_no)
{
	__s32 ret = 0;
	//__s32 i;

	if (SUPPORT_READ_CHECK_SPARE == 1) {
		if (spare == NULL) {
			return 0;
		}

		if ((spare[0] == 0xff) && (spare[1] == 0xff) && (spare[2] == 0xff) && (spare[3] == 0xff) && (spare[4] == 0xff)) {
			return 0;
		}

		if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0xbb) && (spare[3] == 0xff)) {
			return 0; //read bad block table;
		}

		if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0xbb) && (spare[3] == 0x1)) {
			return 0; //read bad block table new version;
		}

		if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0xaa) && (spare[3] == 0xff)) {
			return 0;
		}

		if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0xcc) && (spare[3] == 0xff)) {
			return 0; //new bad block table;
		}

		if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0xcc) && (spare[3] == 0x1)) {
			return 0; //new bad block table;
		}

		if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0xdd) && (spare[3] == 0xff)) {
			return 0;
		}

		if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0xee) && (spare[3] == 0xff)) {
			return 0;
		}

		if (aw_nand_info.partition_nums == 0) {
			//PHY_DBG("%s: build phy partition(%d,%d,%d)...\n", __func__,die_no, blk_no,page_no);
			return 0;
		}

		if ((spare[0] == 0xff) && (spare[1] == 0x55) && (spare[2] == 0x55) && (spare[3] == 0x55) && (spare[4] == 0x55)) {
			//PHY_DBG("%s: filled invalid page(%d,%d,%d)...\n", __func__,die_no, blk_no,page_no);
			return 0;
		}

		if (page_no == (PAGE_CNT_OF_LOGIC_BLK - 1)) {
			if ((spare[1] != 0xaa) || (spare[2] != 0xaa) || (spare[3] != 0xff) || (spare[4] != 0xff)) {
				//PHY_ERR("%s: read last page spare error %d,%d,%d !\n", __func__, die_no, blk_no,page_no);
				//for (i=0; i<16; i++)
				//{
				//	PHY_ERR("%x ", spare[i]);
				//}
				//PHY_ERR("\n");
				ret = 1;
			}
		} else {
			if ((spare[1] & 0xf0) != 0xc0) {
				//PHY_ERR("%s: read page spare error %d,%d,%d !\n",__func__, die_no, blk_no,page_no);
				//for (i=0; i<16; i++)
				//{
				//	PHY_ERR("%x ", spare[i]);
				//}
				//PHY_ERR("\n");
				ret = 1;
			}
		}

		return ret;
	} else {
		return 0;
	}
}

/*
************************************************************************************************************************
*                       READ PAGE DATA FROM VIRTUAL BLOCK
*
*Description: Read page data from virtual block, the block is composed by several physical block.
*             It is named super block too.
*
*Arguments  : nDieNum   the number of the DIE, which the page is belonged to;
*             nBlkNum   the number of the virtual block in the die;
*             nPage     the number of the page in the virtual block;
*             Bitmap    the bitmap of the sectors need access in the page;
*             pBuf      the pointer to the page data buffer;
*             pSpare    the pointer to the spare data buffer.
*
*Return     : read result;
*               = 0     read page data successful;
*               < 0     read page data failed.
************************************************************************************************************************
*/
static __s32 _VirtualPageRead(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare)
{
	__s32 result;
	__u64 spare_bitmap = 0;
	__s32 i, cnt, err;
	struct __PhysicOpPara_t tmpPhyPage;

	if ((nDieNum >= LOGIC_DIE_CNT) || (nBlkNum >= BLK_CNT_OF_LOGIC_DIE) || (nPage >= PAGE_CNT_OF_LOGIC_BLK)) {
		PHY_ERR("%s: Fatal err -0, Wrong input parameter, nDieNum: %d/%d  nBlkNum: %d/%d  nPage: %d/%d\n",
			__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE, nPage, PAGE_CNT_OF_LOGIC_BLK);
		return -1;
	}

	if ((pBuf == NULL) && (pSpare == NULL) && (SectBitmap)) {
		PHY_ERR("%s: Fatal err -1, Wrong input parameter, pBuf: 0x%08x  pSpare: 0x%08x  SectBitmap: 0x%lx\n",
			__func__, pBuf, pSpare, SectBitmap);
		return -1;
	}

	if ((pBuf == NULL) && (pSpare == NULL) && (SectBitmap == 0)) {
		PHY_DBG("%s: Warning -0, pBuf: 0x%08x  pSpare: 0x%08x  SectBitmap: 0x%lx\n",
			__func__, pBuf, pSpare, SectBitmap);
		return 0;
	}

	if (FORMAT_READ_SPARE_DEBUG_ON) {
		for (i = 0; i < 16; i++) {
			*(FORMAT_SPARE_BUF + i) = 0x99;
		}
	}

	//calculate the physical operation parameter by the die number, block number and page number
	_CalculatePhyOpPar(&tmpPhyPage, nDieNum, nBlkNum, nPage);

	//set the sector bitmap in the page, the main data buffer and the spare data buffer
	if ((pBuf == NULL) && pSpare) { //if ((SectBitmap==0) && (pBuf==NULL) && pSpare)
		//1. read spare data only
		//read spare data in first physical page only
		spare_bitmap = 0x0; ///////////FULL_BITMAP_OF_SUPER_PAGE;

		tmpPhyPage.SectBitmap = spare_bitmap;
		tmpPhyPage.SDataPtr = FORMAT_SPARE_BUF;
		tmpPhyPage.MDataPtr = FORMAT_PAGE_BUF;

#if(1)
		result = PHY_PageReadSpare(&tmpPhyPage);
#else
		result = PHY_PageRead(&tmpPhyPage);
#endif
	} else {
		//2. read full page or several sectors
		tmpPhyPage.SectBitmap = SectBitmap;
		tmpPhyPage.SDataPtr = FORMAT_SPARE_BUF;
		tmpPhyPage.MDataPtr = pBuf;

		result = PHY_PageRead(&tmpPhyPage);
	}

	if (pSpare)
		MEMCPY(pSpare, tmpPhyPage.SDataPtr, 16); //8 --> 16

	if (FORMAT_READ_SPARE_DEBUG_ON) {
		if ((pSpare) && (nPage == (PAGE_CNT_OF_LOGIC_BLK - 1))) {
			err = 0;
			//check default value
			cnt = 0;
			for (i = 0; i < 5; i++) {
				if (*((__u8 *)pSpare + i) == 0x99)
					cnt++;
				else
					break;
			}

			if (*((__u8 *)pSpare + 1) == 0xaa) {
				if (*((__u8 *)pSpare + 2) != 0xaa) {
					err = 1;
				}
				if (*((__u8 *)pSpare + 3) != 0xff) {
					err = 1;
				}

				if (*((__u8 *)pSpare + 4) != 0xff) {
					err = 1;
				}
			} else {
				err = 1;
			}

			if (err) {
				cnt = 0;
				for (i = 0; i < 16; i++) {
					if (*((__u8 *)pSpare + i) == 0xFF)
						cnt++;
					else
						break;
				}
				if (cnt == 16)
					return 0;

				PHY_ERR("===%s: nDieNum: %d/%d  nBlkNum: %d/%d  nPage: %d/%d\n", __func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE, nPage, PAGE_CNT_OF_LOGIC_BLK);

				for (i = 0; i < 16; i++) {
					PHY_ERR("0x%x ", *((__u8 *)pSpare + i));
				}
				PHY_ERR("\n");
				return 0;
			}
		}
	}

	return result;
}

__s32 spinand_super_page_read(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare)
{
	__s32 ret = 0;
	__s32 retry_cnt = 2;

	while (retry_cnt != 0) {
		ret = _VirtualPageRead(nDieNum, nBlkNum, nPage, SectBitmap, pBuf, pSpare);
		if (ret < 0)
			break;

		if (_ReadCheckSpare((__u8 *)pSpare, nDieNum, nBlkNum, nPage) == 0) {
			break;
		} else {
			PHY_ERR("%s: NPVP retry_cnt: %d\n", __func__, retry_cnt);
		}
		retry_cnt--;

	}

	return ret;
}

/*
************************************************************************************************************************
*                       WRITE PAGE DATA TO VIRTUAL BLOCK
*
*Description: Write page data to virtual block, the block is composed by several physical block.
*             It is named super block too.
*
*Arguments  : nDieNum   the number of the DIE, which the page is belonged to;
*             nBlkNum   the number of the virtual block in the die;
*             nPage     the number of the page in the virtual block;
*             Bitmap    the bitmap of the sectors need access in the page;
*             pBuf      the pointer to the page data buffer;
*             pSpare    the pointer to the spare data buffer.
*
*Return     : write result;
*               = 0     write page data successful;
*               < 0     write page data failed.
************************************************************************************************************************
*/
__s32 spinand_super_page_write(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare)
{
	__s32 result;
	__s32 i, err;
	struct __PhysicOpPara_t tmpPhyPage;

	if ((nDieNum >= LOGIC_DIE_CNT) || (nBlkNum >= BLK_CNT_OF_LOGIC_DIE) || (nPage >= PAGE_CNT_OF_LOGIC_BLK)) {
		PHY_ERR("%s: Fatal err -0, Wrong input parameter, nDieNum: %d/%d  nBlkNum: %d/%d  nPage: %d/%d\n",
			__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE, nPage, PAGE_CNT_OF_LOGIC_BLK);
		return -1;
	}

	if ((pBuf == NULL) && (pSpare == NULL) && (SectBitmap)) {
		PHY_ERR("%s: Fatal err -1, Wrong input parameter, pBuf: 0x%08x  pSpare: 0x%08x  SectBitmap: 0x%x\n",
			__func__, pBuf, pSpare, SectBitmap);
		return -1;
	}

	if (SectBitmap != FULL_BITMAP_OF_SUPER_PAGE) {
		PHY_ERR("%s: Fatal err -2, Wrong input parameter, unaligned write page SectBitmap: 0x%llx, 0x%llx\n",
			__func__, SectBitmap, FULL_BITMAP_OF_SUPER_PAGE);
		return -1;
	}

	if ((pBuf == NULL) && (pSpare == NULL) && (SectBitmap == 0)) {
		PHY_DBG("%s: Warning -0, pBuf: 0x%08x  pSpare: 0x%08x  SectBitmap: 0x%lx\n",
			__func__, pBuf, pSpare, SectBitmap);
		return 0;
	}

	if (FORMAT_WRITE_SPARE_DEBUG_ON) {
		if ((pSpare) && (nPage == (PAGE_CNT_OF_LOGIC_BLK - 1))) {
			err = 0;
			//check default value
			//cnt = 0;

			if (*((__u8 *)pSpare + 1) == 0xaa) {
				if (*((__u8 *)pSpare + 2) != 0xaa) {
					err = 1;
				}
				if (*((__u8 *)pSpare + 3) != 0xff) {
					err = 1;
				}

				if (*((__u8 *)pSpare + 4) != 0xff) {
					err = 1;
				}
			} else {
				err = 1;
			}

			if (err) {
				PHY_ERR("%s: nDieNum: %d/%d  nBlkNum: %d/%d  nPage: %d/%d\n", __func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE, nPage, PAGE_CNT_OF_LOGIC_BLK);

				for (i = 0; i < 16; i++) {
					PHY_ERR("0x%x ", *((__u8 *)pSpare + i));
				}
				PHY_ERR("\n");

				//Gavin-20130815, do not return -1
				//return -1;
			}
		}
	}

	//calculate the physical operation parameter by te die number, block number and page number
	//_CalculatePhyOpPar(&tmpPhyPage, nDieNum * ZONE_CNT_OF_DIE, nBlkNum, nPage);
	_CalculatePhyOpPar(&tmpPhyPage, nDieNum, nBlkNum, nPage);

	//set the sector bitmap in the page, the main data buffer and the spare data buffer
	tmpPhyPage.SectBitmap = SectBitmap;
	tmpPhyPage.MDataPtr = pBuf;
	tmpPhyPage.SDataPtr = FORMAT_SPARE_BUF;

	//process spare area data
	if (pSpare) {
		MEMCPY(FORMAT_SPARE_BUF, pSpare, 16); //8-->16
	} else {
		MEMSET(FORMAT_SPARE_BUF, 0xff, 16);
	}

	result = PHY_PageWrite(&tmpPhyPage);

	return result;
}

/*
************************************************************************************************************************
*                       ERASE VIRTUAL BLOCK
*
*Description: Erase a virtual blcok.
*
*Arguments  : nDieNum   the number of the DIE, which the block is belonged to;
*             nBlkNum   the number of the virtual block in the die.
*
*Return     : erase result;
*               = 0     virtual block erase successful;
*               < 0     virtual block erase failed.
************************************************************************************************************************
*/
__s32 spinand_super_block_erase(__u32 nDieNum, __u32 nBlkNum)
{
	__s32 i, ret = 0;
	struct __PhysicOpPara_t tmpPhyBlk;

	if ((nDieNum >= LOGIC_DIE_CNT) || (nBlkNum >= BLK_CNT_OF_LOGIC_DIE)) {
		PHY_ERR("%s: Fatal err -0, Wrong input parameter, nDieNum: %d/%d  nBlkNum: %d/%d \n",
			__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE);
		return -1;
	}

	//erase every block belonged to different banks
	for (i = 0; i < INTERLEAVE_BANK_CNT; i++) {
		_CalculatePhyOpPar(&tmpPhyBlk, nDieNum, nBlkNum, i);

		ret |= PHY_BlockErase(&tmpPhyBlk);
	}

	return ret;
}

/*
************************************************************************************************************************
*                       CHECK BAD BLOCK
*
*Description: Check a virtual block is bad or good.
*
*Arguments  : nDieNum   the number of the DIE, which the block is belonged to;
*             nBlkNum   the number of the virtual block in the die.
*
*Return     : erase result;
*               = 0     virtual block is a good block;
*               = 1     virtual block is a bad block.
************************************************************************************************************************
*/
__s32 spinand_super_badblock_check(__u32 nDieNum, __u32 nBlkNum)
{
	struct __PhysicOpPara_t tmpPhyPage = { 0 };
	__u32 tmp_logic_die, tmp_super_blk;
	__s32 tmpPageNum[4];
	__u32 tmpBnkNum, tmpPage, tmpBadFlag;
	__s32 i;
	__u8 *temp_spare_ptr;

	__s32 result __attribute__((unused));

	if ((nDieNum >= LOGIC_DIE_CNT) || (nBlkNum >= BLK_CNT_OF_LOGIC_DIE)) {
		PHY_ERR("%s: Fatal err -0, Wrong input parameter, nDieNum: %d/%d  nBlkNum: %d/%d \n",
			__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum,
			BLK_CNT_OF_LOGIC_DIE);
		return -1;
	}

	//PHY_DBG("[PHY_DBG]%s: Start checking block %d in die %d ....\n", __func__, nBlkNum, nDieNum);

	tmp_logic_die = nDieNum;
	tmp_super_blk = nBlkNum;

	//initiate the number of the pages which need be read, the first page is read always, because the
	//the logical information is stored in the first page, other pages is read for check bad block flag
	tmpPageNum[0] = 0;
	tmpPageNum[1] = -1;
	tmpPageNum[2] = -1;
	tmpPageNum[3] = -1;

	//analyze the number of pages which need be read
	switch (BAD_BLK_FLAG_PST & BAD_BLK_FLAG_MARK) {
	case BAD_BLK_FLAG_FRIST_1_PAGE:
		//the bad block flag is in the first page, same as the logical information, just read 1 page is ok
		break;

	case BAD_BLK_FLAG_FIRST_2_PAGE:
		//the bad block flag is in the first page or the second page, need read the first page and the second page
		tmpPageNum[1] = 1;
		break;

	case BAD_BLK_FLAG_LAST_1_PAGE:
		//the bad block flag is in the last page, need read the first page and the last page
		tmpPageNum[1] = PAGE_CNT_OF_PHY_BLK - 1;
		break;

	case BAD_BLK_FLAG_LAST_2_PAGE:
		//the bad block flag is in the last 2 page, so, need read the first page, the last page and the last-1 page
		tmpPageNum[1] = PAGE_CNT_OF_PHY_BLK - 1;
		tmpPageNum[2] = PAGE_CNT_OF_PHY_BLK - 2;
		break;
	}

	//initiate the bad block flag
	tmpBadFlag = 0;

	//the super block is composed of several physical blocks in several banks
	for (tmpBnkNum = 0; tmpBnkNum < INTERLEAVE_BANK_CNT; tmpBnkNum++) {
		for (i = 3; i >= 0; i--) {
			if (tmpPageNum[i] == -1) {
				//need not check page
				continue;
			}

			//calculate the number of the page in the super block to get spare data
			tmpPage =
				tmpPageNum[i] * INTERLEAVE_BANK_CNT + tmpBnkNum;

			//calculate parameter
			_CalculatePhyOpPar(&tmpPhyPage, tmp_logic_die,
					   tmp_super_blk, tmpPage);
			tmpPhyPage.SectBitmap = FULL_BITMAP_OF_SUPER_PAGE;
			tmpPhyPage.SDataPtr   = FORMAT_SPARE_BUF;
			tmpPhyPage.MDataPtr   = FORMAT_PAGE_BUF;

			temp_spare_ptr = (__u8 *)FORMAT_SPARE_BUF;
			//check if the block is a bad block

			result = PHY_PageReadSpare(&tmpPhyPage);

			if ((*temp_spare_ptr != 0xff)) {
				//set the bad flag of the physical block
				tmpBadFlag = 1;
			}
		}
	}

	/*check if the block is a bad block*/
	if (tmpBadFlag == 1) {
		/*the physical block is a bad block, set bad block flag in the logical information buffer*/
		PHY_DBG("[PHY_DBG] Find a bad block (NO. 0x%x) in the Die 0x%x\n",
			tmp_super_blk, tmp_logic_die);
		return 1; /*bad block*/
	} else {
		return 0; /*good block*/
	}
}

/*
************************************************************************************************************************
*                       MARK BAD BLOCK
*
*Description: Mark a virtual block as a bad block.
*
*Arguments  : nDieNum   the number of the DIE, which the block is belonged to;
*             nBlkNum   the number of the virtual block in the die.
*
*Return     : erase result;
*               = 0     mark virtual block successfully;
*               < 0     mark virtual block failed.
************************************************************************************************************************
*/
__s32 spinand_super_badblock_mark(__u32 nDieNum, __u32 nBlkNum)
{
	__s32   i;
	__u8 tmpSpare[16];
	__s32 result;

	if ((nDieNum >= LOGIC_DIE_CNT) || (nBlkNum >= BLK_CNT_OF_LOGIC_DIE)) {
		PHY_ERR("%s: Fatal err -0, Wrong input parameter, nDieNum: %d/%d  nBlkNum: %d/%d \n",
			__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE);
		return -1;
	}

	PHY_DBG("[PHY_DBG]%s: Start marking block %d in die %d ....\n", __func__, nBlkNum, nDieNum);

	//set bad block flag to the spare data write to nand flash
	for (i = 0; i < 16; i++)
		tmpSpare[i] = 0;

	if (spinand_super_badblock_check(nDieNum, nBlkNum)) {
		PHY_DBG("[PHY_DBG]%s: Block %d in die %d is already a bad block!\n", __func__, nBlkNum, nDieNum);
		return 0;
	}

	//erase block
	result = spinand_super_block_erase(nDieNum, nBlkNum);
	//if (result<0) {
	//	return result;
	//}

	for (i = 0; i < INTERLEAVE_BANK_CNT; i++) {
		N_mdelay(1);
		//write the bad block flag on the first page
		result = spinand_super_page_write(nDieNum, nBlkNum, i, FULL_BITMAP_OF_SUPER_PAGE, FORMAT_PAGE_BUF, (void *)tmpSpare);
		//if (result<0) {
		//	break;
		//}

		//write the bad block flag on the last page
		result |= spinand_super_page_write(nDieNum, nBlkNum, PAGE_CNT_OF_SUPER_BLK - INTERLEAVE_BANK_CNT + i, \
						   FULL_BITMAP_OF_SUPER_PAGE, FORMAT_PAGE_BUF, (void *)tmpSpare);
		//if (result<0) {
		//	break;
		//}
	}

	if (result < 0) {
		PHY_ERR("[PHY_ERR]%s: Mark bad block %d in die %d failed!\n", __func__, nBlkNum, nDieNum);
	} else {
		if (spinand_super_badblock_check(nDieNum, nBlkNum))
			PHY_DBG("[PHY_DBG]%s: Mark bad block %d in die %d ok!\n", __func__, nBlkNum, nDieNum);
		else
			PHY_ERR("[PHY_ERR]%s: Mark bad block %d in die %d failed!\n", __func__, nBlkNum, nDieNum);
	}
	return result;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int spinand_physic_erase_block(unsigned int chip, unsigned int block)
{
	struct boot_physical_param  para;
	int  ret;

	para.chip = chip;
	para.block = block;
	ret = PHY_SimpleErase(&para) ;

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int spinand_physic_read_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf)
{
	struct boot_physical_param  para;
	int  ret;

	para.chip  = chip;
	para.block = block;
	para.page  = page;
	para.sectorbitmap = FULL_BITMAP_OF_SINGLE_PAGE;
	para.mainbuf = (void *) mbuf;
	para.oobbuf = sbuf;

	ret = PHY_SimpleRead(&para);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int spinand_physic_write_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf)
{
	struct boot_physical_param  para;
	int  ret;

	para.chip  = chip;
	para.block = block;
	para.page  = page;
	para.sectorbitmap = FULL_BITMAP_OF_SINGLE_PAGE;
	para.mainbuf = (void *) mbuf;
	para.oobbuf = sbuf;

	ret = PHY_SimpleWrite(&para);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:good block  other:bad block
*Note         :
*****************************************************************************/
int spinand_physic_bad_block_check(unsigned int chip, unsigned int block)
{
	struct boot_physical_param para;
	unsigned char spare[64];
	unsigned char *buf;
	unsigned int start_page, num, i, page_size, page_cnt_per_blk;

	page_size	 = SPINAND_GetPageSize();
	page_cnt_per_blk = SPINAND_GetPageCntPerBlk();
	buf		 = MALLOC(page_size);

	/* analyze the number of pages which need be read */
	switch (BAD_BLK_FLAG_PST & BAD_BLK_FLAG_MARK) {
	case BAD_BLK_FLAG_FRIST_1_PAGE:
		/*
		 * the bad block flag is in the first page, same as the logical
		 * information, just read 1 page is ok
		 */
		start_page = 0;
		num	= 1;
		break;
	case BAD_BLK_FLAG_FIRST_2_PAGE:
		/*
		 * the bad block flag is in the first page or the second page,
		 * need read the first page and the second page
		 */
		start_page = 0;
		num	= 2;
		break;
	case BAD_BLK_FLAG_LAST_1_PAGE:
		/*
		 * the bad block flag is in the last page, need read the first
		 * page and the last page
		 */
		start_page = page_cnt_per_blk - 1;
		num	= 1;
		break;
	case BAD_BLK_FLAG_LAST_2_PAGE:
		/*
		 * the bad block flag is in the last 2 page, so, need read the
		 * first page, the last page and the last-1 page
		 */
		start_page = page_cnt_per_blk - 2;
		num	= 2;
		break;
	default:
		start_page = 0;
		num	= 1;
		break;
	}

	//read and check 1st page
	para.chip	 = chip;
	para.block	= block;
	para.page	 = 0;
	para.sectorbitmap = FULL_BITMAP_OF_SINGLE_PAGE;
	para.mainbuf      = (void *)buf;
	para.oobbuf       = spare;
	PHY_SimpleRead(&para);

	if (spare[0] != 0xff) {
		PHY_ERR("bad block check block fail: %d %d %d ", chip, block,
			para.page);
		PHY_ERR("sdata: %02x %02x %02x %02x \n", spare[0], spare[1],
			spare[2], spare[3]);
		FREE(buf, page_size);
		return -1;
	}

	//read and check other pages
	for (i = 0, para.page = start_page; i < num; i++) {
		para.chip	 = chip;
		para.block	= block;
		para.page	 = 0;
		para.sectorbitmap = FULL_BITMAP_OF_SINGLE_PAGE;
		para.mainbuf      = (void *)buf;
		para.oobbuf       = spare;

		PHY_SimpleRead(&para);

		if (spare[0] != 0xff) {
			FREE(buf, page_size);
			return -1;
		}
		para.page++;
	}

	FREE(buf, page_size);
	return 0;
}

int spinand_physic_bad_block_mark(unsigned int chip, unsigned int block)
{
	__s32	i;
	__s32 ret;
	unsigned char spare[64];
	unsigned char *buf;
	unsigned int page_size, page_cnt_per_blk;

	page_size = SPINAND_GetPageSize();
	page_cnt_per_blk = SPINAND_GetPageCntPerBlk();
	buf = MALLOC(page_size);
	if (!buf) {
		PHY_ERR("bad mark:malloc fail!\n");
		return -1;
	}

	MEMSET(buf, 0x00, page_size);
	MEMSET(spare, 0x00, 64);

	//erase block
	ret = spinand_physic_erase_block(chip, block);
	//if (result<0) {
	//	return result;
	//}

	for (i = 0; i < page_cnt_per_blk; i++) {
		ret = spinand_physic_write_page(chip, block, i, FULL_BITMAP_OF_SINGLE_PAGE, buf, spare);
		if (ret < 0) {
			PHY_ERR("Mark bad block %d in chip %d failed!\n", chip, block);
		}
	}

	return ret;
}



/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int spinand_physic_block_copy(unsigned int chip_s, unsigned int block_s, unsigned int chip_d, unsigned int block_d)
{
	int i, ret = 0;
	unsigned char spare[64];
	unsigned char *buf;
	unsigned int page_size, page_cnt_per_blk;

	page_size = SPINAND_GetPageSize();
	page_cnt_per_blk = SPINAND_GetPageCntPerBlk();

	buf = MALLOC(page_size);

	for (i = 0; i < page_cnt_per_blk; i++) {
		ret |= spinand_physic_read_page(chip_s, block_s, i, FULL_BITMAP_OF_SINGLE_PAGE, buf, spare);
		ret |= spinand_physic_write_page(chip_d, block_d, i, FULL_BITMAP_OF_SINGLE_PAGE, buf, spare);
	}

	FREE(buf, page_size);

	return ret;
}

__s32 FMT_Init(void)
{
	LogicArchiPar.SectCntPerLogicPage = NandStorageInfo.SectorCntPerPage * NandStorageInfo.PlaneCntPerDie;
	LogicArchiPar.PageCntPerLogicBlk = NandStorageInfo.PageCntPerPhyBlk * NandStorageInfo.BankCntPerChip;
	if (SUPPORT_EXT_INTERLEAVE) {
		if (NandStorageInfo.ChipCnt >= 2)
			LogicArchiPar.PageCntPerLogicBlk *= 2;
	}

	//init some local variable
	DieCntOfNand = NandStorageInfo.DieCntPerChip / NandStorageInfo.BankCntPerChip;
	if (!SUPPORT_EXT_INTERLEAVE) {
		DieCntOfNand *= NandStorageInfo.ChipCnt;
	}
	if (SUPPORT_EXT_INTERLEAVE) {
		if (NandStorageInfo.ChipCnt >= 2)
			DieCntOfNand *= (NandStorageInfo.ChipCnt / 2);
	}
	LogicArchiPar.LogicDieCnt = DieCntOfNand;
	SuperBlkCntOfDie = NandStorageInfo.BlkCntPerDie / NandStorageInfo.PlaneCntPerDie;
	LogicArchiPar.LogicBlkCntPerLogicDie = SuperBlkCntOfDie;

	FORMAT_DBG("\n");
	FORMAT_DBG("[FORMAT_DBG] ===========Logical Architecture Parameter===========\n");
	FORMAT_DBG("[FORMAT_DBG]    Page Count of Logic Block:  0x%x\n", LogicArchiPar.PageCntPerLogicBlk);
	FORMAT_DBG("[FORMAT_DBG]    Sector Count of Logic Page: 0x%x\n", LogicArchiPar.SectCntPerLogicPage);
	FORMAT_DBG("[FORMAT_DBG]    Block Count of Die:         0x%x\n", SuperBlkCntOfDie);
	FORMAT_DBG("[FORMAT_DBG]    Die Count:                  0x%x\n", LogicArchiPar.LogicDieCnt);
	FORMAT_DBG("[FORMAT_DBG] ===================================================\n");

	return 0;
}

__s32 FMT_Exit(void)
{
	//release memory resource
	return 0;
}

