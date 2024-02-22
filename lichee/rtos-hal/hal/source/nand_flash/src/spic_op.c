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
#include "nand_common_info.h"
#include "nand_type_spinand.h"
#include "spic_op.h"
#include <sunxi_hal_spi.h>

#define NAND_DEFAULT_FREQUENCY (50)

struct hal_spi_master nand_spim;

__s32 Spic_init(__u32 spi_no)
{
	int ret;
	struct hal_spi_master *spim = &nand_spim;

	spim->port = 0;

	spim->cfg.clock_frequency = NAND_DEFAULT_FREQUENCY;
	spim->cfg.clock_frequency *= 1000 * 1000;
	spim->cfg.chipselect = 0;
	spim->cfg.mode = SPI_MODE_3;
	spim->cfg.flash = 1;
	spim->cfg.bus_sample_mode = SUNXI_SPI_SAMP_MODE_NEW;
	spim->cfg.spi_sample_mode = SUNXI_SPI_SAMP_DELAY_CYCLE_1_0;
	spim->cfg.spi_sample_delay = 0;

	ret = hal_spi_init(spim->port, &spim->cfg);
	if (ret != HAL_SPI_MASTER_OK)
		printf("init spi master failed - %d\n", ret);
	return ret;
}

__s32 Spic_exit(__u32 spi_no)
{
	//struct hal_spi_master *spim = &nand_spim;
	hal_spi_deinit(nand_spim.port);
	return 0;
}

__s32 Spic_rw_new(__u32 spi_no, __u32 tcnt, __u8 *txbuf, __u32 rcnt, __u8 *rxbuf,
		  __u32 dummy_cnt, __u32 single_cnt)
{
	int ret;
	unsigned char cmd = *(unsigned char *)txbuf;

	hal_spi_master_transfer_t tr;
	tr.tx_buf = txbuf;
	tr.tx_len = tcnt;
	tr.rx_buf = rxbuf;
	tr.rx_len = rcnt;
	tr.tx_single_len = single_cnt;
	tr.dummy_byte = dummy_cnt;

	tr.rx_nbits = tr.tx_nbits = SPI_NBITS_SINGLE;

	switch (cmd) {
	case SPI_NAND_READ_X2:
	case SPI_NAND_READ_DUAL_IO:
		tr.rx_nbits = SPI_NBITS_DUAL;
		break;
	case SPI_NAND_READ_X4:
	case SPI_NAND_READ_QUAD_IO:
		tr.rx_nbits = SPI_NBITS_QUAD;
		break;
	case SPI_NAND_PP_X4:
	case SPI_NAND_RANDOM_PP_X4:
		tr.tx_nbits = SPI_NBITS_QUAD;
		break;
	}
	ret = hal_spi_xfer(spi_no, &tr, 1);

	if (ret)
		printf("spi transfer failed %d\n", ret);
	return ret;
}

__s32 spi_nand_rdid(__u32 spi_no, __u32 chip, __u32 id_addr, __u32 addr_cnt,
		    __u32 id_cnt, void *id)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[2];
	__u32 txnum;
	__u32 rxnum;

	txnum = 1 + addr_cnt;
	rxnum = id_cnt; //mira nand:rxnum=5

	sdata[0] = SPI_NAND_RDID;
	sdata[1] = id_addr; //add 00H:Maunufation ID,01H:Device ID

	ret = Spic_rw_new(spi_no, txnum, (void *)sdata, rxnum, (void *)id, 0, txnum);
	return ret;
}
