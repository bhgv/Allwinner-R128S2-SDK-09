/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY?ˉS TECHNOLOGY (SONY, DTS, DOLBY, AVS OR
 * MPEGLA, ETC.)
 * IN ALLWINNERS?ˉSDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT
 * TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY?ˉS TECHNOLOGY.
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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <interrupt.h>
#include <sunxi_hal_spi.h>
#include "platform_spi.h"
#include <hal_cache.h>
#include <hal_mem.h>
#include <hal_osal.h>
#include <hal_log.h>
#include <script.h>
#ifdef CONFIG_DRIVER_SYSCONFIG
#include <hal_cfg.h>
#endif
#include <sunxi_hal_common.h>

// #define SPI_DATA_LEVEL
// #define SPI_DUMPREG_LEVEL

#define SPI_INFO(sspi, fmt, arg...)	hal_log_info("hal-sspi %08x.sspi%d: " fmt, sspi->base, sspi->bus_num, ##arg)
#define SPI_WARN(sspi, fmt, arg...)	hal_log_warn("hal-sspi %08x.sspi%d: " fmt, sspi->base, sspi->bus_num, ##arg)
#define SPI_ERR(sspi, fmt, arg...)	hal_log_err("hal-sspi %08x.sspi%d: " fmt, sspi->base, sspi->bus_num, ##arg)
#define SPI_DBG(sspi, fmt, arg...)	hal_log_debug("hal-sspi %08x.sspi%d: " fmt, sspi->base, sspi->bus_num, ##arg)

#define XFER_TIMEOUT	(500)
#define POLL_TIMEOUT	(0x1ffffff)

static const char *sunxi_spi_mode[] = {
	"SINGLE_HALF_DUPLEX_RX",	/* single mode, half duplex read */
	"SINGLE_HALF_DUPLEX_TX",	/* single mode, half duplex write */
	"SINGLE_FULL_DUPLEX_RX_TX",	/* single mode, full duplex read and write */
	"DUAL_HALF_DUPLEX_RX",		/* dual mode, half duplex read */
	"DUAL_HALF_DUPLEX_TX",		/* dual mode, half duplex write */
	"QUAD_HALF_DUPLEX_RX",		/* quad mode, half duplex read */
	"QUAD_HALF_DUPLEX_TX",		/* quad mode, half duplex write */
	"FULL_DUPLEX_TX_RX",		/* full duplex read and write */
	"MODE_TYPE_NULL",
};

static const unsigned int sunxi_spi_sample_mode[] = {
	0x100, /* SUNXI_SPI_SAMP_DELAY_CYCLE_0_0 */
	0x000, /* SUNXI_SPI_SAMP_DELAY_CYCLE_0_5 */
	0x010, /* SUNXI_SPI_SAMP_DELAY_CYCLE_1_0 */
	0x110, /* SUNXI_SPI_SAMP_DELAY_CYCLE_1_5 */
	0x101, /* SUNXI_SPI_SAMP_DELAY_CYCLE_2_0 */
	0x001, /* SUNXI_SPI_SAMP_DELAY_CYCLE_2_5 */
	0x011  /* SUNXI_SPI_SAMP_DELAY_CYCLE_3_0 */
};

static sunxi_spi_t g_sunxi_spi[SPI_MAX_NUM];
extern struct sunxi_spi_params_t g_sunxi_spi_params[SPI_MAX_NUM];
extern struct sunxi_spi_params_t g_sunxi_spi_params_sip[SPI_SIP_MAX_NUM];

static sunxi_spi_t *sunxi_get_spi(int port)
{
	sunxi_spi_t *sspi = NULL;

	if (port >= 0 && port < SPI_MAX_NUM)
		sspi = &g_sunxi_spi[port];

	return sspi;
}

static void sunxi_spi_dump_reg(sunxi_spi_t *sspi, uint32_t offset, uint32_t len)
{
	uint32_t i;
	uint8_t buf[64], cnt = 0;

	SPI_INFO(sspi, "dump reg:");
	for (i = 0; i < len; i = i + REG_INTERVAL)
	{
		if (i % HEXADECIMAL == 0)
			cnt += sprintf((char *)(buf + cnt), "0x%08lx: ",
						(unsigned long)(sspi->base + offset + i));

		cnt += sprintf((char *)(buf + cnt), "%08lx ",
			(unsigned long)hal_readl(sspi->base + offset + i));

		if (i % HEXADECIMAL == REG_CL)
		{
			SPI_INFO(sspi, "%s", buf);
			cnt = 0;
		}
	}
}

static void sunxi_spi_dump_data(sunxi_spi_t *sspi, const uint8_t *buf, uint32_t len)
{
	uint32_t i, cnt = 0;
	uint8_t *tmp = NULL;

	tmp = hal_malloc(len);
	if (!tmp)
		return;

	SPI_INFO(sspi, "dump data len %d:", len);
	for (i = 0; i < len; i++)
	{
		if (i % HEXADECIMAL == 0)
			cnt += sprintf(tmp + cnt, "0x%08x: ", i);

		cnt += sprintf(tmp + cnt, "%02x ", buf[i]);

		if ((i % HEXADECIMAL == REG_CL) || (i == (len - 1))) {
			SPI_INFO(sspi, "%s", tmp);
			cnt = 0;
		}
	}
}

/* SPI Controller Hardware Register Operation Start */

static void sunxi_spi_soft_reset(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_GC_REG);

	reg_val |= SUNXI_SPI_GC_SRST;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_GC_REG);

	/* Hardware will auto clear this bit when soft reset
	 * Before return, driver must wait reset opertion complete */
	SPI_DBG(sspi, "wait for soft reset\n");
	while (hal_readl(sspi->base + SUNXI_SPI_GC_REG) & SUNXI_SPI_GC_SRST)
		;
	SPI_DBG(sspi, "soft reset completed\n");
}

static void sunxi_spi_enable_tp(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_GC_REG);

	reg_val |= SUNXI_SPI_GC_TP_EN;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_GC_REG);
}

static void sunxi_spi_disable_tp(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_GC_REG);

	reg_val &= ~SUNXI_SPI_GC_TP_EN;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_GC_REG);
}

static void sunxi_spi_bus_sample_mode(sunxi_spi_t *sspi, hal_spi_master_bus_sample_mode_t mode)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_GC_REG);

	switch (mode) {
	case SUNXI_SPI_SAMP_MODE_OLD:
		reg_val &= ~SUNXI_SPI_GC_MODE_SEL;
		break;
	case SUNXI_SPI_SAMP_MODE_NEW:
		reg_val |= SUNXI_SPI_GC_MODE_SEL;
		break;
	}

	hal_writel(reg_val, sspi->base + SUNXI_SPI_GC_REG);
}

static void sunxi_spi_set_master(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_GC_REG);

	reg_val |= SUNXI_SPI_GC_MODE;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_GC_REG);
}

static void sunxi_spi_set_slave(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_GC_REG);

	reg_val &= ~SUNXI_SPI_GC_MODE;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_GC_REG);
}

static void sunxi_spi_enable_bus(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_GC_REG);

	reg_val |= SUNXI_SPI_GC_EN;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_GC_REG);
}

static void sunxi_spi_disable_bus(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_GC_REG);

	reg_val &= ~SUNXI_SPI_GC_EN;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_GC_REG);
}

static void sunxi_spi_start_xfer(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_TC_REG);

	reg_val |= SUNXI_SPI_TC_XCH;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_TC_REG);
}

static void sunxi_spi_set_sample_mode(sunxi_spi_t *sspi, hal_spi_master_spi_sample_mode_t mode)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_TC_REG);
	uint32_t org_val = reg_val;
	uint32_t sdm, sdc, sdc1;

	sdm = (sunxi_spi_sample_mode[mode] >> 8) & 0xf;
	sdc = (sunxi_spi_sample_mode[mode] >> 4) & 0xf;
	sdc1 = (sunxi_spi_sample_mode[mode] >> 0) & 0xf;

	if (sdm)
		reg_val |= SUNXI_SPI_TC_SDM;
	else
		reg_val &= ~SUNXI_SPI_TC_SDM;

	if (sdc)
		reg_val |= SUNXI_SPI_TC_SDC;
	else
		reg_val &= ~SUNXI_SPI_TC_SDC;

	if (sdc1)
		reg_val |= SUNXI_SPI_TC_SDC1;
	else
		reg_val &= ~SUNXI_SPI_TC_SDC1;

	if (reg_val != org_val)
		hal_writel(reg_val, sspi->base + SUNXI_SPI_TC_REG);
}

static void sunxi_spi_set_dummy_type(sunxi_spi_t *sspi, bool ddb)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_TC_REG);

	if (ddb)
		reg_val |= SUNXI_SPI_TC_DDB;
	else
		reg_val &= ~SUNXI_SPI_TC_DDB;

	hal_writel(reg_val, sspi->base + SUNXI_SPI_TC_REG);
}

static void sunxi_spi_set_discard_burst(sunxi_spi_t *sspi, bool dhb)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_TC_REG);

	if (dhb)
		reg_val |= SUNXI_SPI_TC_DHB;
	else
		reg_val &= ~SUNXI_SPI_TC_DHB;

	hal_writel(reg_val, sspi->base + SUNXI_SPI_TC_REG);
}

static void sunxi_spi_ss_level(sunxi_spi_t *sspi, bool status)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_TC_REG);

	if (status)
		reg_val |= SUNXI_SPI_TC_SS_LEVEL;
	else
		reg_val &= ~SUNXI_SPI_TC_SS_LEVEL;

	hal_writel(reg_val, sspi->base + SUNXI_SPI_TC_REG);
}

static void sunxi_spi_ss_owner(sunxi_spi_t *sspi, uint32_t owner)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_TC_REG);

	switch (owner) {
	case HAL_SPI_CS_AUTO:
		reg_val &= ~SUNXI_SPI_TC_SS_OWNER;
		break;
	case HAL_SPI_CS_SOFT:
		reg_val |= SUNXI_SPI_TC_SS_OWNER;
		break;
	}

	hal_writel(reg_val, sspi->base + SUNXI_SPI_TC_REG);
}

static hal_spi_master_status_t sunxi_spi_ss_select(sunxi_spi_t *sspi, uint8_t chipselect)
{
	hal_spi_master_status_t ret = 0;
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_TC_REG);

	if (chipselect < HAL_SPI_MASTER_MAX) {
		reg_val &= ~SUNXI_SPI_TC_SS_SEL;
		reg_val |= (chipselect << SUNXI_SPI_TC_SS_SEL_POS);
		hal_writel(reg_val, sspi->base + SUNXI_SPI_TC_REG);
		ret = HAL_SPI_MASTER_OK;
	} else {
		ret = HAL_SPI_MASTER_ERROR;
	}

	return ret;
}

static void sunxi_spi_ss_ctrl(sunxi_spi_t *sspi, bool ssctl)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_TC_REG);

	if (ssctl)
		reg_val |= SUNXI_SPI_TC_SSCTL;
	else
		reg_val &= ~SUNXI_SPI_TC_SSCTL;

	hal_writel(reg_val, sspi->base + SUNXI_SPI_TC_REG);
}

static void sunxi_spi_ss_polarity(sunxi_spi_t *sspi, bool pol)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_TC_REG);

	if (pol)
		reg_val |= SUNXI_SPI_TC_SPOL; /* default SSPOL = 1,Low level effect */
	else
		reg_val &= ~SUNXI_SPI_TC_SPOL;

	hal_writel(reg_val, sspi->base + SUNXI_SPI_TC_REG);
}

static void sunxi_spi_config_tc(sunxi_spi_t *sspi, uint32_t config)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_TC_REG);

	/* POL */
	if (config & SPI_CPOL)
		reg_val |= SUNXI_SPI_TC_CPOL;
	else
		reg_val &= ~SUNXI_SPI_TC_CPOL;

	/* PHA */
	if (config & SPI_CPHA)
		reg_val |= SUNXI_SPI_TC_CPHA;
	else
		reg_val &= ~SUNXI_SPI_TC_CPHA;

	/* LMTF--LSB/MSB transfer first select */
	if (config & SPI_LSB_FIRST)
		reg_val |= SUNXI_SPI_TC_FBS;
	else
		reg_val &= ~SUNXI_SPI_TC_FBS; /* default LMTF =0, MSB first */

	hal_writel(reg_val, sspi->base + SUNXI_SPI_TC_REG);
}

