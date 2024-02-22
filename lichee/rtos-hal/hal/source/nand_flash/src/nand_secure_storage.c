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

/*****************************************************************************/
#define _SECURE_STORAGE_C_
/*****************************************************************************/

#include "nand_osal.h"
#include "nand_struct.h"
#include "nand_info_init.h"

#include "nand_physic_interface.h"

#define MAX_SECURE_STORAGE_ITEM 32

#define MIN_SECURE_STORAGE_BLOCK_NUM 8
#define MAX_SECURE_STORAGE_BLOCK_NUM 50

int nand_secure_storage_block     = 0;
int nand_secure_storage_block_bak = 0;

static int nand_secure_storage_read_one(unsigned int block, int item,
				 unsigned char *mbuf, unsigned int len);

static int nand_secure_storage_repair(int flag);
static int nand_secure_storage_update(void);

/*Interfaces are defined but not used, mainly to eliminate compilation warnings*/
#define __maybe_unused		__attribute__((unused))

static int nand_secure_storage_test_read(unsigned int item, unsigned int block) __maybe_unused;
static int nand_secure_storage_test(unsigned int item) __maybe_unused;
static int nand_secure_storage_clean(void) __maybe_unused;

/*****************************************************************************/

static __u32 NAND_Get_SecureBlock_Start(void)
{
	int block_start;

	block_start = aw_nand_info.boot->uboot_next_block;
	return block_start;
}

