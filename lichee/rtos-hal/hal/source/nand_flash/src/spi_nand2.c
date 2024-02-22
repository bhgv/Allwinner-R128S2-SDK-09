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

/**
 * spinand2 feature:
 *
 * 1. Complete and continuous ECC protected area
 * 2. suitable for 2 or 3 bit ecc status. (see function spi_nand_read_status)
 * 3. driver register:
 *	0: 50% (default)
 *	1: 25%
 *	2: 75%
 *	3: 100%
 * 4. if use quad mode, must enable QE
 */
#include "nand_cfg.h"
#include "nand_osal.h"
#include "nand_struct.h"
#include "nand_type_spinand.h"
#include "nand_physic_interface_spinand.h"
#include "nand_common_info.h"
#include "nand_scan.h"
#include "spic_op.h"
#include "nand_ecc_op.h"

#define STAT_MODE_READ 0
/* Just for compatibility of function PHY_Wait_Status() */
#define STAT_MODE_ERASE_WRITE 1
#define STAT_MODE_WRITE 3
#define STAT_MODE_ERASE 4

extern __u32 _cal_addr_in_chip(__u32 block, __u32 page);
extern __u32 _cal_first_valid_bit(__u32 secbitmap);
extern __u32 _cal_valid_bits(__u32 secbitmap);

__s32 m2_spi_nand_get(__u32 spi_no, __u32 chip, __u8 addr, __u8 *reg)
{
	__u8 sdata[2] = {SPI_NAND_GETSR, addr};
	__u32 txnum = 2, rxnum = 1;

	return Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0, txnum);
}

__s32 m2_spi_nand_set(__u32 spi_no, __u32 chip, __u8 addr, __u8 reg)
{
	__u8 sdata[3] = {SPI_NAND_SETSR, addr, reg};
	__u32 txnum = 3, rxnum = 0;

	return Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, txnum);
}

/* Write Enable */
__s32 m2_spi_nand_wren(__u32 spi_no)
{
	__u8 sdata = SPI_NAND_WREN;
	__u32 txnum = 1, rxnum = 0;

	return Spic_rw_new(spi_no, txnum, (void *)&sdata, rxnum, NULL, 0, txnum);
}

/* Write Disable */
__s32 m2_spi_nand_wrdi(__u32 spi_no)
{
	__u8 sdata = SPI_NAND_WRDI;
	__u32 txnum = 1, rxnum = 0;

	return Spic_rw_new(spi_no, txnum, (void *)&sdata, rxnum, NULL, 0, txnum);
}

/* get status register */
__s32 m2_spi_nand_getsr(__u32 spi_no, __u8 *reg)
{
	/*
	 * status adr:0xc0
	 * feature adr:0xb0
	 * protection adr:0xa0
	 */
	__u8 sdata[2] = {SPI_NAND_GETSR, 0xc0};
	__u32 txnum = 2, rxnum = 1;

	return Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0, txnum);
}

/**
 * wait nand finish operation
 *
 * operation: Program Execute, Page Read, Block Erase, Reset
 */
__s32 m2_spi_wait_status(__u32 spi_no, __u8 *status)
{
	__s32 timeout = 0xfffff;
	__s32 ret;

	while (timeout--) {
		ret = m2_spi_nand_getsr(spi_no, status);
		if (ret != NAND_OP_TRUE) {
			PHY_ERR("%s getsr fail!\n", __func__);
			return ret;
		}

		if (!(*(__u8 *)status & SPI_NAND_READY))
			break;

		if (timeout < 0) {
			PHY_ERR("%s timeout!\n", __func__);
			return ERR_TIMEOUT;
		}
	}
	return NAND_OP_TRUE;
}

__s32 m2_spi_nand_setstatus(__u32 spi_no, __u32 chip, __u8 reg)
{
	__s32 ret = NAND_OP_TRUE;

	ret = m2_spi_nand_wren(spi_no);
	if (ret != NAND_OP_TRUE)
		return ret;

	__u8 sdata[3] = {SPI_NAND_SETSR, 0xc0, reg};
	__u32 txnum = 3, rxnum = 0;

	return Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, txnum);
}

__s32 m2_spi_nand_getblocklock(__u32 spi_no, __u32 chip, __u8 *reg)
{
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0(control others) and 0xD0 (control driver)
	 * protection addr: 0xA0
	 */
	__u8 sdata[2] = {SPI_NAND_GETSR, 0xA0};
	__u32 txnum = 2, rxnum = 1;

	return Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0, txnum);
}