static void sunxi_spi_enable_irq(sunxi_spi_t *sspi, uint32_t bitmap)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_INT_CTL_REG);

	bitmap &= SUNXI_SPI_INT_CTL_MASK;
	reg_val |= bitmap;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_INT_CTL_REG);
}

static void sunxi_spi_disable_irq(sunxi_spi_t *sspi, uint32_t bitmap)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_INT_CTL_REG);

	bitmap &= SUNXI_SPI_INT_CTL_MASK;
	reg_val &= ~bitmap;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_INT_CTL_REG);
}

static uint32_t sunxi_spi_qry_irq_enable(sunxi_spi_t *sspi)
{
	return (SUNXI_SPI_INT_CTL_MASK & hal_readl(sspi->base + SUNXI_SPI_INT_CTL_REG));
}

static uint32_t sunxi_spi_qry_irq_pending(sunxi_spi_t *sspi)
{
	return (SUNXI_SPI_INT_STA_MASK & hal_readl(sspi->base + SUNXI_SPI_INT_STA_REG));
}

static void sunxi_spi_clr_irq_pending(sunxi_spi_t *sspi, uint32_t pending_bit)
{
	pending_bit &= SUNXI_SPI_INT_STA_MASK;
	hal_writel(pending_bit, sspi->base + SUNXI_SPI_INT_STA_REG);
}

static void sunxi_spi_reset_fifo(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_FIFO_CTL_REG);
	int ret;

	reg_val |= SUNXI_SPI_FIFO_CTL_RST;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_FIFO_CTL_REG);

	/* Hardware will auto clear this bit when fifo reset
	 * Before return, driver must wait reset opertion complete
	 */
	SPI_DBG(sspi, "wait for fifo reset\n");
	while (hal_readl(sspi->base + SUNXI_SPI_FIFO_CTL_REG) & SUNXI_SPI_FIFO_CTL_RST)
		;
	SPI_DBG(sspi, "fifo reset completed\n");
}

static void sunxi_spi_enable_dma_irq(sunxi_spi_t *sspi, uint32_t bitmap)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_FIFO_CTL_REG);

	bitmap &= SUNXI_SPI_FIFO_CTL_DRQ_EN;
	reg_val |= bitmap;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_FIFO_CTL_REG);
}

static void sunxi_spi_disable_dma_irq(sunxi_spi_t *sspi, uint32_t bitmap)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_FIFO_CTL_REG);

	bitmap &= SUNXI_SPI_FIFO_CTL_DRQ_EN;
	reg_val &= ~bitmap;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_FIFO_CTL_REG);
}

static void sunxi_spi_set_fifo_trig_level_rx(sunxi_spi_t *sspi, uint32_t level)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_FIFO_CTL_REG);

	level &= (SUNXI_SPI_FIFO_CTL_RX_TRIG_LEVEL >> SUNXI_SPI_FIFO_CTL_RX_TRIG_LEVEL_POS);
	reg_val &= ~SUNXI_SPI_FIFO_CTL_RX_TRIG_LEVEL;
	reg_val |= (level << SUNXI_SPI_FIFO_CTL_RX_TRIG_LEVEL_POS);

	hal_writel(reg_val, sspi->base + SUNXI_SPI_FIFO_CTL_REG);
}

static void sunxi_spi_set_fifo_trig_level_tx(sunxi_spi_t *sspi, uint32_t level)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_FIFO_CTL_REG);

	level &= (SUNXI_SPI_FIFO_CTL_TX_TRIG_LEVEL >> SUNXI_SPI_FIFO_CTL_TX_TRIG_LEVEL_POS);
	reg_val &= ~SUNXI_SPI_FIFO_CTL_TX_TRIG_LEVEL;
	reg_val |= (level << SUNXI_SPI_FIFO_CTL_TX_TRIG_LEVEL_POS);

	hal_writel(reg_val, sspi->base + SUNXI_SPI_FIFO_CTL_REG);
}

static uint32_t sunxi_spi_query_txfifo(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_FIFO_STA_REG) & SUNXI_SPI_FIFO_STA_TX_CNT;

	return (reg_val >> SUNXI_SPI_FIFO_STA_TX_CNT_POS);
}

static uint32_t sunxi_spi_query_rxfifo(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_FIFO_STA_REG) & SUNXI_SPI_FIFO_STA_RX_CNT;

	return (reg_val >> SUNXI_SPI_FIFO_STA_RX_CNT_POS);
}

static void sunxi_spi_set_sample_delay_sw(sunxi_spi_t *sspi, uint32_t status)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_SAMP_DL_REG);

	if (status)
		reg_val |= SUNXI_SPI_SAMP_DL_SW_EN;
	else
		reg_val &= ~SUNXI_SPI_SAMP_DL_SW_EN;

	hal_writel(reg_val, sspi->base + SUNXI_SPI_SAMP_DL_REG);
}

static void sunxi_spi_set_sample_delay(sunxi_spi_t *sspi, uint32_t sample_delay)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_SAMP_DL_REG);

	reg_val &= ~SUNXI_SPI_SAMP_DL_SW;
	reg_val |= (sample_delay & SUNXI_SPI_SAMP_DL_SW);
	hal_writel(reg_val, sspi->base + SUNXI_SPI_SAMP_DL_REG);
}

static void sunxi_spi_set_bc_tc_stc(sunxi_spi_t *sspi, uint32_t tx_len, uint32_t rx_len, uint32_t stc_len, uint32_t dummy_cnt)
{
	uint32_t reg_val;

	/* set MBC(0x30) = tx_len + rx_len + dummy_cnt */
	reg_val = hal_readl(sspi->base + SUNXI_SPI_MBC_REG);
	reg_val &= ~SUNXI_SPI_MBC;
	reg_val |= ((tx_len + rx_len + dummy_cnt) & SUNXI_SPI_MBC);
	hal_writel(reg_val, sspi->base + SUNXI_SPI_MBC_REG);

	/* set MTC(0x34) = tx_len */
	reg_val = hal_readl(sspi->base + SUNXI_SPI_MTC_REG);
	reg_val &= ~SUNXI_SPI_MWTC;
	reg_val |= (tx_len & SUNXI_SPI_MWTC);
	hal_writel(reg_val, sspi->base + SUNXI_SPI_MTC_REG);

	/* set BBC(0x38) = dummy cnt & single mode transmit counter */
	reg_val = hal_readl(sspi->base + SUNXI_SPI_BCC_REG);
	reg_val &= ~SUNXI_SPI_BCC_STC;
	reg_val |= (stc_len & SUNXI_SPI_MWTC);
	reg_val &= ~SUNXI_SPI_BCC_DBC;
	reg_val |= (dummy_cnt & (SUNXI_SPI_BCC_DBC >> SUNXI_SPI_BCC_DBC_POS)) << SUNXI_SPI_BCC_DBC_POS;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BCC_REG);
}

static void sunxi_spi_enable_dual(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BCC_REG);

	reg_val |= SUNXI_SPI_BCC_DRM;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BCC_REG);
}

static void sunxi_spi_disable_dual(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BCC_REG);

	reg_val &= ~SUNXI_SPI_BCC_DRM;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BCC_REG);
}

static void sunxi_spi_enable_quad(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BCC_REG);

	reg_val |= SUNXI_SPI_BCC_QUAD_EN;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BCC_REG);
}

static void sunxi_spi_disable_quad(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BCC_REG);

	reg_val &= ~SUNXI_SPI_BCC_QUAD_EN;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BCC_REG);
}

void sunxi_spi_bit_start_xfer(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	reg_val |= SUNXI_SPI_BATC_TCE;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
}

static void sunxi_spi_bit_sample_mode(sunxi_spi_t *sspi, uint8_t mode)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	if (mode)
		reg_val |= SUNXI_SPI_BATC_MSMS;
	else
		reg_val &= ~SUNXI_SPI_BATC_MSMS;

	hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
}

uint32_t sunxi_spi_bit_qry_irq_pending(sunxi_spi_t *sspi)
{
	return (SUNXI_SPI_BATC_TBC & hal_readl(sspi->base + SUNXI_SPI_BATC_REG));
}

void sunxi_spi_bit_clr_irq_pending(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	reg_val |= SUNXI_SPI_BATC_TBC;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
}

void sunxi_spi_bit_enable_irq(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	reg_val |= SUNXI_SPI_BATC_TBC_INT_EN;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
}

void sunxi_spi_bit_disable_irq(sunxi_spi_t *sspi)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	reg_val &= ~SUNXI_SPI_BATC_TBC_INT_EN;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
}

void sunxi_spi_bit_set_bc(sunxi_spi_t *sspi, uint32_t tx_len, uint32_t rx_len)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	reg_val &= ~(SUNXI_SPI_BATC_RX_FRM_LEN_MASK | SUNXI_SPI_BATC_TX_FRM_LEN_MASK);
	reg_val |= (tx_len & (SUNXI_SPI_BATC_TX_FRM_LEN_MASK >> SUNXI_SPI_BATC_TX_FRM_LEN_POS)) << SUNXI_SPI_BATC_TX_FRM_LEN_POS;
	reg_val |= (rx_len & (SUNXI_SPI_BATC_RX_FRM_LEN_MASK >> SUNXI_SPI_BATC_RX_FRM_LEN_POS)) << SUNXI_SPI_BATC_RX_FRM_LEN_POS;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
}

static void sunxi_spi_bit_ss_level(sunxi_spi_t *sspi, uint8_t status)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	if (status)
		reg_val |= SUNXI_SPI_BATC_SS_LEVEL;
	else
		reg_val &= ~SUNXI_SPI_BATC_SS_LEVEL;
	hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
}

void sunxi_spi_bit_ss_owner(sunxi_spi_t *sspi, uint32_t owner)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	switch (owner) {
	case HAL_SPI_CS_AUTO:
		reg_val &= ~SUNXI_SPI_BATC_SS_OWNER;
		break;
	case HAL_SPI_CS_SOFT:
		reg_val |= SUNXI_SPI_BATC_SS_OWNER;
		break;
	}

	hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
}

void sunxi_spi_bit_ss_polarity(sunxi_spi_t *sspi, bool pol)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	if (pol)
		reg_val |= SUNXI_SPI_BATC_SPOL; /* default SSPOL = 1,Low level effect */
	else
		reg_val &= ~SUNXI_SPI_BATC_SPOL;

	hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
}

static hal_spi_master_status_t sunxi_spi_bit_ss_select(sunxi_spi_t *sspi, uint32_t chipselect)
{
	hal_spi_master_status_t ret = 0;
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	if (chipselect < HAL_SPI_MASTER_MAX) {
		reg_val &= ~SUNXI_SPI_BATC_SS_SEL_MASK;
		reg_val |= (chipselect << SUNXI_SPI_BATC_SS_SEL_POS);
		hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
		ret = HAL_SPI_MASTER_OK;
	} else {
		ret = HAL_SPI_MASTER_ERROR;
	}

	return ret;
}

static void sunxi_spi_bit_config_tc(sunxi_spi_t *sspi, uint32_t config)
{
	uint32_t reg_val = hal_readl(sspi->base + SUNXI_SPI_BATC_REG);

	reg_val &= ~SUNXI_SPI_BATC_WMS_MASK;
	if (config & SPI_3WIRE)
		reg_val |= (SUNXI_SPI_BIT_3WIRE_MODE << SUNXI_SPI_BATC_WMS_POS);
	else
		reg_val |= (SUNXI_SPI_BIT_STD_MODE << SUNXI_SPI_BATC_WMS_POS);

	hal_writel(reg_val, sspi->base + SUNXI_SPI_BATC_REG);
}

/* SPI Controller Hardware Register Operation End */

static hal_spi_master_status_t sunxi_spi_set_delay_chain(sunxi_spi_t *sspi, hal_spi_master_bus_sample_mode_t sample_mode, uint32_t clk)
{
	sunxi_spi_bus_sample_mode(sspi, sample_mode);

	switch (sample_mode) {
	case SUNXI_SPI_SAMP_MODE_OLD:
		sunxi_spi_set_sample_delay_sw(sspi, false);
		if (clk >= SUNXI_SPI_SAMP_HIGH_FREQ)
			sunxi_spi_set_sample_mode(sspi, SUNXI_SPI_SAMP_DELAY_CYCLE_1_0);
		else if (clk <= SUNXI_SPI_SAMP_LOW_FREQ)
			sunxi_spi_set_sample_mode(sspi, SUNXI_SPI_SAMP_DELAY_CYCLE_0_0);
		else
			sunxi_spi_set_sample_mode(sspi, SUNXI_SPI_SAMP_DELAY_CYCLE_0_5);
		break;
	case SUNXI_SPI_SAMP_MODE_NEW:
		sunxi_spi_set_sample_delay_sw(sspi, true);
		sunxi_spi_set_sample_mode(sspi, sspi->config.spi_sample_mode);
		sunxi_spi_set_sample_delay(sspi, sspi->config.spi_sample_delay);
		break;
	default:
		SPI_ERR(sspi, "unsupport sspi sample mode %d\n", sample_mode);
	}

	return HAL_SPI_MASTER_OK;
}

