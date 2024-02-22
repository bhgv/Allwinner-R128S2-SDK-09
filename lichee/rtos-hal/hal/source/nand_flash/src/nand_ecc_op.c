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

#include "nand_osal.h"
#include "nand_common_info.h"
#include "nand_ecc_op.h"

#define DUMP_SPARE 0
#define DUMP_PRINT printk

#if DUMP_SPARE
void hexdump(const char *buf, int len)
{
	int i;
	for (i = 0; i < len;) {
		DUMP_PRINT("0x%02x ", (int)(buf[i]));
		if (++i % 16 == 0)
			DUMP_PRINT("\n");
	}
	DUMP_PRINT("\n");
}
#endif

static inline __s32 general_check_ecc(__u8 ecc, __u8 limit_from, __u8 limit_to,
				      __u8 err_from, u8 err_to)
{
	if (ecc < limit_from) {
		return NAND_OP_TRUE;
	} else if (ecc >= limit_from && ecc <= limit_to) {
		PHY_DBG("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else if (ecc >= err_from && ecc <= err_to) {
		PHY_ERR("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	} else {
		PHY_ERR("unknown ecc value 0x%x\n", ecc);
		return ERR_ECC;
	}
}

static __s32 check_ecc_bit2_limit1_err2_limit3(__u8 ecc)
{
	if (ecc == 0) {
		return NAND_OP_TRUE;
	} else if (ecc == 1 || ecc == 3) {
		PHY_DBG("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		PHY_ERR("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}

static __s32 check_ecc_bit4_limit5_7_err8(__u8 ecc)
{
	if (ecc <=4) {
		return NAND_OP_TRUE;
	} else if ((ecc>=5) && (ecc<=7)) {
		PHY_DBG("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		PHY_DBG("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}

static __s32 check_ecc_bit3_limit5_err2(__u8 ecc)
{
	if (ecc <= 1) {
		return NAND_OP_TRUE;
	} else if (ecc == 3 || ecc == 5) {
		PHY_DBG("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		PHY_ERR("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}
static __s32 check_ecc_bit4_limit5_7_err8_limit12(__u8 ecc)
{
	if (ecc <= 4) {
		return NAND_OP_TRUE;
	} else if (((ecc >= 5) && (ecc <= 7)) || (ecc >= 12)) {
		PHY_DBG("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		PHY_DBG("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}

static __s32 check_ecc_bit2_limit3_err2(__u8 ecc)
{
	if (ecc <= 1) {
		return NAND_OP_TRUE;
	} else if (ecc == 3) {
		PHY_DBG("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		PHY_DBG("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}

__s32 spi_nand_check_ecc(enum ecc_type type, __u8 status)
{
	__u8 ecc;

	switch (type) {
	case BIT3_LIMIT3_TO_6_ERR7:
		ecc = status & 0x07;
		return general_check_ecc(ecc, 3, 6, 7, 7);
	case BIT2_LIMIT1_ERR2:
		ecc = status & 0x03;
		return general_check_ecc(ecc, 1, 1, 2, 2);
	case BIT2_LIMIT1_ERR2_LIMIT3:
		ecc = status & 0x03;
		return check_ecc_bit2_limit1_err2_limit3(ecc);
	case BIT4_LIMIT3_TO_4_ERR15:
		ecc = status & 0x0f;
		return general_check_ecc(ecc, 3, 4, 15, 15);
	case BIT3_LIMIT3_TO_4_ERR7:
		ecc = status & 0x07;
		return general_check_ecc(ecc, 3, 4, 7, 7);
	case BIT3_LIMIT5_ERR2:
		ecc = status & 0x07;
		return check_ecc_bit3_limit5_err2(ecc);
	case BIT4_LIMIT5_TO_7_ERR8_LIMIT_12:
		ecc = status & 0x0f;
		return check_ecc_bit4_limit5_7_err8_limit12(ecc);
	case BIT2_LIMIT3_ERR2:
		ecc = status & 0x03;
		return check_ecc_bit2_limit3_err2(ecc);
	case BIT4_LIMIT5_TO_7_ERR8:
		ecc = status & 0x03;
		return check_ecc_bit4_limit5_7_err8(ecc);
	case BIT4_LIMIT4_TO_8_ECC15:
		ecc = status & 0x0f;
		return general_check_ecc(ecc, 4, 8, 9, 15);
	default:
		return NAND_OP_FALSE;
	}
}

static __s32 __copy_to_spare(__u8 *to, __u8 *from, __u32 cnt, __u32 spare_size,
			     __u32 offset, __u32 datalen)
{
	__u32 i;
#if DUMP_SPARE
	__u32 len = cnt;
#endif

	/* bad block mark byte */
	to[0] = from[0];

	for (i = 0; cnt > 0; i++) {
		/*
		 * only the last time, num maybe not datalen, that's why
		 * memcpy offset add datalen rather than num
		 */
		__u32 num = cnt > datalen ? datalen : cnt;
		MEMCPY(to + offset + spare_size * i, from + datalen * i, num);
		cnt -= num;
	}
#if DUMP_SPARE
	DUMP_PRINT("Write: from: ram buf\n");
	hexdump((const char *)from, len);
	DUMP_PRINT("Write: to: phy spare\n");
	hexdump((const char *)to, 64);
#endif
	return NAND_OP_TRUE;
}

static __s32 __copy_from_spare(__u8 *to, __u8 *from, __u32 cnt, __u32 spare_size,
			       __u32 offset, __u32 datalen)
{
	__u32 i;
#if DUMP_SPARE
	__u32 len = cnt;
#endif

	for (i = 0; cnt > 0; i++) {
		/*
		 * only the last time, num maybe not datalen, that's why
		 * memcpy offset add datalen rather than num
		 */
		__u32 num = cnt > datalen ? datalen : cnt;
		MEMCPY(to + datalen * i, from + offset + spare_size * i, num);
		cnt -= num;
	}
#if DUMP_SPARE
	DUMP_PRINT("Read: from: phy spare\n");
	hexdump((const char *)from, 64);
	DUMP_PRINT("Read: to: ram buf\n");
	hexdump((const char *)to, len);
#endif
	return NAND_OP_TRUE;
}

__s32 spi_nand_copy_to_spare(enum ecc_protected_type type, __u8 *to, __u8 *from, __u32 cnt)
{
	switch (type) {
	case SIZE16_OFF0_LEN16:
			return __copy_to_spare(to, from, cnt, 16, 0, 16);
	case SIZE16_OFF4_LEN12:
		return __copy_to_spare(to, from, cnt, 16, 4, 12);
	case SIZE16_OFF4_LEN4_OFF8:
		return __copy_to_spare(to, from, cnt, 16, 4, 4);
	case SIZE16_OFF4_LEN8_OFF4:
		return __copy_to_spare(to, from, cnt, 16, 4, 8);
	case SIZE16_OFF32_LEN16:
		return __copy_to_spare(to, from, cnt, 16, 32, 16);
	case SIZE64_OFF8_LEN40_OFF16:
		return __copy_to_spare(to, from, cnt, 64, 8, 40);
	default:
		return NAND_OP_FALSE;
	}
}

__s32 spi_nand_copy_from_spare(enum ecc_protected_type type, __u8 *to, __u8 *from, __u32 cnt)
{
	switch (type) {
	case SIZE16_OFF0_LEN16:
			return __copy_from_spare(to, from, cnt, 16, 0, 16);
	case SIZE16_OFF4_LEN12:
		return __copy_from_spare(to, from, cnt, 16, 4, 12);
	case SIZE16_OFF4_LEN4_OFF8:
		return __copy_from_spare(to, from, cnt, 16, 4, 4);
	case SIZE16_OFF4_LEN8_OFF4:
		return __copy_from_spare(to, from, cnt, 16, 4, 8);
	case SIZE16_OFF32_LEN16:
		return __copy_from_spare(to, from, cnt, 16, 32, 16);
	case SIZE64_OFF8_LEN40_OFF16:
		return __copy_from_spare(to, from, cnt, 64, 8, 40);
	default:
		return NAND_OP_FALSE;
	}
}