__s32 m2_spi_nand_setblocklock(__u32 spi_no, __u32 chip, __u8 reg)
{
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0(control others) and 0xD0 (control driver)
	 * protection addr: 0xA0
	 */
	__u8 sdata[3] = {SPI_NAND_SETSR, 0xA0, reg};
	__u32 txnum = 3, rxnum = 0;

	return Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, txnum);
}

__s32 m2_spi_nand_getotp(__u32 spi_no, __u32 chip, __u8 *reg)
{
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0(control others) and 0xD0 (control driver)
	 * protection addr: 0xA0
	 */
	__u8 sdata[2] = {SPI_NAND_GETSR, 0xB0};
	__u32 txnum = 2, rxnum = 1;

	return Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0, txnum);
}

__s32 m2_spi_nand_setotp(__u32 spi_no, __u32 chip, __u8 reg)
{
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0(control others) and 0xD0 (control driver)
	 * protection addr: 0xA0
	 */
	__u8 sdata[3] = {SPI_NAND_SETSR, 0xB0, reg};
	__u32 txnum = 3, rxnum = 0;

	return Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, txnum);
}

__s32 m2_spi_nand_getoutdriver(__u32 spi_no, __u32 chip, __u8 *reg)
{
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0(control others) and 0xD0 (control driver)
	 * protection addr: 0xA0
	 */
	__u8 sdata[2] = {SPI_NAND_GETSR, 0xD0};
	__u32 txnum = 2, rxnum = 1;

	return Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0, txnum);
}

__s32 m2_spi_nand_setoutdriver(__u32 spi_no, __u32 chip, __u8 reg)
{
	/*
	 * status addr: 0xC0
	 * feature addr: 0xB0 and 0xD0
	 * protection addr: 0xA0
	 */
	__u8 sdata[3] = {SPI_NAND_SETSR, 0xD0, reg};
	__u32 txnum = 3, rxnum = 0;

	return Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, txnum);
}

__s32 m2_spi_nand_read_int_eccsr(__u32 spi_no, __u8 *reg)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[2] = {SPI_NAND_READ_INT_ECCSTATUS, 0x0};
	__u32 txnum = 2, rxnum = 1;

	ret = Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, (void *)reg, 0, txnum);

	return ret;
}

static inline __s32 m2_spi_nand_check_fail_status(__u8 status, __u8 mask)
{
	if (status & mask) {
		if (mask == SPI_NAND_ERASE_FAIL)
			PHY_ERR("%s: erase fail, status = 0x%x\n",
				__func__, status);
		else
			PHY_ERR("%s: write fail, status = 0x%x\n",
				__func__, status);
		return NAND_OP_FALSE;
	}
	return NAND_OP_TRUE;
}

/**
 * check status
 *
 * @mode: 0-check ecc status  1-check operation status
 *
 * All of modes will wait status, it means waiting operation end.
 */
__s32 m2_spi_nand_read_status(__u32 spi_no, __u32 chip, __u8 status, __u32 mode)
{
	__s32 ret;
	__u8 ext_ecc = 0;

	struct __NandStorageInfo_t  *info = &NandStorageInfo;

	/* write and erase */
	if (mode) {
		ret = m2_spi_wait_status(spi_no, &status);
		if (ret != NAND_OP_TRUE)
			return ret;

		if (mode == STAT_MODE_ERASE) {
			return m2_spi_nand_check_fail_status(status,
							     (__u8)SPI_NAND_ERASE_FAIL);
		} else if (mode == STAT_MODE_WRITE) {
			return m2_spi_nand_check_fail_status(status,
							     (__u8)SPI_NAND_WRITE_FAIL);
		} else {
			ret |= m2_spi_nand_check_fail_status(status,
							     (__u8)SPI_NAND_ERASE_FAIL);
			ret |= m2_spi_nand_check_fail_status(status,
							     (__u8)SPI_NAND_WRITE_FAIL);
			return ret;
		}
		/* read (ecc status) */
	} else {
		__u32 ecc_type;

		if (info->EccType == ECC_TYPE_ERR) {
			PHY_ERR("invalid EccType on nand info\n");
			return ERR_ECC;
		}

		ret = m2_spi_wait_status(spi_no, &status);
		if (ret != NAND_OP_TRUE)
			return ret;

		if (info->EccType & HAS_EXT_ECC_STATUS) {
			/* extern ecc status should not shift */
			ret = m2_spi_nand_read_int_eccsr(spi_no, &status);
			if (ret != NAND_OP_TRUE)
				return ret;
		} else {
			status = status >> SPI_NAND_ECC_FIRST_BIT;
		}

		if (status && (info->EccType & HAS_EXT_ECC_SE01)) {
			ret = m2_spi_nand_get(spi_no, 0, 0xF0, &ext_ecc);
			if (ret != NAND_OP_TRUE)
				return ret;
			/* PHY_DBG("GD ecc status: %x %x\n", status, ext_ecc); */
			ext_ecc = ((ext_ecc >> 4) & 0x3);
			status = ((status << 2) | ext_ecc);
		}

		ecc_type = info->EccType & ECC_TYPE_BITMAP;
		return spi_nand_check_ecc(ecc_type, status);
	}
}