static hal_spi_master_status_t sunxi_spi_bit_sample_delay(sunxi_spi_t *sspi, uint32_t clk)
{
	if (clk > SUNXI_SPI_SAMP_LOW_FREQ)
		sunxi_spi_bit_sample_mode(sspi, 1);
	else
		sunxi_spi_bit_sample_mode(sspi, 0);

	return 0;
}

static hal_spi_master_status_t sunxi_spi_xfer_setup(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;

	switch (sspi->config.bus_mode)
	{
	case HAL_SPI_BUS_MASTER:
	case HAL_SPI_BUS_SLAVE:
		sunxi_spi_config_tc(sspi, sspi->config.mode);
		break;
	case HAL_SPI_BUS_BIT:
		sunxi_spi_bit_config_tc(sspi, sspi->config.mode);
		break;
	default:
		SPI_ERR(sspi, "setup bus mode %d unsupport\n", sspi->config.bus_mode);
		ret = HAL_SPI_MASTER_INVALID_PARAMETER;
	}

	return ret;
}

static void sunxi_spi_bus_set_cs(sunxi_spi_t *sspi, bool status)
{
	hal_spi_master_status_t ret = 0;

	ret = sunxi_spi_ss_select(sspi, sspi->config.chipselect);
	if (ret < 0) {
		SPI_ERR(sspi, "failed to select cs %d\n", sspi->config.chipselect);
		return ;
	}

	sunxi_spi_ss_polarity(sspi, !(sspi->config.mode & SPI_CS_HIGH));

	if (sspi->config.cs_mode == HAL_SPI_CS_SOFT)
		sunxi_spi_ss_level(sspi, status);
}

static void sunxi_spi_bit_set_cs(sunxi_spi_t *sspi, bool status)
{
	hal_spi_master_status_t ret = 0;

	ret = sunxi_spi_bit_ss_select(sspi, sspi->config.chipselect);;
	if (ret < 0) {
		SPI_ERR(sspi, "failed to select cs %d\n", sspi->config.chipselect);
		return ;
	}

	sunxi_spi_bit_ss_polarity(sspi, !(sspi->config.mode & SPI_CS_HIGH));

	if (sspi->config.cs_mode == HAL_SPI_CS_SOFT)
		sunxi_spi_bit_ss_level(sspi, status);
}

static void sunxi_spi_set_cs(sunxi_spi_t *sspi, bool status)
{
	switch (sspi->config.bus_mode)
	{
	case HAL_SPI_BUS_BIT:
		sunxi_spi_bit_set_cs(sspi, status);
		break;
	default:
		sunxi_spi_bus_set_cs(sspi, status);
	}
}

static bool sunxi_spi_can_dma(sunxi_spi_t *sspi, uint32_t len)
{
	return (sspi->use_dma && len >= min(sspi->para->rx_fifosize, sspi->para->tx_fifosize));
}

static hal_spi_master_status_t sunxi_spi_mode_check_master(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;

	/* full duplex */
	if (t->tx_buf && t->rx_buf)
	{
		if (sspi->config.flash) /* fake full duplex for flash read/write */
		{
			sunxi_spi_set_discard_burst(sspi, true);
			sunxi_spi_set_bc_tc_stc(sspi, t->tx_len, t->rx_len, t->tx_single_len, t->dummy_byte);
			sspi->mode_type = FULL_DUPLEX_TX_RX;

			switch (t->rx_nbits)
			{
			case SPI_NBITS_QUAD:
				sunxi_spi_disable_dual(sspi);
				sunxi_spi_enable_quad(sspi);
				break;
			case SPI_NBITS_DUAL:
				sunxi_spi_disable_quad(sspi);
				sunxi_spi_enable_dual(sspi);
				break;
			default:
				sunxi_spi_disable_quad(sspi);
				sunxi_spi_disable_dual(sspi);
			}
		}
		else /* real full duplex transmit by single mode */
		{
			sunxi_spi_set_discard_burst(sspi, false);
			sunxi_spi_disable_quad(sspi);
			sunxi_spi_disable_dual(sspi);
			sunxi_spi_set_bc_tc_stc(sspi, t->tx_len, 0, t->tx_single_len, 0);
			sspi->mode_type = SINGLE_FULL_DUPLEX_RX_TX;
		}
	}
	else
	{
		/* half duplex transmit */
		sunxi_spi_set_discard_burst(sspi, true);
		if (t->tx_buf)
		{
			switch (t->tx_nbits)
			{
			case SPI_NBITS_QUAD:
				sunxi_spi_disable_dual(sspi);
				sunxi_spi_enable_quad(sspi);
				sunxi_spi_set_bc_tc_stc(sspi, t->tx_len, 0, t->tx_single_len, t->dummy_byte);
				sspi->mode_type = QUAD_HALF_DUPLEX_TX;
				break;
			case SPI_NBITS_DUAL:
				sunxi_spi_disable_quad(sspi);
				sunxi_spi_enable_dual(sspi);
				sunxi_spi_set_bc_tc_stc(sspi, t->tx_len, 0, t->tx_single_len, t->dummy_byte);
				sspi->mode_type = DUAL_HALF_DUPLEX_TX;
				break;
			default:
				sunxi_spi_disable_quad(sspi);
				sunxi_spi_disable_dual(sspi);
				sunxi_spi_set_bc_tc_stc(sspi, t->tx_len, 0, t->tx_len, t->dummy_byte);
				sspi->mode_type = SINGLE_HALF_DUPLEX_TX;
			}
		}
		else if (t->rx_buf)
		{
			switch (t->tx_nbits)
			{
			case SPI_NBITS_QUAD:
				sunxi_spi_disable_dual(sspi);
				sunxi_spi_enable_quad(sspi);
				sunxi_spi_set_bc_tc_stc(sspi, 0, t->rx_len, 0, t->dummy_byte);
				sspi->mode_type = QUAD_HALF_DUPLEX_RX;
				break;
			case SPI_NBITS_DUAL:
				sunxi_spi_disable_quad(sspi);
				sunxi_spi_enable_dual(sspi);
				sunxi_spi_set_bc_tc_stc(sspi, 0, t->rx_len, 0, t->dummy_byte);
				sspi->mode_type = DUAL_HALF_DUPLEX_RX;
				break;
			default:
				sunxi_spi_disable_quad(sspi);
				sunxi_spi_disable_dual(sspi);
				sunxi_spi_set_bc_tc_stc(sspi, 0, t->rx_len, 0, t->dummy_byte);
				sspi->mode_type = SINGLE_HALF_DUPLEX_RX;
			}
		}
	}

	SPI_DBG(sspi, "master mode type %s\n", sunxi_spi_mode[sspi->mode_type]);

	return ret;
}

static hal_spi_master_status_t sunxi_spi_mode_check_slave(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;

	if (t->tx_buf && t->rx_buf)
	{
		SPI_ERR(sspi, "slave mode not support full duplex mode\n");
		ret = HAL_SPI_MASTER_INVALID_PARAMETER;
	}
	else
	{
		sunxi_spi_set_discard_burst(sspi, true);
		sunxi_spi_disable_quad(sspi);
		sunxi_spi_disable_dual(sspi);
		sunxi_spi_set_bc_tc_stc(sspi, 0, 0, 0, 0);
		if (t->tx_buf)
			sspi->mode_type = SINGLE_HALF_DUPLEX_TX;
		else if (t->rx_buf)
			sspi->mode_type = SINGLE_HALF_DUPLEX_RX;
		SPI_DBG(sspi, "slave mode type %s\n", sunxi_spi_mode[sspi->mode_type]);
	}

	return ret;
}

static hal_spi_master_status_t sunxi_spi_mode_check_bit(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;

	if (t->tx_buf && t->rx_buf)
	{
		SPI_ERR(sspi, "bit mode not support full duplex mode\n");
		ret = HAL_SPI_MASTER_INVALID_PARAMETER;
	}
	else
	{
		if (t->bits_per_word > SUNXI_SPI_BIT_MAX_LEN)
		{
			SPI_ERR(sspi, "unsupport bits_per_word %d size\n", t->bits_per_word);
			ret = HAL_SPI_MASTER_INVALID_PARAMETER;
		}
		else if (t->tx_buf)
		{
			sspi->mode_type = SINGLE_HALF_DUPLEX_TX;
			sunxi_spi_bit_set_bc(sspi, t->bits_per_word, 0);
		}
		else if (t->rx_buf)
		{
			sspi->mode_type = SINGLE_HALF_DUPLEX_RX;
			sunxi_spi_bit_set_bc(sspi, 0, t->bits_per_word);
		}
		SPI_DBG(sspi, "bit mode type %s\n", sunxi_spi_mode[sspi->mode_type]);
	}

	return ret;
}

static hal_spi_master_status_t sunxi_spi_mode_check(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;

	if (sspi->mode_type != MODE_TYPE_NULL)
		return HAL_SPI_MASTER_ERROR;

	switch (sspi->config.bus_mode)
	{
	case HAL_SPI_BUS_MASTER:
		ret = sunxi_spi_mode_check_master(sspi, t);
		break;
	case HAL_SPI_BUS_SLAVE:
		ret = sunxi_spi_mode_check_slave(sspi, t);
		break;
	case HAL_SPI_BUS_BIT:
		ret = sunxi_spi_mode_check_bit(sspi, t);
		break;
	default:
		SPI_ERR(sspi, "mode check bus mode %d unsupport\n", sspi->config.bus_mode);
		ret = HAL_SPI_MASTER_INVALID_PARAMETER;
	}

	return ret;
}

static hal_spi_master_status_t sunxi_spi_cpu_rx(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	uint32_t len = t->rx_len;
	uint8_t *buf = t->rx_buf;
	uint32_t poll_time = POLL_TIMEOUT;

#ifdef SPI_DUMPREG_LEVEL
	sunxi_spi_dump_reg(sspi, 0, 0x40);
#endif

	while (len && poll_time)
	{
		/* rxFIFO counter */
		if (sunxi_spi_query_rxfifo(sspi))
		{
			*buf++ = hal_readb(sspi->base + SUNXI_SPI_RXDATA_REG);
			--len;
			poll_time = POLL_TIMEOUT;
		}
		else
		{
			--poll_time;
		}
	}

	if (poll_time <= 0)
	{
		SPI_ERR(sspi, "cpu receive data time out\n");
		sunxi_spi_dump_reg(sspi, 0, 0x40);
		sspi->result = SPI_XFER_FAILED;
		return HAL_SPI_MASTER_ERROR_TIMEOUT;
	}

#ifdef SPI_DATA_LEVEL
	sunxi_spi_dump_data(sspi, buf, sspi->transfer->rx_len);
#endif

	return HAL_SPI_MASTER_OK;
}

static hal_spi_master_status_t sunxi_spi_cpu_tx(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	uint32_t len = t->tx_len;
	const uint8_t *buf = t->tx_buf;
	uint32_t poll_time = POLL_TIMEOUT;

#ifdef SPI_DUMPREG_LEVEL
	sunxi_spi_dump_reg(sspi, 0, 0x40);
#endif

#ifdef SPI_DATA_LEVEL
	sunxi_spi_dump_data(sspi, buf, len);
#endif

	while (len && poll_time)
	{
		if (sunxi_spi_query_txfifo(sspi) >= sspi->para->tx_fifosize)
		{
			--poll_time;
		}
		else
		{
			hal_writeb(*buf++, sspi->base + SUNXI_SPI_TXDATA_REG);
			--len;
			poll_time = POLL_TIMEOUT;
		}
	}

	if (poll_time <= 0)
	{
		SPI_ERR(sspi, "cpu transfer data time out\n");
		sunxi_spi_dump_reg(sspi, 0, 0x40);
		sspi->result = SPI_XFER_FAILED;
		return HAL_SPI_MASTER_ERROR_TIMEOUT;
	}

	return HAL_SPI_MASTER_OK;
}

