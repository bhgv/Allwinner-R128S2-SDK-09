/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY��S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS��SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY��S TECHNOLOGY.
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

#ifndef __SPI_SUN55IW3_H__
#define __SPI_SUN55IW3_H__

#include <hal_clk.h>

#define SUNXI_SPI0_PBASE		(0x04025000ul) /* 4K */
#define SUNXI_SPI1_PBASE		(0x04026000ul) /* 4K */
#define SUNXI_SPI2_PBASE		(0x04027000ul) /* 4K */
#define SUNXI_RSPI_PBASE		(0x07092000ul) /* 4K */

#ifdef CONFIG_ARCH_RISCV
#define SUNXI_IRQ_SPI0		(MAKE_IRQn(92, 1))
#define SUNXI_IRQ_SPI1		(MAKE_IRQn(92, 2))
#define SUNXI_IRQ_SPI2		(MAKE_IRQn(92, 3))
#define SUNXI_IRQ_RSPI		(MAKE_IRQn(76, 0))
#elif defined(CONFIG_ARCH_DSP)
#define SUNXI_IRQ_SPI0		(MAKE_IRQn(40, 1)|RINTC_IRQ_MASK)
#define SUNXI_IRQ_SPI1		(MAKE_IRQn(40, 2)|RINTC_IRQ_MASK)
#define SUNXI_IRQ_SPI2		(MAKE_IRQn(40, 3)|RINTC_IRQ_MASK)
#define SUNXI_IRQ_RSPI		16
#endif

/* define for R_SPI select DSP_DMA/SYS_DMA */
#define RSPI_DMA_SEL_ADDR	0x07010000
#define RSPI_DMA_SEL_OFFSET	0x370
#define RSPI_DMA_SEL_BIT	4

__attribute__((__unused__)) static struct sunxi_spi_params_t g_sunxi_spi_params[] = {
	/* SPI0 */
	{	.port = 0,
		.reg_base = SUNXI_SPI0_PBASE, .irq_num = SUNXI_IRQ_SPI0, .gpio_num = 6,
		.pclk_pll_type = HAL_SUNXI_CCU, .pclk_pll_id = CLK_PLL_PERI0_300M,
		.pclk_hosc_type = HAL_SUNXI_FIXED_CCU, .pclk_hosc_id = CLK_SRC_HOSC24M,
		.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_SPI0,
		.mclk_type = HAL_SUNXI_CCU, .mclk_id = CLK_SPI0,
		.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_SPI0,
		.gpio_clk = GPIOC(12), .gpio_mosi = GPIOC(2), .gpio_miso = GPIOC(4),
		.gpio_cs0 = GPIOC(3), .gpio_wp = GPIOC(15), .gpio_hold = GPIOC(16),
		.mux = 4, .driv_level = GPIO_DRIVING_LEVEL2,
	#if defined(SUNXI_USED_DMA0)
		.drq_tx = DRQDST_SPI0_TX, .drq_rx = DRQSRC_SPI0_RX,
	#endif
		.rx_fifosize = 128, .tx_fifosize = 64, .dma_force_fixed = false,
	},
	/* SPI1 */
	{	.port = 1,
		.reg_base = SUNXI_SPI1_PBASE, .irq_num = SUNXI_IRQ_SPI1, .gpio_num = 4,
		.pclk_pll_type = HAL_SUNXI_CCU, .pclk_pll_id = CLK_PLL_PERI0_300M,
		.pclk_hosc_type = HAL_SUNXI_FIXED_CCU, .pclk_hosc_id = CLK_SRC_HOSC24M,
		.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_SPI1,
		.mclk_type = HAL_SUNXI_CCU, .mclk_id = CLK_SPI1,
		.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_SPI1,
		.gpio_clk = GPIOI(3), .gpio_mosi = GPIOI(4), .gpio_miso = GPIOI(5),
		.gpio_cs0 = GPIOI(2), .mux = 3, .driv_level = GPIO_DRIVING_LEVEL2,
	#if defined(SUNXI_USED_DMA0)
		.drq_tx = DRQDST_SPI1_TX, .drq_rx = DRQSRC_SPI1_RX,
	#endif
		.rx_fifosize = 128, .tx_fifosize = 64, .dma_force_fixed = false,
	},
	/* SPI2 */
	{	.port = 2,
		.reg_base = SUNXI_SPI2_PBASE, .irq_num = SUNXI_IRQ_SPI2, .gpio_num = 4,
		.pclk_pll_type = HAL_SUNXI_CCU, .pclk_pll_id = CLK_PLL_PERI0_300M,
		.pclk_hosc_type = HAL_SUNXI_FIXED_CCU, .pclk_hosc_id = CLK_SRC_HOSC24M,
		.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_SPI2,
		.mclk_type = HAL_SUNXI_CCU, .mclk_id = CLK_SPI2,
		.reset_type = HAL_SUNXI_RESET, .reset_id = RST_BUS_SPI2,
		.gpio_clk = GPIOB(1), .gpio_mosi = GPIOB(2), .gpio_miso = GPIOB(3),
		.gpio_cs0 = GPIOB(0), .mux = 3, .driv_level = GPIO_DRIVING_LEVEL2,
	#if defined(SUNXI_USED_DMA0)
		.drq_tx = DRQDST_SPI2_TX, .drq_rx = DRQSRC_SPI2_RX,
	#endif
		.rx_fifosize = 128, .tx_fifosize = 64, .dma_force_fixed = false,
	},
	/* R_SPI */
	{	.port = 3,
		.reg_base = SUNXI_RSPI_PBASE, .irq_num = SUNXI_IRQ_RSPI, .gpio_num = 4,
		.pclk_pll_type = HAL_SUNXI_CCU, .pclk_pll_id = CLK_PLL_PERI0_300M,
		.pclk_hosc_type = HAL_SUNXI_FIXED_CCU, .pclk_hosc_id = CLK_SRC_HOSC24M,
		.bus_type = HAL_SUNXI_R_CCU, .bus_id = CLK_BUS_R_SPI,
		.mclk_type = HAL_SUNXI_R_CCU, .mclk_id = CLK_R_SPI,
		.reset_type = HAL_SUNXI_R_RESET, .reset_id = RST_R_SPI,
		.gpio_clk = GPIOL(11), .gpio_mosi = GPIOL(12), .gpio_miso = GPIOL(13),
		.gpio_cs0 = GPIOL(10), .mux = 6, .driv_level = GPIO_DRIVING_LEVEL2,
	#if defined(SUNXI_USED_DMA0) || defined(SUNXI_USED_DMA1)
		.drq_tx = DRQDST_R_SPI_TX, .drq_rx = DRQSRC_R_SPI_RX,
		.rspi_dma_sel = true,	/* Do not modify NEVER!!! R_SPI can only use SYS_DMA/DSP_DMA under RV/DSP */
	#endif
		.rx_fifosize = 128, .tx_fifosize = 64, .dma_force_fixed = false,
	},
};

static struct sunxi_spi_params_t g_sunxi_spi_params_sip[] = {
	{},
};

#define SPI_MAX_NUM ARRAY_SIZE(g_sunxi_spi_params)
#define SPI_SIP_MAX_NUM ARRAY_SIZE(g_sunxi_spi_params_sip)

#endif /* __SPI_SUN55IW3_H__ */