__s32 m2_spi_nand_block_erase(__u32 spi_no, __u32 row_addr)
{
	__u8 sdata[4] = {0};
	__s32 ret = NAND_OP_TRUE;
	__u8  status = 0;
	__u32 txnum;
	__u32 rxnum;

	N_mdelay(1);
	ret = m2_spi_nand_wren(spi_no);
	if (ret != NAND_OP_TRUE)
		return ret;

	txnum = 4;
	rxnum = 0;

	sdata[0] = SPI_NAND_BE;
	sdata[1] = (row_addr >> 16) & 0xff;
	sdata[2] = (row_addr >> 8) & 0xff;
	sdata[3] = row_addr & 0xff;

	ret = Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, txnum);
	if (ret != NAND_OP_TRUE)
		return ret;

	return m2_spi_nand_read_status(spi_no, 0, status, STAT_MODE_ERASE);
}

__s32 m2_spi_nand_reset(__u32 spi_no, __u32 chip)
{
	__u8 sdata = SPI_NAND_RESET;
	__s32 ret = NAND_OP_TRUE;
	__u32 txnum = 1, rxnum = 0;
	__u8  status = 0;

	ret = Spic_rw_new(spi_no, txnum, (void *)&sdata, rxnum, NULL, 0, txnum);
	if (ret != NAND_OP_TRUE)
		return ret;

	return m2_spi_wait_status(spi_no, &status);
}

/**
 * read data from nand
 *
 * return ecc status.
 */
__s32 m2_spi_nand_read(__u32 spi_no, __u32 page_num, __u32 mbyte_cnt,
		       __u32 sbyte_cnt, void *mbuf, void *sbuf, __u32 column)
{
	__u32 txnum, rxnum, page_addr = page_num;
	__u8  sdata[8] = {0}, status = 0;
	__s32 ret, ecc_status = 0;
	__u8 *spare_buf = PageCachePool.SpareCache1;
	struct __NandStorageInfo_t  *info = &NandStorageInfo;
	__u32 op_opt = info->OperationOpt;
	__u32 data_area_size = SECTOR_SIZE * info->SectorCntPerPage;
	__u32 spare_area_size = 64;
	__u8 op_width, op_dummy = 0;

	if (op_opt & SPINAND_ONEDUMMY_AFTER_RANDOMREAD)
		op_dummy = 1;
	if (CFG_SUPPORT_QUAD_READ && op_opt & SPINAND_QUAD_READ)
		op_width = 4;
	else if (CFG_SUPPORT_DUAL_READ && op_opt & SPINAND_DUAL_READ)
		op_width = 2;
	else
		op_width = 1;
	if (info->EccProtectedType == ECC_PROTECTED_TYPE) {
		PHY_ERR("invalid EccProtectedType on nand info\n");
		return NAND_OP_FALSE;
	}

	/* read page to nand cache */
	txnum = 4;
	rxnum = 0;
	sdata[0] = SPI_NAND_PAGE_READ;
	sdata[1] = (page_addr >> 16) & 0xff;
	sdata[2] = (page_addr >> 8) & 0xff;
	sdata[3] = page_addr & 0xff;
	ret = Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, txnum);
	if (ret != NAND_OP_TRUE)
		return ret;

	/* wait operation and get ecc status */
	ecc_status = m2_spi_nand_read_status(spi_no, 0, status, STAT_MODE_READ);

	if (mbuf) {
		/* read main data and spare data from nand cache */
		if (op_dummy) {
			txnum = 5;
			/* 1byte dummy */
			sdata[1] = 0x0;
			sdata[2] = ((column >> 8) & 0xff);
			sdata[3] = column & 0xff;
			sdata[4] = 0x0;
		} else {
			txnum = 4;
			sdata[1] = ((column >> 8) & 0xff);
			if (op_opt & SPINAND_TWO_PLANE_SELECT) {
				if (page_num & info->PageCntPerPhyBlk)
					sdata[1] = ((column >> 8) & 0x0f) | 0x10;
				else
					sdata[1] = ((column >> 8) & 0x0f);
			}
			sdata[2] = column & 0xff;
			sdata[3] = 0x0;
		}
		rxnum = sbuf ? mbyte_cnt + spare_area_size : mbyte_cnt;
		if (op_width == 4) {
			sdata[0] = SPI_NAND_READ_X4;
		} else if (op_width == 2) {
			sdata[0] = SPI_NAND_READ_X2;
		} else {
			sdata[0] = SPI_NAND_FAST_READ_X1;
		}

		ret = Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, mbuf, 0, txnum);
		if (ret != NAND_OP_TRUE)
			goto err;

		if (sbuf) {
			ret = spi_nand_copy_from_spare(info->EccProtectedType,
						       sbuf, mbuf + data_area_size, sbyte_cnt);
			if (ret != NAND_OP_TRUE)
				goto err;
			/* invalid data */
			if (((__u8 *)mbuf)[data_area_size] != 0xff ||
			    (((__u8 *)sbuf)[0] != 0xff))
				((__u8 *)sbuf)[0] = 0x0;
		}
	} else if (sbuf) {
		/* read spare data only */
		if (op_dummy) {
			txnum = 5;
			/* 1byte dummy */
			sdata[1] = 0x0;
			sdata[2] = ((data_area_size >> 8) & 0xff);
			sdata[3] = data_area_size & 0xff;
			sdata[4] = 0x0;
		} else {
			txnum = 4;
			sdata[1] = ((data_area_size >> 8) & 0xff);
			sdata[2] = data_area_size & 0xff;
			sdata[3] = 0x0;
		}
		rxnum = spare_area_size;
		if (op_width == 4) {
			sdata[0] = SPI_NAND_READ_X4;
		} else if (op_width == 2) {
			sdata[0] = SPI_NAND_READ_X2;
		} else {
			sdata[0] = SPI_NAND_FAST_READ_X1;
		}

		ret = Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, spare_buf, 0, txnum);
		if (ret != NAND_OP_TRUE)
			goto err;

		ret = spi_nand_copy_from_spare(info->EccProtectedType, sbuf,
					       spare_buf, sbyte_cnt);
		if (ret != NAND_OP_TRUE)
			goto err;
		/* invalid data */
		if ((spare_buf[0] != 0xff) || (((__u8 *)sbuf)[0] != 0xff))
			((__u8 *)sbuf)[0] = 0x0;
	}

	ret = ecc_status;