static hal_spi_master_status_t sunxi_spi_cpu_tx_rx(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	uint32_t len = t->tx_len;
	const uint8_t *tx_buf = t->tx_buf;
	uint8_t *rx_buf = t->rx_buf;
	uint32_t align_loop, left_loop;
	int i;
	uint8_t fifosize;
	hal_spi_master_status_t ret = 0;

	fifosize = min(sspi->para->rx_fifosize, sspi->para->tx_fifosize);

	align_loop = len / fifosize;
	left_loop  = len % fifosize;

	if (align_loop > 0) {
		for (i = 0; i < align_loop; i++) {
			sspi->transfer->tx_len = fifosize;
			sspi->transfer->rx_len = fifosize;
			ret = sunxi_spi_cpu_tx(sspi, t);
			if (ret < 0)
				goto err0;
			ret = sunxi_spi_cpu_rx(sspi, t);
			if (ret < 0)
				goto err0;
			sspi->transfer->tx_buf += fifosize;
			sspi->transfer->rx_buf += fifosize;
		}
	}

	if (left_loop)
	{
		sspi->transfer->tx_len = left_loop;
		sspi->transfer->rx_len = left_loop;
		ret = sunxi_spi_cpu_tx(sspi, t);
		if (ret < 0)
			goto err0;
		ret = sunxi_spi_cpu_rx(sspi, t);
		if (ret < 0)
			goto err0;
	}

err0:
	sspi->transfer->tx_len = len;
	sspi->transfer->rx_len = len;
	sspi->transfer->tx_buf = tx_buf;
	sspi->transfer->rx_buf = rx_buf;
	if (ret < 0)
		SPI_ERR(sspi, "cpu tx rx error with align_%d left_%d i_%d ret_%d\n", align_loop, left_loop, i, ret);

	return ret;
}

static hal_spi_master_status_t sunxi_spi_bit_cpu_rx(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	uint8_t bits = t->bits_per_word;
	uint8_t *buf = (uint8_t *)t->rx_buf;
	uint32_t data;

	if (bits <= 8) {
		data = hal_readb(sspi->base + SUNXI_SPI_RB_REG);
		*((uint8_t *)buf) = data & (BIT(bits) - 1);
	} else if (bits <= 16) {
		data = hal_readw(sspi->base + SUNXI_SPI_RB_REG);
		*((u16 *)buf) = data & (BIT(bits) - 1);
	} else {
		data = hal_readl(sspi->base + SUNXI_SPI_RB_REG);
		*((uint32_t *)buf) = data & (BIT(bits) - 1);
	}

	SPI_DBG(sspi, "bit cpu rx data %#x\n", data);

	return 0;
}

static hal_spi_master_status_t sunxi_spi_bit_cpu_tx(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	uint8_t bits = t->bits_per_word;
	uint8_t *buf = (uint8_t *)t->tx_buf;
	uint32_t data;

	if (bits <= 8) {
		data = *((uint8_t *)buf) & (BIT(bits) - 1);
		hal_writeb(data, sspi->base + SUNXI_SPI_TB_REG);
	} else if (bits <= 16) {
		data = *((u16 *)buf) & (BIT(bits) - 1);
		hal_writew(data, sspi->base + SUNXI_SPI_TB_REG);
	} else {
		data = *((uint32_t *)buf) & (BIT(bits) - 1);
		hal_writel(data, sspi->base + SUNXI_SPI_TB_REG);
	}

	SPI_DBG(sspi, "bit cpu tx data %#x\n", data);

	return 0;
}

static void sunxi_spi_dma_cb_rx(void *data)
{
	sunxi_spi_t *sspi = (sunxi_spi_t *)data;
	hal_spi_master_transfer_t *t = sspi->transfer;
	uint32_t cnt;

	SPI_DBG(sspi, "dma rx callback\n");

	cnt = sunxi_spi_query_rxfifo(sspi);
	if (cnt > 0)
	{
		SPI_ERR(sspi, "dma done but rxfifo not empty 0x%x\n", cnt);
		sunxi_spi_soft_reset(sspi);
		sspi->result = -1;
	}

	hal_dcache_invalidate((unsigned long)sspi->align_dma_buf, ALIGN_UP(t->rx_len, 64));
	memcpy(t->rx_buf, sspi->align_dma_buf, t->rx_len);

	hal_sem_post(sspi->done);
}

static void sunxi_spi_dma_cb_tx(void *data)
{
	sunxi_spi_t *sspi = (sunxi_spi_t *)data;

	SPI_DBG(sspi, "dma tx callback\n");
}

static void sunxi_spi_config_dma(struct dma_slave_config *config, int len, uint32_t triglevel, bool dma_force_fixed)
{
	int width, burst;

	if (dma_force_fixed) {
		/* if dma is force fixed, use old configuration to make sure the stability and compatibility */
		if (len % DMA_SLAVE_BUSWIDTH_4_BYTES == 0)
			width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		else
			width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		burst = 4;
	} else {
		if (len % DMA_SLAVE_BUSWIDTH_4_BYTES == 0) {
			width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			if (triglevel < SUNXI_SPI_FIFO_DEFAULT)
				burst = 8;
			else
				burst = 16;
		} else if (len % DMA_SLAVE_BUSWIDTH_2_BYTES == 0) {
			width = DMA_SLAVE_BUSWIDTH_2_BYTES;
			burst = 16;
		} else {
			width = DMA_SLAVE_BUSWIDTH_1_BYTE;
			burst = 16;
		}
	}

	config->src_addr_width = width;
	config->dst_addr_width = width;
	config->src_maxburst = burst;
	config->dst_maxburst = burst;
}

static hal_spi_master_status_t sunxi_spi_config_dma_rx(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_dma_chan_status_t ret;
	struct dma_slave_config *config = &sspi->dma_rx.config;

	if (t->rx_len > ALIGN_DMA_BUF_SIZE)
	{
		SPI_ERR(sspi, "rx len is over dma align buf size %d\n", ALIGN_DMA_BUF_SIZE);
		return HAL_SPI_MASTER_INVALID_PARAMETER;
	}

	memset(sspi->align_dma_buf, 0, t->rx_len);
	hal_dcache_invalidate((unsigned long)sspi->align_dma_buf, ALIGN_UP(t->rx_len, 64));

#ifdef SPI_DUMPREG_LEVEL
	sunxi_spi_dump_reg(sspi, 0, 0x40);
#endif

	config->direction = DMA_DEV_TO_MEM;
	config->dst_addr = (unsigned long)sspi->align_dma_buf;
	config->src_addr = sspi->base + SUNXI_SPI_RXDATA_REG;
	if (sspi->config.bus_mode == HAL_SPI_BUS_SLAVE && sspi->para->dma_force_fixed)
	{
		config->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		config->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		config->dst_maxburst = DMA_SLAVE_BURST_1;
		config->src_maxburst = DMA_SLAVE_BURST_1;
	}
	else
	{
		sunxi_spi_config_dma(config, t->rx_len, sspi->rx_triglevel, sspi->para->dma_force_fixed);
	}
	config->slave_id = sunxi_slave_id(DRQDST_SDRAM, sspi->para->drq_rx);

	SPI_DBG(sspi, "config:\n"
				"  direction: %d\n"
				"  src_addr: 0x%08lx,"
				"  dst_addr: 0x%08lx\n"
				"  src_addr_width: %d,"
				"  dst_addr_width: %d\n"
				"  src_maxburst: %lu,"
				"  dst_maxburst: %lu\n"
				"  slave_id: 0x%08lx\n",
				config->direction, config->src_addr,
				config->dst_addr, config->src_addr_width,
				config->dst_addr_width, config->src_maxburst,
				config->dst_maxburst, config->slave_id);

	ret = hal_dma_slave_config(sspi->dma_rx.chan, config);
	if (ret)
	{
		SPI_ERR(sspi, "dma rx slave config failed %d\n", ret);
		return HAL_SPI_MASTER_ERROR;
	}

	ret = hal_dma_prep_device(sspi->dma_rx.chan, config->dst_addr, config->src_addr, t->rx_len, config->direction);
	if (ret)
	{
		SPI_ERR(sspi, "dma rx prep device failed %d\n", ret);
		return HAL_SPI_MASTER_ERROR;
	}

	sspi->dma_rx.chan->callback = sunxi_spi_dma_cb_rx;
	sspi->dma_rx.chan->callback_param = (void *)sspi;

	return HAL_SPI_MASTER_OK;
}

static hal_spi_master_status_t sunxi_spi_config_dma_tx(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_dma_chan_status_t ret;
	struct dma_slave_config *config = &sspi->dma_tx.config;

	if (t->tx_len > ALIGN_DMA_BUF_SIZE)
	{
		SPI_ERR(sspi, "tx len is over dma align buf size %d\n", ALIGN_DMA_BUF_SIZE);
		return HAL_SPI_MASTER_INVALID_PARAMETER;
	}

	memcpy(sspi->align_dma_buf, t->tx_buf, t->tx_len);
	hal_dcache_clean((unsigned long)sspi->align_dma_buf, ALIGN_UP(t->tx_len, 64));

#ifdef SPI_DUMPREG_LEVEL
	sunxi_spi_dump_reg(sspi, 0, 0x40);
#endif

#ifdef SPI_DATA_LEVEL
	sunxi_spi_dump_data(sspi, t->tx_buf, t->tx_len);
#endif

	config->direction = DMA_MEM_TO_DEV;
	config->dst_addr = sspi->base + SUNXI_SPI_TXDATA_REG;
	config->src_addr = (unsigned long)sspi->align_dma_buf;
	sunxi_spi_config_dma(config, t->tx_len, sspi->tx_triglevel, sspi->para->dma_force_fixed);
	config->slave_id = sunxi_slave_id(sspi->para->drq_tx, DRQDST_SDRAM);

	SPI_DBG(sspi, "config:\n"
				"  direction: %d\n"
				"  src_addr: 0x%08lx,"
				"  dst_addr: 0x%08lx\n"
				"  src_addr_width: %d,"
				"  dst_addr_width: %d\n"
				"  src_maxburst: %lu,"
				"  dst_maxburst: %lu\n"
				"  slave_id: 0x%08lx\n",
				config->direction, config->src_addr,
				config->dst_addr, config->src_addr_width,
				config->dst_addr_width, config->src_maxburst,
				config->dst_maxburst, config->slave_id);

	ret = hal_dma_slave_config(sspi->dma_tx.chan, config);
	if (ret)
	{
		SPI_ERR(sspi, "dma tx slave config failed %d\n", ret);
		return HAL_SPI_MASTER_ERROR;
	}

	ret = hal_dma_prep_device(sspi->dma_tx.chan, config->dst_addr, config->src_addr, t->tx_len, config->direction);
	if (ret)
	{
		SPI_ERR(sspi, "dma tx prep device failed %d\n", ret);
		return HAL_SPI_MASTER_ERROR;
	}

	sspi->dma_tx.chan->callback = sunxi_spi_dma_cb_tx;
	sspi->dma_tx.chan->callback_param = (void *)sspi;

	return HAL_SPI_MASTER_OK;
}

static hal_spi_master_status_t sunxi_spi_dma_rx(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_dma_chan_status_t ret;

	sunxi_spi_enable_dma_irq(sspi, SUNXI_SPI_FIFO_CTL_RX_DRQ_EN);
	sunxi_spi_config_dma_rx(sspi, t);
	ret = hal_dma_start(sspi->dma_rx.chan);
	if (ret)
	{
		SPI_ERR(sspi, "dma rx start error %d\n", ret);
		return HAL_SPI_MASTER_ERROR;
	}

	return HAL_SPI_MASTER_OK;
}

static hal_spi_master_status_t sunxi_spi_dma_tx(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_dma_chan_status_t ret;

	sunxi_spi_enable_dma_irq(sspi, SUNXI_SPI_FIFO_CTL_TX_DRQ_EN);
	sunxi_spi_config_dma_tx(sspi, t);
	ret = hal_dma_start(sspi->dma_tx.chan);
	if (ret)
	{
		SPI_ERR(sspi, "dma tx start error %d\n", ret);
		return HAL_SPI_MASTER_ERROR;
	}

	return HAL_SPI_MASTER_OK;
}