static int nand_is_support_secure_storage(void)
{
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_secure_storage_write_init(unsigned int block)
{
	int ret = -1;
	unsigned char *mbuf;
	unsigned int page_size, i, page_cnt_per_blk;
	unsigned char spare[64];

	page_size	 = NAND_GetPageSize();
	page_cnt_per_blk = NAND_GetPageCntPerBlk();
	mbuf		 = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	MEMSET(mbuf, 0, page_size);
	MEMSET(spare, 0xff, 64);
	spare[1] = 0xaa;
	spare[2] = 0x5c;
	spare[3] = 0x00;
	spare[4] = 0x00;
	spare[5] = 0x12;
	spare[6] = 0x34;

	if (nand_physic_erase_block(0, block) == 0) {
		for (i = 0; i < page_cnt_per_blk; i++) {
			nand_physic_write_page(0, block, i, page_size / 512,
					       mbuf, spare);
		}
		mbuf[0] = 0X11;
		ret     = nand_secure_storage_read_one(block, 0, mbuf, 2048);
	}

	FREE(mbuf, page_size);
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_secure_storage_clean_data(unsigned int num)
{
	if (num == 0)
		nand_physic_erase_block(0, nand_secure_storage_block);
	if (num == 1)
		nand_physic_erase_block(0, nand_secure_storage_block_bak);
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_secure_storage_clean(void)
{
	nand_secure_storage_clean_data(0);
	nand_secure_storage_clean_data(1);
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int is_nand_secure_storage_block(unsigned int block)
{
	unsigned char *mbuf;
	int page_size, ret = 0;
	unsigned char spare[64];

	page_size = NAND_GetPageSize();
	mbuf      = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	nand_physic_read_page(0, block, 0, page_size / 512, mbuf, spare);
	if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0x5c)) {
		ret = 1;
	}

	FREE(mbuf, page_size);
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_secure_storage_init(int flag)
{
	unsigned int nBlkNum;
	int ret;

	nand_secure_storage_block     = 0;
	nand_secure_storage_block_bak = 0;

	if (nand_is_support_secure_storage() != 0)
		return 0;

	nBlkNum = NAND_Get_SecureBlock_Start();

	for (; nBlkNum < MAX_SECURE_STORAGE_BLOCK_NUM; nBlkNum++) {
		ret = is_nand_secure_storage_block(nBlkNum);
		if (ret == 1) {
			nand_secure_storage_block = nBlkNum;
			break;
		}
	}

	for (nBlkNum += 1; nBlkNum < MAX_SECURE_STORAGE_BLOCK_NUM; nBlkNum++) {
		ret = is_nand_secure_storage_block(nBlkNum);
		if (ret == 1) {
			nand_secure_storage_block_bak = nBlkNum;
			break;
		}
	}

	if ((nand_secure_storage_block < MIN_SECURE_STORAGE_BLOCK_NUM) ||
	    (nand_secure_storage_block >= nand_secure_storage_block_bak)) {
		nand_secure_storage_block     = 0;
		nand_secure_storage_block_bak = 0;
		PHY_ERR("nand secure storage fail: %d,%d\n",
			nand_secure_storage_block,
			nand_secure_storage_block_bak);
		ret = -1;
	} else {
		if (flag != 0) {
			nand_secure_storage_update();
		}
		ret = 0;
		PHY_DBG("nand secure storage ok: %d,%d\n",
			nand_secure_storage_block,
			nand_secure_storage_block_bak);
	}
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         : only called when first build
*****************************************************************************/
int nand_secure_storage_first_build(unsigned int start_block)
{
	int block = -1;
	unsigned int nBlkNum;
	int ret;

	if (nand_is_support_secure_storage() != 0)
		return start_block;

	if ((nand_secure_storage_block_bak > MIN_SECURE_STORAGE_BLOCK_NUM) &&
	    (nand_secure_storage_block_bak < MAX_SECURE_STORAGE_BLOCK_NUM)) {
		PHY_DBG("start block:%d\n",
			nand_secure_storage_block_bak + 1);
		return nand_secure_storage_block_bak + 1;
	}

	nBlkNum = start_block;

	for (; nBlkNum < MAX_SECURE_STORAGE_BLOCK_NUM; nBlkNum++) {
		if (nand_physic_bad_block_check(0, nBlkNum) == 0) {
			ret = is_nand_secure_storage_block(nBlkNum);
			if (ret != 1) {
				nand_secure_storage_write_init(nBlkNum);
			}
			nand_secure_storage_block = nBlkNum;
			break;
		}
	}

	for (nBlkNum += 1; nBlkNum < MAX_SECURE_STORAGE_BLOCK_NUM; nBlkNum++) {
		if (nand_physic_bad_block_check(0, nBlkNum) == 0) {
			ret = is_nand_secure_storage_block(nBlkNum);
			if (ret != 1) {
				nand_secure_storage_write_init(nBlkNum);
			}
			nand_secure_storage_block_bak = nBlkNum;
			break;
		}
	}

	if ((nand_secure_storage_block < MIN_SECURE_STORAGE_BLOCK_NUM) ||
	    (nand_secure_storage_block >= nand_secure_storage_block_bak)) {
		PHY_ERR("nand secure storage firsr build  fail: %d,%d\n",
			nand_secure_storage_block,
			nand_secure_storage_block_bak);
		nand_secure_storage_block     = 0;
		nand_secure_storage_block_bak = 0;
		block			      = start_block + 2;
	} else {
		block = nand_secure_storage_block_bak + 1;
		PHY_DBG("nand secure storage firsr build  ok: %d,%d\n",
			nand_secure_storage_block,
			nand_secure_storage_block_bak);
	}
	return block;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static unsigned int nand_secure_check_sum(unsigned char *mbuf, unsigned int len)
{
	unsigned int check_sum, i;
	unsigned int *p;

	p	 = (unsigned int *)mbuf;
	check_sum = 0x1234;
	len >>= 2;

	for (i = 0; i < len; i++) {
		check_sum += p[i];
	}
	return check_sum;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_secure_storage_read_one(unsigned int block, int item,
				 unsigned char *mbuf, unsigned int len)
{
	unsigned char spare[64];
	unsigned int check_sum, read_sum, page_cnt_per_blk;
	int page_size;

	page_size = NAND_GetPageSize();

	page_cnt_per_blk = NAND_GetPageCntPerBlk();

	for (; item < page_cnt_per_blk; item += MAX_SECURE_STORAGE_ITEM) {
		MEMSET(spare, 0, 64);
		nand_physic_read_page(0, block, item, page_size / 512, mbuf,
				      spare);
		if ((spare[1] != 0xaa) || (spare[2] != 0x5c)) {
			continue;
		}

		check_sum = nand_secure_check_sum(mbuf, len);

		read_sum = spare[3];
		read_sum <<= 8;
		read_sum |= spare[4];
		read_sum <<= 8;
		read_sum |= spare[5];
		read_sum <<= 8;
		read_sum |= spare[6];

		if (read_sum == check_sum) {
			if (read_sum == 0x1234) {
				return 1;
			}
			return 0;
		} else {
			PHY_DBG("spare_sum:0x%x,check_sum:0x%x\n", read_sum,
				check_sum);
		}
	}
	return -1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :api
*****************************************************************************/
int nand_secure_storage_read(int item, unsigned char *buf, unsigned int len)
{
	int ret = 0;
	unsigned char *mbuf = NULL;
	int page_size, i;
	int pages_cnt_per_item = 0;
	unsigned int item_tmp = item;

	if (nand_is_support_secure_storage())
		return 0;

	NAND_PhysicLock();

	if (len % 1024) {
		PHY_ERR("error! len = %d, agali 1024Bytes\n", len);
		ret = -1;
		goto out;
	}

	page_size = NAND_GetPageSize();
	
	if (len <= page_size)
		pages_cnt_per_item = 1;
	else {
		pages_cnt_per_item = len / page_size;
		len		   = page_size;
	}

	mbuf = MALLOC(page_size);
	if (!mbuf) {
		PHY_ERR("malloc error! %s[%d]\n", __FUNCTION__, __LINE__);
		ret = -1;
		goto out;
	}

	for (i = 0; i < pages_cnt_per_item; i++) {
		item = item_tmp * pages_cnt_per_item + i;
		ret  = nand_secure_storage_read_one(nand_secure_storage_block,
						    item, mbuf, len);
		if (ret != 0)
			ret = nand_secure_storage_read_one(
				      nand_secure_storage_block_bak, item, mbuf, len);

		if (ret == 0) {
			if (pages_cnt_per_item == 1)
				MEMCPY(buf, mbuf, len);
			else
				MEMCPY(buf + i * len, mbuf, len);
		} else {
			if (pages_cnt_per_item == 1)
				MEMSET(buf, 0, len);
			else
				MEMSET(buf + i * len, 0, len);
		}
	}

	FREE(mbuf, page_size);
out:
	NAND_PhysicUnLock();

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :api
*****************************************************************************/
int nand_secure_storage_write(int item, unsigned char *buf, unsigned int len)
{
	int ret = -1;
	unsigned char *mbuf = NULL;
	unsigned int page_size, i, check_sum, page, page_cnt_per_blk;
	unsigned char spare[64];
	int pages_cnt_per_item = 0;

	if (nand_is_support_secure_storage())
		return 0;

	NAND_PhysicLock();

	nand_secure_storage_update();

	page_size	= NAND_GetPageSize();
	page_cnt_per_blk = NAND_GetPageCntPerBlk();

	if (len % 1024) {
		PHY_ERR("error! len = %d, agali 1024Bytes\n", len);
		ret = -1;
		goto out;
	}

	
	if (len <= page_size)
		pages_cnt_per_item = 1;
	else {
		pages_cnt_per_item = len / page_size;
		len		   = page_size;
	}

	mbuf = MALLOC(page_size);
	if (!mbuf) {
		PHY_ERR("malloc error! %s[%d]\n", __FUNCTION__, __LINE__);
		ret = -1;
		goto out;
	}

	nand_physic_erase_block(0, nand_secure_storage_block_bak);
	for (i = 0; i < MAX_SECURE_STORAGE_ITEM * pages_cnt_per_item; i++) {
		if ((i / pages_cnt_per_item) != item) {
			nand_physic_read_page(0, nand_secure_storage_block, i,
					      page_size / 512, mbuf, spare);
		} else {
			MEMSET(mbuf, 0, page_size);
			if (pages_cnt_per_item == 1)
				MEMCPY(mbuf, buf, len);
			else
				MEMCPY(mbuf,
				       buf +
				       page_size *
				       (i % pages_cnt_per_item),
				       page_size);
			check_sum = nand_secure_check_sum(mbuf, len);

			MEMSET(spare, 0xff, 64);
			spare[1] = 0xaa;
			spare[2] = 0x5c;
			spare[3] = (unsigned char)(check_sum >> 24);
			spare[4] = (unsigned char)(check_sum >> 16);
			spare[5] = (unsigned char)(check_sum >> 8);
			spare[6] = (unsigned char)(check_sum);
		}
		if (nand_physic_write_page(0, nand_secure_storage_block_bak, i,
					   page_size / 512, mbuf, spare))
			PHY_ERR("nand_secure_storage_write fail\n");
	}

	for (i = MAX_SECURE_STORAGE_ITEM * pages_cnt_per_item;
	     i < page_cnt_per_blk; i++) {
		page = i % (MAX_SECURE_STORAGE_ITEM * pages_cnt_per_item);
		nand_physic_read_page(0, nand_secure_storage_block_bak, page,
				      page_size / 512, mbuf, spare);
		nand_physic_write_page(0, nand_secure_storage_block_bak, i,
				       page_size / 512, mbuf, spare);
	}

	nand_secure_storage_repair(2);

	FREE(mbuf, page_size);
	ret = 0;
out:
	NAND_PhysicUnLock();
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : -1;0;1;2
*Note         :
*****************************************************************************/
static int nand_secure_storage_check(void)
{
	unsigned char *mbuf;
	int i, page_size, ret = -1, ret1, ret2;

	page_size = NAND_GetPageSize();
	mbuf      = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	for (i = 0; i < MAX_SECURE_STORAGE_ITEM; i++) {
		ret1 = nand_secure_storage_read_one(nand_secure_storage_block,
						    i, mbuf, page_size);
		ret2 = nand_secure_storage_read_one(
			       nand_secure_storage_block_bak, i, mbuf, page_size);
		if (ret1 != ret2) {
			break;
		}
		if (ret1 < 0) {
			break;
		}
	}

	if ((ret1 < 0) && (ret2 < 0)) {
		ret = -1;
		PHY_ERR("nand secure storage check fail:%d\n", i);
		goto ss_check_out;
	}

	if (ret1 == ret2) {
		ret = 0;
		goto ss_check_out;
	}

	if ((ret1 == 0) || (ret2 < 0)) {
		ret = 1;
	}

	if ((ret2 == 0) || (ret1 < 0)) {
		ret = 2;
	}

ss_check_out:

	FREE(mbuf, page_size);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_secure_storage_repair(int flag)
{
	int ret = 0;
	unsigned int block_s, block_d;

	if (flag == 0) {
		return 0;
	}

	if (flag == 1) {
		block_s = nand_secure_storage_block;
		block_d = nand_secure_storage_block_bak;
	}

	if (flag == 2) {
		block_s = nand_secure_storage_block_bak;
		block_d = nand_secure_storage_block;
	}

	ret |= nand_physic_erase_block(0, block_d);

	ret |= nand_physic_block_copy(0, block_s, 0, block_d);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_secure_storage_update(void)
{
	int ret, retry = 0;

	while (1) {
		ret = nand_secure_storage_check();
		if (ret == 0) {
			break;
		}

		retry++;
		if (ret < 0) {
			PHY_ERR("secure storage fail 1\n");
			return -1;
		}
		if (ret > 0) {
			PHY_DBG("secure storage repair:%d\n", ret);
			nand_secure_storage_repair(ret);
		}
		if (retry > 3) {
			return -1;
		}
	}
	PHY_DBG("secure storage updata ok!\n");
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_secure_storage_test(unsigned int item)
{
	unsigned char *mbuf;
	int i, page_size, ret = -1;

	if (item >= MAX_SECURE_STORAGE_ITEM) {
		return 0;
	}

	page_size = NAND_GetPageSize();
	mbuf      = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	for (i = 0; i < MAX_SECURE_STORAGE_ITEM; i++) {
		ret = nand_secure_storage_read(i, mbuf, 2048);
		PHY_DBG("read secure storage:%d ret %d buf :0x%x,0x%x,0x%x,0x%x,\n",
			i, ret, mbuf[0], mbuf[1], mbuf[2], mbuf[3]);
	}

	MEMSET(mbuf, 0, 2048);
	mbuf[0] = 0x00 + item;
	mbuf[1] = 0x11 + item;
	mbuf[2] = 0x22 + item;
	mbuf[3] = 0x33 + item;
	nand_secure_storage_write(item, mbuf, 2048);

	for (i = 0; i < MAX_SECURE_STORAGE_ITEM; i++) {
		ret = nand_secure_storage_read(i, mbuf, 2048);
		PHY_DBG("read secure storage:%d ret %d buf :0x%x,0x%x,0x%x,0x%x,\n",
			i, ret, mbuf[0], mbuf[1], mbuf[2], mbuf[3]);
	}

	FREE(mbuf, page_size);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_secure_storage_test_read(unsigned int item, unsigned int block)
{
	unsigned char *mbuf;
	int page_size;
	int ret __attribute__((unused));


	page_size = NAND_GetPageSize();
	mbuf      = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	if (block == 0)
		block = nand_secure_storage_block;
	if (block == 1)
		block = nand_secure_storage_block_bak;

	ret = nand_secure_storage_read_one(block, item, mbuf, 2048);

	PHY_DBG("nand_secure_storage_test_read item:%d ret:%d buf:0x%x,0x%x,0x%x,0x%x,\n",
		item, ret, mbuf[0], mbuf[1], mbuf[2], mbuf[3]);

	FREE(mbuf, page_size);
	return 0;
}