err:
	return ret;
}

__s32 m2_spi_nand_write(__u32 spi_no, __u32 page_addr, __u32 mbyte_cnt,
			__u32 sbyte_cnt, void *mbuf, void *sbuf, __u32 column)
{
	__u32 txnum, rxnum;
	__u8 *sdata, status = 0;
	__s32 ret;
	struct __NandStorageInfo_t *info = &NandStorageInfo;
	__u32 op_opt = info->OperationOpt;
	__u32 data_area_size = SECTOR_SIZE * info->SectorCntPerPage;
	__u32 spare_area_size = 64;
	__u8 op_width;

	sdata = PageCachePool.SpiPageCache;
	MEMSET((void *)sdata, 0xFF, data_area_size + spare_area_size + 64);

	if ((CFG_SUPPORT_QUAD_READ && op_opt & SPINAND_QUAD_PROGRAM))
		op_width = 4;
	else
		op_width = 1;
	if (info->EccProtectedType == ECC_PROTECTED_TYPE) {
		PHY_ERR("invalid EccProtectedType on nand info\n");
		return NAND_OP_FALSE;
	}

	/* write enable */
	ret = m2_spi_nand_wren(spi_no);
	if (ret != NAND_OP_TRUE)
		return ret;

	if (0xef != info->NandChipId[0]) {
		/* cmd(1B) + addr(2B) + data(2048B) + spare */
		txnum = 3 + data_area_size + spare_area_size;
		rxnum = 0;
		sdata[0] = op_width == 4 ? SPI_NAND_PP_X4 : SPI_NAND_PP;
		sdata[1] = (column >> 8) & 0xff;
		if (op_opt & SPINAND_TWO_PLANE_SELECT) {
			if (page_addr & info->PageCntPerPhyBlk)
				sdata[1] = ((column >> 8) & 0x0f) | 0x10;
			else
				sdata[1] = ((column >> 8) & 0x0f);
		}
		sdata[2] = column & 0xff;
		/* copy data */
		MEMCPY((void *)(sdata + 3), mbuf, mbyte_cnt);
		/* copy spare data */
		ret = spi_nand_copy_to_spare(info->EccProtectedType,
					     sdata + 3 + data_area_size, sbuf, sbyte_cnt);
		if (ret != NAND_OP_TRUE)
			return ret;

		ret = Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, 3);
		if (ret != NAND_OP_TRUE)
			return ret;
	} else {
		/* Special case for Winbond 4x100 bug: there may be an
		 * unexpected 0xFF @ 0x400 or 0x401 offset address of
		 * physical page*/
		/* step 1, program half page */
		/* cmd(1B) + addr(2B) + data((data_area_size>>1)) */
		txnum = 3 + (data_area_size >> 1);
		rxnum = 0;
		sdata[0] = op_width == 4 ? SPI_NAND_PP_X4 : SPI_NAND_PP;
		sdata[1] = (column >> 8) & 0xff;
		if (op_opt & SPINAND_TWO_PLANE_SELECT) {
			if (page_addr & info->PageCntPerPhyBlk)
				sdata[1] = ((column >> 8) & 0x0f) | 0x10;
			else
				sdata[1] = ((column >> 8) & 0x0f);
		}
		sdata[2] = column & 0xff;
		/* copy data */
		MEMCPY((void *)(sdata + 3), mbuf, (data_area_size >> 1));

		ret = Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, 3);
		if (ret != NAND_OP_TRUE)
			return ret;

		/* step 2, random_program another half page */
		/* cmd(1B) + addr(2B) + data((data_area_size>>1)) + spare(64B) */
		txnum = 3 + (data_area_size >> 1) + spare_area_size;
		rxnum = 0;

		MEMSET((void *)sdata, 0xff, txnum);

		sdata[0] = SPI_NAND_RANDOM_PP_X4;
		// column address
		sdata[1] = ((data_area_size >> 1) >> 8) & 0xff;  // 4bit dummy,12bit column adr
		sdata[2] = (__u8)((data_area_size >> 1) & 0xff); // A7:A0

		MEMCPY((void *)(sdata + 3), mbuf + (data_area_size >> 1), (data_area_size >> 1));
		/* copy spare data */
		ret = spi_nand_copy_to_spare(info->EccProtectedType,
					     sdata + 3 + (data_area_size >> 1), sbuf, sbyte_cnt);
		if (ret != NAND_OP_TRUE)
			return ret;

		ret = Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, 3);
		if (ret != NAND_OP_TRUE)
			return ret;
	}

	txnum = 4;
	rxnum = 0;
	sdata[0] = SPI_NAND_PE;
	sdata[1] = (page_addr >> 16) & 0xff;
	sdata[2] = (page_addr >> 8) & 0xff;
	sdata[3] = page_addr & 0xff;

	ret = Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, NULL, 0, txnum);
	if (ret != NAND_OP_TRUE)
		return ret;

	/* Wait End and return result */
	return m2_spi_nand_read_status(spi_no, 0, status, STAT_MODE_WRITE);
}