static hal_spi_master_status_t sunxi_spi_xfer_master(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;
	int timeout;

	sunxi_spi_enable_irq(sspi, SUNXI_SPI_INT_CTL_TC_EN);

	switch (sspi->mode_type)
	{
	case SINGLE_HALF_DUPLEX_RX:
	case DUAL_HALF_DUPLEX_RX:
	case QUAD_HALF_DUPLEX_RX:
		if (sunxi_spi_can_dma(sspi, t->rx_len))
		{
			SPI_DBG(sspi, "master xfer rx by dma %d\n", t->rx_len);
			sunxi_spi_disable_irq(sspi, SUNXI_SPI_INT_CTL_TC_EN);
			ret = sunxi_spi_dma_rx(sspi, t);
			if (ret < 0)
				goto out;
			sunxi_spi_start_xfer(sspi);
		}
		else
		{
			SPI_DBG(sspi, "master xfer rx by cpu %d\n", t->rx_len);
			sunxi_spi_start_xfer(sspi);
			ret = sunxi_spi_cpu_rx(sspi, t);
			if (ret < 0)
				goto out;
		}
		break;
	case SINGLE_HALF_DUPLEX_TX:
	case DUAL_HALF_DUPLEX_TX:
	case QUAD_HALF_DUPLEX_TX:
		if (sunxi_spi_can_dma(sspi, t->tx_len))
		{
			SPI_DBG(sspi, "master xfer tx by dma %d\n", t->tx_len);
			sunxi_spi_start_xfer(sspi);
			ret = sunxi_spi_dma_tx(sspi, t);
			if (ret < 0)
				goto out;
		}
		else
		{
			SPI_DBG(sspi, "master xfer tx by cpu %d\n", t->tx_len);
			sunxi_spi_start_xfer(sspi);
			ret = sunxi_spi_cpu_tx(sspi, t);
			if (ret < 0)
				goto out;
		}
		break;
	case SINGLE_FULL_DUPLEX_RX_TX:
		if (sunxi_spi_can_dma(sspi, t->tx_len)) {
			SPI_DBG(sspi, "master xfer rx & tx by dma %d\n", t->tx_len);
			sunxi_spi_disable_irq(sspi, SUNXI_SPI_INT_CTL_TC_EN);
			ret = sunxi_spi_dma_rx(sspi, t);
			if (ret < 0)
				goto out;
			sunxi_spi_start_xfer(sspi);
			ret = sunxi_spi_dma_tx(sspi, t);
			if (ret < 0)
				goto out;
		} else {
			SPI_DBG(sspi, "master xfer rx & tx by cpu %d\n", t->tx_len);
			sunxi_spi_start_xfer(sspi);
			ret = sunxi_spi_cpu_tx_rx(sspi, t);
			if (ret < 0)
				goto out;
		}
		break;
	case FULL_DUPLEX_TX_RX:
		if (sunxi_spi_can_dma(sspi, t->rx_len))
		{
			SPI_DBG(sspi, "master xfer tx %d then rx %d by dma\n", t->tx_len, t->rx_len);
			sunxi_spi_disable_irq(sspi, SUNXI_SPI_INT_CTL_TC_EN);
			sunxi_spi_start_xfer(sspi);
			ret = sunxi_spi_cpu_tx(sspi, t);
			if (ret < 0)
				goto out;
			ret = sunxi_spi_dma_rx(sspi, t);
			if (ret < 0)
				goto out;
		}
		else
		{
			SPI_DBG(sspi, "master xfer tx %d then rx %d by cpu\n", t->tx_len, t->rx_len);
			sunxi_spi_start_xfer(sspi);
			ret = sunxi_spi_cpu_tx(sspi, t);
			if (ret < 0)
				goto out;
			ret = sunxi_spi_cpu_rx(sspi, t);
			if (ret < 0)
				goto out;
		}
		break;
	default:
		SPI_ERR(sspi, "unknown master transfer mode type %d\n", sspi->mode_type);
		ret = HAL_SPI_MASTER_INVALID_PARAMETER;
		goto out;
	}

	timeout = hal_sem_timedwait(sspi->done, XFER_TIMEOUT);
	if (timeout != 0)
	{
		SPI_ERR(sspi, "master transfer timeout type(%s)\n", sunxi_spi_mode[sspi->mode_type]);
		sspi->result = SPI_XFER_FAILED;
		sunxi_spi_dump_reg(sspi, 0, 0x40);
		ret = HAL_SPI_MASTER_ERROR_TIMEOUT;
		goto out;
	}
	else if (sspi->result < 0)
	{
		SPI_ERR(sspi, "master transfer failed %d\n", sspi->result);
		sunxi_spi_dump_reg(sspi, 0, 0x40);
		ret = HAL_SPI_MASTER_ERROR;
		goto out;
	}

out:
	if (sunxi_spi_can_dma(sspi, t->tx_len) || sunxi_spi_can_dma(sspi, t->rx_len)) {
		hal_dma_stop(sspi->dma_rx.chan);
		hal_dma_stop(sspi->dma_tx.chan);
		hal_dma_chan_desc_free(sspi->dma_rx.chan);
		hal_dma_chan_desc_free(sspi->dma_tx.chan);
	}

	return ret;
}

hal_spi_master_status_t sunxi_spi_xfer_slave(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;
	uint32_t poll_time = POLL_TIMEOUT;

	sspi->slave_aborted = false;

	switch (sspi->mode_type)
	{
	case SINGLE_HALF_DUPLEX_RX:
		if (sunxi_spi_can_dma(sspi, t->rx_len))
		{
			SPI_DBG(sspi, "slave xfer rx by dma %d\n", t->rx_len);
			if (sspi->para->dma_force_fixed)
				sunxi_spi_set_fifo_trig_level_rx(sspi, 1);
			ret = sunxi_spi_dma_rx(sspi, t);
			if (ret < 0)
				goto out;
		}
		else
		{
			SPI_DBG(sspi, "slave xfer rx by cpu %d\n",t->rx_len);
			sunxi_spi_set_fifo_trig_level_rx(sspi, t->rx_len - 1);
			sunxi_spi_clr_irq_pending(sspi, SUNXI_SPI_INT_STA_RX_RDY);
			sunxi_spi_enable_irq(sspi, SUNXI_SPI_INT_CTL_RX_RDY_EN);
			if (hal_sem_wait(sspi->done) || sspi->slave_aborted)
			{
				SPI_WARN(sspi, "slave xfer aborted\n");
				ret = HAL_SPI_MASTER_ERROR;
				goto out;
			}
			if (sspi->result < 0)
			{
				SPI_ERR(sspi, "slave transfer failed %d\n", sspi->result);
				ret = HAL_SPI_MASTER_ERROR;
				goto out;
			}
			return sunxi_spi_cpu_rx(sspi, t);
		}
		break;
	case SINGLE_HALF_DUPLEX_TX:
		if (sunxi_spi_can_dma(sspi, t->tx_len))
		{
			SPI_DBG(sspi, "slave xfer tx by dma %d\n", t->tx_len);
			ret = sunxi_spi_dma_tx(sspi, t);
			if (ret < 0)
				goto out;
			/* Waiting for DMA fill data into fifo */
			while (!sunxi_spi_query_txfifo(sspi) && poll_time--)
				;
			if (poll_time <= 0) {
				SPI_ERR(sspi, "timeout for waiting fill data into fifo\n");
				ret = HAL_SPI_MASTER_ERROR_TIMEOUT;
				goto out;
			}
		}
		else
		{
			SPI_DBG(sspi, "slave xfer tx by cpu %d\n", t->tx_len);
			ret = sunxi_spi_cpu_tx(sspi, t);
			if (ret < 0)
				goto out;
		}
		sunxi_spi_clr_irq_pending(sspi, SUNXI_SPI_INT_STA_TX_EMP);
		sunxi_spi_enable_irq(sspi, SUNXI_SPI_INT_CTL_TX_EMP_EN);
		break;
	default:
		SPI_ERR(sspi, "unknown slave transfer mode type %d\n", sspi->mode_type);
		ret = HAL_SPI_MASTER_INVALID_PARAMETER;
		goto out;
	}

	if (hal_sem_wait(sspi->done) || sspi->slave_aborted)
	{
		SPI_WARN(sspi, "slave xfer aborted\n");
		sspi->result = SPI_XFER_FAILED;
		ret = HAL_SPI_MASTER_ERROR;
		goto out;
	}
	if (sspi->result < 0)
	{
		SPI_ERR(sspi, "slave transfer failed %d\n", sspi->result);
		ret = HAL_SPI_MASTER_ERROR;
		goto out;
	}

out:
	if (sunxi_spi_can_dma(sspi, t->tx_len) || sunxi_spi_can_dma(sspi, t->rx_len)) {
		hal_dma_stop(sspi->dma_rx.chan);
		hal_dma_stop(sspi->dma_tx.chan);
		hal_dma_chan_desc_free(sspi->dma_rx.chan);
		hal_dma_chan_desc_free(sspi->dma_tx.chan);
	}

	return ret;
}

static hal_spi_master_status_t sunxi_spi_xfer_bit(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;
	int timeout;

	switch (sspi->mode_type) {
	case SINGLE_HALF_DUPLEX_RX:
		SPI_DBG(sspi, "bit xfer rx by cpu %d\n", t->bits_per_word);
		sunxi_spi_bit_start_xfer(sspi);
		break;
	case SINGLE_HALF_DUPLEX_TX:
		SPI_DBG(sspi, "bit xfer tx by cpu %d\n", t->bits_per_word);
		ret = sunxi_spi_bit_cpu_tx(sspi, t);
		sunxi_spi_bit_start_xfer(sspi);
		break;
	default:
		SPI_ERR(sspi, "unknown bit transfer mode type %d\n", sspi->mode_type);
		ret = HAL_SPI_MASTER_INVALID_PARAMETER;
		goto out;
	}

	timeout = hal_sem_timedwait(sspi->done, XFER_TIMEOUT);
	if (timeout != 0)
	{
		SPI_ERR(sspi, "bit transfer timeout type(%s)\n", sunxi_spi_mode[sspi->mode_type]);
		sspi->result = SPI_XFER_FAILED;
		sunxi_spi_dump_reg(sspi, 0, 0x40);
		ret = HAL_SPI_MASTER_ERROR_TIMEOUT;
		goto out;
	}
	else if (sspi->result < 0)
	{
		SPI_ERR(sspi, "bit transfer failed %d\n", sspi->result);
		sunxi_spi_dump_reg(sspi, 0, 0x40);
		ret = HAL_SPI_MASTER_ERROR;
		goto out;
	}

	/* Bit-Aligned mode didn't have hw fifo, read the data immediately when transfer complete */
	if (sspi->mode_type == SINGLE_HALF_DUPLEX_RX)
		ret = sunxi_spi_bit_cpu_rx(sspi, t);

out:
	return ret;
}

static void sunxi_spi_master_handler(sunxi_spi_t *sspi, uint32_t enable, uint32_t status)
{
	if (status & SUNXI_SPI_INT_STA_TC)
	{
		SPI_DBG(sspi, "master IRQ TC comes\n");
		sunxi_spi_disable_irq(sspi, SUNXI_SPI_INT_CTL_TC_EN | SUNXI_SPI_INT_CTL_ERR);
		hal_sem_post(sspi->done);
	}
	else if (status & SUNXI_SPI_INT_STA_ERR)
	{
		SPI_ERR(sspi, "master IRQ status error %"PRIx32"\n", status);
		sunxi_spi_disable_irq(sspi, SUNXI_SPI_INT_CTL_TC_EN | SUNXI_SPI_INT_CTL_ERR);
		sunxi_spi_soft_reset(sspi);
		sspi->result = SPI_XFER_FAILED;
		hal_sem_post(sspi->done);
	}
	else
	{
		SPI_DBG(sspi, "master unkown status(%"PRIx32") enable(%"PRIx32")\n", status, enable);
	}
}

static int sunxi_spi_slave_handler(sunxi_spi_t *sspi, uint32_t enable, uint32_t status)
{
	if ((enable & SUNXI_SPI_INT_CTL_RX_RDY_EN) && (status & SUNXI_SPI_INT_STA_RX_RDY))
	{
		SPI_DBG(sspi, "slave recv data is ready\n");
		sunxi_spi_disable_irq(sspi, SUNXI_SPI_INT_CTL_RX_RDY_EN);
		hal_sem_post(sspi->done);
	}
	else if ((enable & SUNXI_SPI_INT_CTL_TX_EMP_EN) && (status & SUNXI_SPI_INT_STA_TX_EMP))
	{
		SPI_DBG(sspi, "slave send data is done\n");
		sunxi_spi_disable_irq(sspi, SUNXI_SPI_INT_CTL_TX_EMP_EN);
		hal_sem_post(sspi->done);
	}
	else if (status & SUNXI_SPI_INT_STA_TX_OVF)
	{
		SPI_DBG(sspi, "slave txfifo overflow %"PRIx32"\n", status);
		sspi->result = SPI_XFER_FAILED;
		hal_sem_post(sspi->done);
	}
	else
	{
		SPI_DBG(sspi, "slave unkown status(%"PRIx32") enable(%"PRIx32")\n", status, enable);
	}

	return 0;
}

static void unxi_spi_bit_handler(sunxi_spi_t *sspi, uint32_t enable, uint32_t status)
{
	if (status & SUNXI_SPI_BATC_TBC)
	{
		SPI_DBG(sspi, "bit IRQ TC comes\n");
		sunxi_spi_bit_disable_irq(sspi);
		hal_sem_post(sspi->done);
	}
	else
	{
		SPI_ERR(sspi, "bit unkown status(%"PRIx32")\n", status);
	}
}

/* wake up the sleep thread, and give the result code */
static hal_irqreturn_t sunxi_spi_handler(void *ptr)
{
	sunxi_spi_t *sspi = (sunxi_spi_t *)ptr;
	uint32_t status = 0, enable = 0;

	switch (sspi->config.bus_mode)
	{
	case HAL_SPI_BUS_BIT:
		status = sunxi_spi_bit_qry_irq_pending(sspi);
		sunxi_spi_bit_clr_irq_pending(sspi);
		SPI_DBG(sspi, "bit irq handler status(%"PRIx32")\n", status);
		break;
	default:
		enable = sunxi_spi_qry_irq_enable(sspi);
		status = sunxi_spi_qry_irq_pending(sspi);
		sunxi_spi_clr_irq_pending(sspi, status);
		SPI_DBG(sspi, "spi irq handler enable(%"PRIx32") status(%"PRIx32")\n", enable, status);
	}

	switch (sspi->config.bus_mode)
	{
	case HAL_SPI_BUS_MASTER:
		sunxi_spi_master_handler(sspi, enable, status);
		break;
	case HAL_SPI_BUS_SLAVE:
		sunxi_spi_slave_handler(sspi, enable, status);
		break;
	case HAL_SPI_BUS_BIT:
		unxi_spi_bit_handler(sspi, enable, status);
		break;
	default:
		SPI_ERR(sspi, "irq bus mode %d unsupport\n", sspi->config.bus_mode);
	}

	return 0;
}

static hal_spi_master_status_t sunxi_spi_clk_init(sunxi_spi_t *sspi, uint32_t mod_clk)
{
	unsigned long rate;
	struct sunxi_spi_params_t *para = sspi->para;
	int rate_pll = 0, rate_hosc = 0;
	hal_clk_status_t ret_clk;
	hal_clk_t pclk_pll, pclk_hosc;
	hal_spi_master_status_t ret_spi = 0;

	sspi->reset = hal_reset_control_get(para->reset_type, para->reset_id);
	hal_reset_control_deassert(sspi->reset);

	sspi->mclk = hal_clock_get(para->mclk_type, para->mclk_id);
	sspi->bus_clk = hal_clock_get(para->bus_type, para->bus_id);
	pclk_pll = hal_clock_get(para->pclk_pll_type, para->pclk_pll_id);
	pclk_hosc = hal_clock_get(para->pclk_hosc_type, para->pclk_hosc_id);

	SPI_DBG(sspi, "get pclk_pll %d\n", hal_clk_get_rate(pclk_pll));
	SPI_DBG(sspi, "get pclk_hosc %d\n", hal_clk_get_rate(pclk_hosc));
	SPI_DBG(sspi, "get mod rate %d\n", mod_clk);

	if (hal_clk_get_rate(pclk_pll) >= mod_clk)
	{
		if (!hal_clk_set_parent(sspi->mclk, pclk_pll))
			rate_pll = hal_clk_round_rate(sspi->mclk, mod_clk);
	}
	if (hal_clk_get_rate(pclk_hosc) >= mod_clk)
	{
		if (!hal_clk_set_parent(sspi->mclk, pclk_hosc))
			rate_hosc = hal_clk_round_rate(sspi->mclk, mod_clk);
	}

	if (abs(rate_pll - mod_clk) < abs(rate_hosc - mod_clk))
	{
		sspi->pclk = pclk_pll;
		SPI_DBG(sspi, "choose pll clk mod rate\n", rate_pll);
	}
	else
	{
		sspi->pclk = pclk_hosc;
		SPI_DBG(sspi, "choose hosc clk mod rate\n", rate_hosc);
	}

	ret_clk = hal_clk_set_parent(sspi->mclk, sspi->pclk);
	if (ret_clk)
	{
		SPI_ERR(sspi, "failed set mclk parent to pclk %d\n", ret_clk);
		ret_spi = HAL_SPI_MASTER_ERROR;
		goto out;
	}

	rate = hal_clk_round_rate(sspi->mclk, mod_clk);
	ret_clk = hal_clk_set_rate(sspi->mclk, rate);
	if (ret_clk)
	{
		SPI_ERR(sspi, "failed set mclk %u rate %d\n", rate, ret_clk);
		ret_spi = HAL_SPI_MASTER_ERROR;
		goto out;
	}

	sspi->config.clock_frequency = rate;

	ret_clk = hal_clock_enable(sspi->bus_clk);
	if (ret_clk)
	{
		SPI_ERR(sspi, "failed to enable bus_clk %d\n", ret_clk);
		ret_spi = HAL_SPI_MASTER_ERROR;
		goto out;
	}

	ret_clk = hal_clock_enable(sspi->mclk);
	if (ret_clk)
	{
		SPI_ERR(sspi, "failed to enable mclk %d\n", ret_clk);
		ret_spi = HAL_SPI_MASTER_ERROR;
		goto out;
	}

	SPI_INFO(sspi, "init clock rate %lu success\n", rate);

out:
	return ret_spi;
}

static hal_spi_master_status_t sunxi_spi_clk_exit(sunxi_spi_t *sspi)
{
	hal_clk_status_t ret;

	hal_clock_disable(sspi->bus_clk);
	hal_clock_disable(sspi->mclk);

	hal_clock_put(sspi->bus_clk);
	hal_clock_put(sspi->mclk);

	hal_reset_control_assert(sspi->reset);
	hal_reset_control_put(sspi->reset);

	return HAL_SPI_MASTER_OK;
}

static hal_spi_master_status_t sunxi_spi_pinctrl_init(sunxi_spi_t *sspi)
{
	char spi_name[16];
#ifdef CONFIG_DRIVER_SYSCONFIG
	user_gpio_set_t gpio_cfg[SPI_MAX_GPIO_NUM] = {0};
#endif
	gpio_pin_t spi_pin[SPI_MAX_GPIO_NUM] = {0};
	gpio_muxsel_t spi_muxsel[SPI_MAX_GPIO_NUM] = {0};
	gpio_driving_level_t spi_drv_level[SPI_MAX_GPIO_NUM] = {0};
	struct sunxi_spi_params_t *para = sspi->para;
	int use_syscfg = false;
	int count, i;
	hal_spi_master_status_t ret = 0;

#ifdef CONFIG_DRIVER_SYSCONFIG
	memset(spi_name, 0, sizeof(spi_name));
	sprintf(spi_name, "spi%d", sspi->bus_num);
	count = hal_cfg_get_gpiosec_keycount(spi_name);
	if (count >= SPI_MIN_GPIO_NUM)
	{
		SPI_INFO(sspi, "use pinctrl in sys_config\n");
		hal_cfg_get_gpiosec_data(spi_name, gpio_cfg, count);
		for (i = 0; i < count; i++)
		{
			spi_pin[i] = (gpio_cfg[i].port - 1) * 32 + gpio_cfg[i].port_num;
			spi_muxsel[i] = gpio_cfg[i].mul_sel;
			spi_drv_level[i] = gpio_cfg[i].drv_level;
		}
		use_syscfg = true;
		goto pin_setting;
	}
	else
	{
		SPI_ERR(sspi, "count %d not support in sys_config\n", count);
		/* go fallthrough, use platform header config */
	}
#endif

	spi_pin[0] = para->gpio_clk;
	spi_pin[1] = para->gpio_mosi;
	spi_pin[2] = para->gpio_cs0;
	spi_pin[3] = para->gpio_miso;
	spi_pin[4] = para->gpio_wp;
	spi_pin[5] = para->gpio_hold;
	count = para->gpio_num;
	for (i = 0; i < count; i++)
	{
		spi_muxsel[i] = para->mux;
		spi_drv_level[i] = para->driv_level;
	}

	if (count >= SPI_MIN_GPIO_NUM)
	{
		SPI_INFO(sspi, "use pinctrl in platform header\n");
		goto pin_setting;
	}
	else
	{
		SPI_ERR(sspi, "count %d not support in platform header\n", count);
		return HAL_SPI_MASTER_INVALID_PARAMETER;
	}

pin_setting:
	for (i = 0; i < count; i++)
	{
		ret = hal_gpio_pinmux_set_function(spi_pin[i], spi_muxsel[i]);
		if (ret)
		{
			SPI_ERR(sspi, "PIN%u set function failed! return %d\n", spi_pin[i], ret);
			ret = HAL_SPI_MASTER_ERROR;
			goto exit;
		}

		ret = hal_gpio_set_driving_level(spi_pin[i], spi_drv_level[i]);
		if (ret)
		{
			SPI_ERR(sspi, "PIN%u set driving level failed! return %d\n", spi_pin[i], ret);
			ret = HAL_SPI_MASTER_ERROR;
			goto exit;
		}

		if (use_syscfg)
		{
#ifdef CONFIG_DRIVER_SYSCONFIG
			if (gpio_cfg[i].pull)
				ret = hal_gpio_set_pull(spi_pin[i], gpio_cfg[i].pull);
#endif
		}
		else
		{
			if (i == 2 || i == 4 || i == 5) /* cs/wp/hold */
				ret = hal_gpio_set_pull(spi_pin[i], GPIO_PULL_UP);
			else
				ret = hal_gpio_set_pull(spi_pin[i], GPIO_PULL_DOWN_DISABLED);
		}
	}

exit:
	return ret;
}

static hal_spi_master_status_t sunxi_spi_request_dma(sunxi_spi_t *sspi)
{
#ifdef CONFIG_DRIVERS_DMA
	hal_dma_chan_status_t ret_dma = HAL_DMA_CHAN_STATUS_FREE;
	hal_spi_master_status_t ret_spi = HAL_SPI_MASTER_OK;

#ifdef CONFIG_ARCH_SUN55IW3
	/* For SUN55IW3 platform, R_SPI default select DSP_DMA port.
	 * If need use R_SPI under SYS_DMA, pls. change select port.
	 * Other SPI port can only use SYS_DMA, no need to select.
	 */
	if (sspi->para->rspi_dma_sel) {
		sspi->dma_sel_base = RSPI_DMA_SEL_ADDR + RSPI_DMA_SEL_OFFSET;
		/* R_SPI DMA SEL : 0-DSP_DMA(default) 1-SYS_DMA */
#if defined(CONFIG_ARCH_RISCV)
	hal_writel(hal_readl(sspi->dma_sel_base) | (1 << RSPI_DMA_SEL_BIT), sspi->dma_sel_base);
#elif defined(CONFIG_ARCH_DSP)
	hal_writel(hal_readl(sspi->dma_sel_base) & ~(1 << RSPI_DMA_SEL_BIT), sspi->dma_sel_base);
#endif
	}
#endif

	if (sspi->para->drq_tx && sspi->para->drq_rx)
	{
		ret_dma = hal_dma_chan_request(&sspi->dma_tx.chan);
		if (ret_dma == HAL_DMA_CHAN_STATUS_BUSY)
		{
			SPI_ERR(sspi, "failed to request dma tx channel\n");
			ret_spi = HAL_SPI_MASTER_ERROR;
			goto exit;
		}

		ret_dma = hal_dma_chan_request(&sspi->dma_rx.chan);
		if (ret_dma == HAL_DMA_CHAN_STATUS_BUSY)
		{
			SPI_ERR(sspi, "failed to request dma rx channel\n");
			ret_spi = HAL_SPI_MASTER_ERROR;
			goto exit_dma_rx;
		}

		SPI_INFO(sspi, "request dma channel tx(%d) and rx(%d)\n",
					sspi->dma_tx.chan->chan_count, sspi->dma_rx.chan->chan_count);
	}
	else
	{
		SPI_ERR(sspi, "dma drq rx_%d tx_%d error\n", sspi->para->drq_tx, sspi->para->drq_rx);
	}

	return ret_spi;

exit_dma_rx:
	hal_dma_chan_free(sspi->dma_rx.chan);
exit_dma_tx:
	hal_dma_chan_free(sspi->dma_tx.chan);
exit:
	return ret_spi;

#else
	return HAL_SPI_MASTER_ERROR;
#endif
}