__s32 m2_read_single_page(struct boot_physical_param *readop,
			  __u32 spare_only_flag)
{
	__s32 ret = NAND_OP_TRUE;
	__u32 addr;
	__u32 first_sector;
	__u32 sector_num;
	__u8 *data_buf;

	data_buf = PageCachePool.SpiPageCache;

	addr = _cal_addr_in_chip(readop->block, readop->page);

	first_sector = _cal_first_valid_bit(readop->sectorbitmap);
	sector_num = _cal_valid_bits(readop->sectorbitmap);

	if (spare_only_flag)
		ret = m2_spi_nand_read(0, addr, 0, 16, NULL, readop->oobbuf, 0);
	else
		ret = m2_spi_nand_read(0, addr, 512 * sector_num, 16, data_buf,
				       readop->oobbuf, 512 * first_sector);

	if (spare_only_flag == 0)
		MEMCPY((__u8 *)readop->mainbuf + 512 * first_sector, data_buf,
		       512 * sector_num);

	return ret;
}

__s32 m2_write_single_page(struct boot_physical_param *writeop)
{
	__u8  sparebuf[32];
	__u32 addr;

	MEMSET(sparebuf, 0xff, 32);
	/* we just use 16 bytes spare area */
	if (writeop->oobbuf)
		MEMCPY(sparebuf, writeop->oobbuf, 16);

	addr = _cal_addr_in_chip(writeop->block, writeop->page);

	return m2_spi_nand_write(0, addr,
				 512 * NandStorageInfo.SectorCntPerPage, 16,
				 writeop->mainbuf, sparebuf, 0);
}

__s32 m2_erase_single_block(struct boot_physical_param *eraseop)
{
	__u32 addr;

	addr = _cal_addr_in_chip(eraseop->block, 0);
	return m2_spi_nand_block_erase(0, addr);
}