static hal_spi_master_status_t sunxi_spi_release_dma(sunxi_spi_t *sspi)
{
	hal_spi_master_status_t ret = 0;
	if (!sspi->use_dma)
		goto exit;

#ifdef CONFIG_DRIVERS_DMA
	hal_dma_status_t ret_dma;
	hal_spi_master_status_t ret_spi;

	ret_dma = hal_dma_stop(sspi->dma_tx.chan);
	if (ret_dma)
	{
		SPI_ERR(sspi, "failed stop dma tx %d\n", ret_dma);
		ret_spi = HAL_SPI_MASTER_ERROR;
		goto exit;
	}

	ret_dma = hal_dma_stop(sspi->dma_rx.chan);
	if (ret_dma)
	{
		SPI_ERR(sspi, "failed stop dma rx %d\n", ret_dma);
		ret_spi = HAL_SPI_MASTER_ERROR;
		goto exit;
	}

	ret_dma = hal_dma_chan_free(sspi->dma_tx.chan);
	if (ret_dma)
	{
		SPI_ERR(sspi, "failed free dma tx channel %d\n", ret_dma);
		ret_spi = HAL_SPI_MASTER_ERROR;
		goto exit;
	}

	ret_dma = hal_dma_chan_free(sspi->dma_rx.chan);
	if (ret_dma)
	{
		SPI_ERR(sspi, "failed free dma rx channel %d\n", ret_dma);
		ret_spi = HAL_SPI_MASTER_ERROR;
		goto exit;
	}
#endif

exit:
	return ret;
}

static hal_spi_master_status_t sunxi_spi_request_irq(sunxi_spi_t *sspi)
{
	char irqname[32];

	snprintf(irqname, 32, "sspi%d", sspi->bus_num);
	if (hal_request_irq(sspi->irqnum, sunxi_spi_handler, irqname, sspi) < 0)
		return HAL_SPI_MASTER_ERROR;
	hal_enable_irq(sspi->irqnum);

	return HAL_SPI_MASTER_OK;
}

static void sunxi_spi_release_irq(sunxi_spi_t *sspi)
{
	hal_disable_irq(sspi->irqnum);
	hal_free_irq(sspi->irqnum);
}

static hal_spi_master_status_t sunxi_spi_hw_init(sunxi_spi_t *sspi)
{
	hal_spi_master_status_t ret = 0;

	if (sspi->config.clock_frequency < SUNXI_SPI_MIN_FREQUENCY || sspi->config.clock_frequency > SUNXI_SPI_MAX_FREQUENCY)
	{
		sspi->config.clock_frequency = SUNXI_SPI_MAX_FREQUENCY;
		SPI_ERR(sspi, "frequency no in range, use default value %d\n", sspi->config.clock_frequency);
	}

	if (sunxi_spi_clk_init(sspi, sspi->config.clock_frequency))
	{
		SPI_ERR(sspi, "init clk rate %ld failed\n", sspi->config.clock_frequency);
		ret = HAL_SPI_MASTER_ERROR;
		goto exit;
	}

	if (sunxi_spi_pinctrl_init(sspi))
	{
		SPI_ERR(sspi, "init pinctrl failed\n");
		ret = HAL_SPI_MASTER_ERROR;
		goto exit_clk;
	}

	switch (sspi->config.bus_mode)
	{
	case HAL_SPI_BUS_MASTER:
		sunxi_spi_enable_bus(sspi);
		sunxi_spi_set_master(sspi);
		sunxi_spi_set_dummy_type(sspi, false);
		sunxi_spi_ss_ctrl(sspi, false);
		sunxi_spi_enable_tp(sspi);
		sunxi_spi_ss_owner(sspi, sspi->config.cs_mode);
		sunxi_spi_set_delay_chain(sspi, sspi->config.bus_sample_mode, sspi->config.clock_frequency);
		break;
	case HAL_SPI_BUS_SLAVE:
		sunxi_spi_enable_bus(sspi);
		sunxi_spi_set_slave(sspi);
		sunxi_spi_disable_tp(sspi);
		sunxi_spi_set_delay_chain(sspi, sspi->config.bus_sample_mode, sspi->config.clock_frequency);
		break;
	case HAL_SPI_BUS_BIT:
		/* set SUNXI_SPI_GC_EN 0 to enter the SPI Bit-Aligned mode */
		sunxi_spi_disable_bus(sspi);
		sunxi_spi_bit_ss_owner(sspi, sspi->config.cs_mode);
		sunxi_spi_bit_sample_delay(sspi, sspi->config.clock_frequency);
		break;
	default:
		SPI_ERR(sspi, "init bus mode %d unsupport\n", sspi->config.bus_mode);
		ret = HAL_SPI_MASTER_INVALID_PARAMETER;
		goto exit;
	}

	/* reset fifo */
	sunxi_spi_reset_fifo(sspi);
	sunxi_spi_set_fifo_trig_level_rx(sspi, sspi->rx_triglevel);
	sunxi_spi_set_fifo_trig_level_tx(sspi, sspi->tx_triglevel);

	return ret;

exit_clk:
	sunxi_spi_clk_exit(sspi);
exit:
	return ret;
}

static hal_spi_master_status_t sunxi_spi_hw_exit(sunxi_spi_t *sspi)
{
	switch (sspi->config.bus_mode)
	{
	case HAL_SPI_BUS_MASTER:
		sunxi_spi_disable_bus(sspi);
		sunxi_spi_disable_tp(sspi);
		break;
	case HAL_SPI_BUS_SLAVE:
		sunxi_spi_disable_bus(sspi);
		sunxi_spi_set_master(sspi);
		sunxi_spi_disable_tp(sspi);
		break;
	case HAL_SPI_BUS_BIT:
		break;
	default:
		SPI_ERR(sspi, "exit bus mode %d unsupport\n", sspi->config.bus_mode);
	}

	sunxi_spi_clk_exit(sspi);

	return HAL_SPI_MASTER_OK;
}

#ifdef CONFIG_COMPONENTS_PM
static int sunxi_spi_dev_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	sunxi_spi_t *sspi = (sunxi_spi_t *)dev->data;

	hal_disable_irq(sspi->irqnum);

	hal_clock_disable(sspi->bus_clk);
	hal_clock_disable(sspi->mclk);

	SPI_DBG(sspi, "suspend success\n");

	return 0;
}

static int sunxi_spi_dev_resume(struct pm_device *dev, suspend_mode_t mode)
{
	sunxi_spi_t *sspi = (sunxi_spi_t *)dev->data;

	hal_clock_enable(sspi->bus_clk);
	hal_clock_enable(sspi->mclk);

	hal_enable_irq(sspi->irqnum);

	SPI_DBG(sspi, "resume success\n");

	return 0;
}

static struct pm_devops spi_dev_devops = {
	.suspend = sunxi_spi_dev_suspend,
	.resume = sunxi_spi_dev_resume,
};
#endif

hal_spi_master_status_t hal_spi_init(int port, hal_spi_master_config_t *cfg)
{
	sunxi_spi_t *sspi = sunxi_get_spi(port);
	hal_spi_master_status_t ret;
#ifdef CONFIG_COMPONENTS_PM
	char *devicename;
	char *wlname;
#endif

	if (!sspi)
		return HAL_SPI_MASTER_ERROR_PORT;

	if (!cfg)
		return HAL_SPI_MASTER_INVALID_PARAMETER;

	if (sspi->used != HAL_SPI_MASTER_IDLE)
	{
		SPI_ERR(sspi, "port %d is using, cannot init\n", port);
		return HAL_SPI_MASTER_ERROR_BUSY;
	}

	sspi->used = HAL_SPI_MASTER_USING;

#ifdef SIP_SPI0_PARAMS
	if (cfg->sip)
		sspi->para = &g_sunxi_spi_params_sip[port];
	else
#endif
		sspi->para = &g_sunxi_spi_params[port];

	sspi->base = sspi->para->reg_base;
	sspi->irqnum = sspi->para->irq_num;
	sspi->bus_num = port;
	sspi->mode_type = MODE_TYPE_NULL;
	sspi->rx_triglevel = sspi->para->rx_fifosize / 2;
	sspi->tx_triglevel = sspi->para->tx_fifosize / 2;
	memcpy(&sspi->config, cfg, sizeof(sspi->config));

	ret = sunxi_spi_request_dma(sspi);
	if (ret == HAL_SPI_MASTER_OK)
	{
		sspi->use_dma = true;
		sspi->align_dma_buf = hal_malloc_coherent(ALIGN_DMA_BUF_SIZE);
		if (!sspi->align_dma_buf)
		{
			SPI_ERR(sspi, "alloc dma coherent %d failed\n", ALIGN_DMA_BUF_SIZE);
			ret = HAL_SPI_MASTER_ERROR_NOMEM;
			goto exit_dma;
		}
	}
	else
	{
		SPI_ERR(sspi, "dma channel request failed\n");
		sspi->use_dma = false;
	}

	ret = sunxi_spi_request_irq(sspi);
	if (ret < 0)
	{
		SPI_ERR(sspi, "cannot request irq %d\n", ret);
		ret = HAL_SPI_MASTER_ERROR;
		goto exit_dma_buf;
	}

	ret = sunxi_spi_hw_init(sspi);
	if (ret < 0)
	{
		SPI_ERR(sspi, "hw init failed %d\n", ret);
		ret = HAL_SPI_MASTER_ERROR;
		goto exit_irq;
	}

	sspi->done = hal_sem_create(0);
	if (!sspi->done)
	{
		SPI_ERR(sspi, "create done failed\n");
		ret = HAL_SPI_MASTER_ERROR;
		goto exit_hw;
	}

	sspi->mutex = hal_mutex_create();
	if (!sspi->mutex)
	{
		SPI_ERR(sspi, "create mutex failed\n");
		ret = HAL_SPI_MASTER_ERROR;
		goto exit_sem;
	}

#ifdef CONFIG_COMPONENTS_PM
	wlname = hal_malloc(32);
	snprintf(wlname, 32, "sspi%d_wakelock", port);
	sspi->wl.name = wlname;
	sspi->wl.ref = 0;

	devicename = hal_malloc(32);
	snprintf(devicename, 32, "sspi%d_dev", port);
	sspi->pm.name = devicename;
	sspi->pm.ops = &spi_dev_devops;
	sspi->pm.data = sspi;

	pm_devops_register(&sspi->pm);
#endif

	return HAL_SPI_MASTER_OK;

exit_sem:
	hal_sem_delete(sspi->done);
exit_hw:
	sunxi_spi_hw_exit(sspi);
exit_irq:
	sunxi_spi_release_irq(sspi);
exit_dma_buf:
	hal_free_coherent(sspi->align_dma_buf);
exit_dma:
	sunxi_spi_release_dma(sspi);
	return ret;
}

hal_spi_master_status_t hal_spi_deinit(int port)
{
	sunxi_spi_t *sspi = sunxi_get_spi(port);

	if (!sspi)
		return HAL_SPI_MASTER_ERROR_PORT;

	if (sspi->used != HAL_SPI_MASTER_USING)
	{
		SPI_ERR(sspi, "port %d is un-used, cannot deinit\n", port);
		return HAL_SPI_MASTER_ERROR_BUSY;
	}

	sspi->used = HAL_SPI_MASTER_IDLE;

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&sspi->pm);
	hal_free((void *)sspi->wl.name);
	hal_free((void *)sspi->pm.name);
#endif

	hal_mutex_delete(sspi->mutex);
	hal_sem_delete(sspi->done);
	sunxi_spi_hw_exit(sspi);
	sunxi_spi_release_irq(sspi);
	hal_free_coherent(sspi->align_dma_buf);
	sunxi_spi_release_dma(sspi);

	memset(&sspi->config, 0, sizeof(sspi->config));

	return HAL_SPI_MASTER_OK;
}

static hal_spi_master_status_t hal_spi_xfer_one(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;

	if ((!t->tx_buf && !t->rx_buf) || (!t->tx_len && !t->rx_len))
	{
		SPI_ERR(sspi, "get tx/rx buffer or tx/rx len failed\n");
		return HAL_SPI_MASTER_INVALID_PARAMETER;
	}

	SPI_DBG(sspi, "tl=%lu rl=%lu, tsl=%lu\n", t->tx_len, t->rx_len, t->tx_single_len);

	ret = sunxi_spi_xfer_setup(sspi, t);
	if (ret < 0)
	{
		SPI_ERR(sspi, "failed to setup sspi xfer %d\n", ret);
		return ret;
	}

	switch (sspi->config.bus_mode)
	{
	case HAL_SPI_BUS_BIT:
		sunxi_spi_bit_disable_irq(sspi);
		sunxi_spi_bit_clr_irq_pending(sspi);
		sunxi_spi_bit_enable_irq(sspi);
		break;
	default:
		sunxi_spi_reset_fifo(sspi);
		sunxi_spi_set_fifo_trig_level_rx(sspi, sspi->rx_triglevel);
		sunxi_spi_set_fifo_trig_level_tx(sspi, sspi->tx_triglevel);

		sunxi_spi_disable_irq(sspi, SUNXI_SPI_INT_CTL_MASK);
		sunxi_spi_clr_irq_pending(sspi, SUNXI_SPI_INT_STA_MASK);
		sunxi_spi_disable_dma_irq(sspi, SUNXI_SPI_FIFO_CTL_DRQ_EN);
		sunxi_spi_enable_irq(sspi, SUNXI_SPI_INT_CTL_ERR);
	}

	ret = sunxi_spi_mode_check(sspi, t);
	if (ret < 0)
	{
		SPI_ERR(sspi, "mode check failed %d\n", ret);
		return ret;
	}

	sspi->transfer = t;
	sspi->result = SPI_XFER_READY;
	hal_sem_clear(sspi->done);

	switch (sspi->config.bus_mode)
	{
	case HAL_SPI_BUS_MASTER:
		ret = sunxi_spi_xfer_master(sspi, t);
		break;
	case HAL_SPI_BUS_SLAVE:
		ret = sunxi_spi_xfer_slave(sspi, t);
		break;
	case HAL_SPI_BUS_BIT:
		ret = sunxi_spi_xfer_bit(sspi, t);
		break;
	default:
		SPI_ERR(sspi, "xfer bus mode %d unsupport\n", sspi->config.bus_mode);
		ret = HAL_SPI_MASTER_INVALID_PARAMETER;
	}

	if (sspi->mode_type != MODE_TYPE_NULL)
		sspi->mode_type = MODE_TYPE_NULL;

	return ret;
}

hal_spi_master_status_t hal_spi_xfer(int port, hal_spi_master_transfer_t *t, int num)
{
	hal_spi_master_status_t ret = 0;;
	sunxi_spi_t *sspi = sunxi_get_spi(port);
	int i;

	if (!sspi)
		return HAL_SPI_MASTER_ERROR_PORT;

	if (sspi->used != HAL_SPI_MASTER_USING)
		return HAL_SPI_MASTER_ERROR;

	if (!t || !num)
	{
		SPI_ERR(sspi, "xfer argument error\n");
		return HAL_SPI_MASTER_INVALID_PARAMETER;
	}

#ifdef CONFIG_COMPONENTS_PM
	pm_wakelocks_acquire(&sspi->wl, PM_WL_TYPE_WAIT_INC, OS_WAIT_FOREVER);
#endif

	hal_mutex_lock(sspi->mutex);

	sunxi_spi_set_cs(sspi, false);

	for (i = 0; i < num; i++)
	{
		ret = hal_spi_xfer_one(sspi, &t[i]);
		if (ret < 0)
			SPI_ERR(sspi, "xfer transfer index %d failed %d\n", i, ret);
	}

	sunxi_spi_set_cs(sspi, true);

	hal_mutex_unlock(sspi->mutex);

#ifdef CONFIG_COMPONENTS_PM
	pm_wakelocks_release(&sspi->wl);
#endif

	return ret;
}

hal_spi_master_status_t hal_spi_write(int port, const void *buf, uint32_t size)
{
	sunxi_spi_t *sspi = sunxi_get_spi(port);
	hal_spi_master_status_t ret;
	hal_spi_master_transfer_t tr = {
		.tx_buf = (uint8_t *)buf,
		.tx_len = size,
		.tx_single_len = size,
		.tx_nbits = SPI_NBITS_SINGLE,
	};

	if (!sspi)
		return HAL_SPI_MASTER_ERROR_PORT;

	if (sspi->used != HAL_SPI_MASTER_USING)
		return HAL_SPI_MASTER_ERROR;

	SPI_DBG(sspi, "write data, len is %ld \n", size);
	ret = hal_spi_xfer(port, &tr, 1);
	return ret;
}

hal_spi_master_status_t hal_spi_read(int port, void *buf, uint32_t size)
{
	sunxi_spi_t *sspi = sunxi_get_spi(port);
	hal_spi_master_status_t ret;
	hal_spi_master_transfer_t tr = {
		.rx_buf = (uint8_t *)buf,
		.rx_len = size,
		.tx_single_len = size,
		.rx_nbits = SPI_NBITS_SINGLE,
	};

	if (!sspi)
		return HAL_SPI_MASTER_ERROR_PORT;

	if (sspi->used != HAL_SPI_MASTER_USING)
		return HAL_SPI_MASTER_ERROR;

	SPI_DBG(sspi, "read data, len is %ld \n", size);
	ret = hal_spi_xfer(port, &tr, 1);
	return ret;
}

hal_spi_master_status_t hal_spi_slave_abort(int port)
{
	sunxi_spi_t *sspi = sunxi_get_spi(port);

	if (!sspi)
		return HAL_SPI_MASTER_ERROR_PORT;

	sspi->slave_aborted = true;
	hal_sem_post(sspi->done);

	return HAL_SPI_MASTER_OK;
}

#ifdef CONFIG_DRIVERS_SPI_PANIC_TRANSFER
static hal_spi_master_status_t sunxi_spi_xfer_atomic(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;
	u32 status = 0, enable = 0;
	u32 poll_time = POLL_TIMEOUT;

	switch (sspi->mode_type)
	{
	case SINGLE_HALF_DUPLEX_RX:
	case DUAL_HALF_DUPLEX_RX:
	case QUAD_HALF_DUPLEX_RX:
		SPI_DBG(sspi, "atomic xfer rx by cpu %d\n", t->rx_len);
		sunxi_spi_start_xfer(sspi);
		ret = sunxi_spi_cpu_rx(sspi, t);
		if (ret < 0)
			goto out;
		break;
	case SINGLE_HALF_DUPLEX_TX:
	case DUAL_HALF_DUPLEX_TX:
	case QUAD_HALF_DUPLEX_TX:
		SPI_DBG(sspi, "atomic xfer tx by cpu %d\n", t->tx_len);
		sunxi_spi_start_xfer(sspi);
		ret = sunxi_spi_cpu_tx(sspi, t);
		if (ret < 0)
			goto out;
		break;
	case SINGLE_FULL_DUPLEX_RX_TX:
		SPI_DBG(sspi, "atomic xfer rx & tx by cpu %d\n", t->tx_len);
		sunxi_spi_start_xfer(sspi);
		ret = sunxi_spi_cpu_tx_rx(sspi, t);
		if (ret < 0)
			goto out;
		break;
	case FULL_DUPLEX_TX_RX:
		SPI_DBG(sspi, "atomic xfer tx %d then rx %d by cpu\n", t->tx_len, t->rx_len);
		sunxi_spi_start_xfer(sspi);
		ret = sunxi_spi_cpu_tx(sspi, t);
		if (ret < 0)
			goto out;
		ret = sunxi_spi_cpu_rx(sspi, t);
		if (ret < 0)
			goto out;
		break;
	default:
		SPI_ERR(sspi, "unknown atomic transfer mode type %d\n", sspi->mode_type);
		ret = HAL_SPI_MASTER_INVALID_PARAMETER;
		goto out;
	}

	while (--poll_time)
	{
		enable = sunxi_spi_qry_irq_enable(sspi);
		status = sunxi_spi_qry_irq_pending(sspi);
		sunxi_spi_clr_irq_pending(sspi, status);
		SPI_DBG(sspi, "spi irq handler status(%"PRIx32") enable(%"PRIx32")\n", status, enable);

		if (status & SUNXI_SPI_INT_STA_TC)
		{
			SPI_DBG(sspi, "xfer atomic IRQ TC comes\n");
			break;
		}
		else if (status & SUNXI_SPI_INT_STA_ERR)
		{
			SPI_ERR(sspi, "xfer atomic IRQ status error %#x\n", status);
			sunxi_spi_soft_reset(sspi);
			sspi->result = -1;
			break;
		}
	}

	if (poll_time <= 0)
	{
		SPI_ERR(sspi, "xfer atomic wait for irq timeout!\n");
		ret = HAL_SPI_MASTER_ERROR_TIMEOUT;
		goto out;
	}
	else if (sspi->result < 0)
	{
		SPI_ERR(sspi, "xfer atomic failed %d\n", sspi->result);
		ret = HAL_SPI_MASTER_ERROR;
		goto out;
	}

out:
	return ret;
}

static hal_spi_master_status_t hal_spi_xfer_one_atomic(sunxi_spi_t *sspi, hal_spi_master_transfer_t *t)
{
	hal_spi_master_status_t ret = 0;

	if ((!t->tx_buf && !t->rx_buf) || (!t->tx_len && !t->rx_len))
	{
		SPI_ERR(sspi, "get tx/rx buffer or tx/rx len failed\n");
		return HAL_SPI_MASTER_INVALID_PARAMETER;
	}

	SPI_DBG(sspi, "tl=%lu rl=%lu, tsl=%lu\n", t->tx_len, t->rx_len, t->tx_single_len);

	ret = sunxi_spi_xfer_setup(sspi, t);
	if (ret < 0)
	{
		SPI_ERR(sspi, "failed to setup sspi xfer %d\n", ret);
		return ret;
	}

	sunxi_spi_reset_fifo(sspi);
	sunxi_spi_set_fifo_trig_level_rx(sspi, sspi->rx_triglevel);
	sunxi_spi_set_fifo_trig_level_tx(sspi, sspi->tx_triglevel);

	sunxi_spi_disable_irq(sspi, SUNXI_SPI_INT_CTL_MASK);
	sunxi_spi_clr_irq_pending(sspi, SUNXI_SPI_INT_STA_MASK);
	sunxi_spi_disable_dma_irq(sspi, SUNXI_SPI_FIFO_CTL_DRQ_EN);
	sunxi_spi_enable_irq(sspi, SUNXI_SPI_INT_CTL_ERR);

	ret = sunxi_spi_mode_check(sspi, t);
	if (ret < 0)
	{
		SPI_ERR(sspi, "mode check failed %d\n", ret);
		return ret;
	}

	sspi->transfer = t;
	sspi->result = SPI_XFER_READY;

	ret = sunxi_spi_xfer_atomic(sspi, t);

	if (sspi->mode_type != MODE_TYPE_NULL)
		sspi->mode_type = MODE_TYPE_NULL;

	return ret;
}

hal_spi_master_status_t hal_spi_panic_xfer(int port, hal_spi_master_transfer_t *t, int num)
{
	hal_spi_master_status_t ret = 0;;
	sunxi_spi_t *sspi = sunxi_get_spi(port);
	int i;

	if (!sspi)
		return HAL_SPI_MASTER_ERROR_PORT;

	if (sspi->used != HAL_SPI_MASTER_USING)
		return HAL_SPI_MASTER_ERROR;

	if (!t || !num)
	{
		SPI_ERR(sspi, "xfer argument error\n");
		return HAL_SPI_MASTER_INVALID_PARAMETER;
	}

	sunxi_spi_set_cs(sspi, false);

	for (i = 0; i < num; i++)
	{
		ret = hal_spi_xfer_one_atomic(sspi, &t[i]);
		if (ret < 0)
			SPI_ERR(sspi, "xfer atomic transfer index %d failed %d\n", i, ret);
	}

	sunxi_spi_set_cs(sspi, true);

	return ret;
}
#endif
