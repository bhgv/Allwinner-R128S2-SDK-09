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

#ifndef __SPI_SUN8IW19_H__
#define __SPI_SUN8IW19_H__

#define SUNXI_SPI0_PBASE         0X05010000 /* 4K */
#define SUNXI_SPI1_PBASE         0X05011000 /* 4K */
#define SUNXI_SPI2_PBASE         0X05012000 /* 4K */
#define SUNXI_IRQ_SPI0                 86
#define SUNXI_IRQ_SPI1                 87
#define SUNXI_IRQ_SPI2                 88

static struct sunxi_spi_params_t g_sunxi_spi_params[] = {
	/* SPI0 */
	{	.port = 0,
		.reg_base = SUNXI_SPI0_PBASE, .irq_num = SUNXI_IRQ_SPI0, .gpio_num = 6,
		.pclk_pll_type = HAL_SUNXI_CCU, .pclk_pll_id = HAL_CLK_PLL_PERI0,
		.pclk_hosc_type = HAL_SUNXI_CCU, .pclk_hosc_id = HAL_CLK_SRC_HOSC24M,
		.bus_type = HAL_SUNXI_CCU, .bus_id = 0,
		.mclk_type = HAL_SUNXI_CCU, .mclk_id = HAL_CLK_PERIPH_SPI0,
		.reset_type = HAL_SUNXI_RESET, .reset_id = 0,
		.gpio_clk = GPIO_PC0, .gpio_mosi = GPIO_PC2, .gpio_miso = GPIO_PC3,
		.gpio_cs0 = GPIO_PC1, .gpio_wp = GPIO_PC4, .gpio_hold = GPIO_PC5,
		.mux = 4, .driv_level = GPIO_DRIVING_LEVEL2,
	#ifdef CONFIG_DRIVERS_DMA
		.drq_tx = DRQDST_SPI0_TX, .drq_rx = DRQSRC_SPI0_RX,
	#endif
		.rx_fifosize = 64, .tx_fifosize = 64, .dma_force_fixed = true,
	},
	/* SPI1 */
	{	.port = 1,
		.reg_base = SUNXI_SPI1_PBASE, .irq_num = SUNXI_IRQ_SPI1, .gpio_num = 4,
		.pclk_pll_type = HAL_SUNXI_CCU, .pclk_pll_id = HAL_CLK_PLL_PERI0,
		.pclk_hosc_type = HAL_SUNXI_CCU, .pclk_hosc_id = HAL_CLK_SRC_HOSC24M,
		.bus_type = HAL_SUNXI_CCU, .bus_id = 0,
		.mclk_type = HAL_SUNXI_CCU, .mclk_id = HAL_CLK_PERIPH_SPI1,
		.reset_type = HAL_SUNXI_RESET, .reset_id = 0,
		.gpio_clk = GPIO_PH0, .gpio_mosi = GPIO_PH1, .gpio_miso = GPIO_PH2,
		.gpio_cs0 = GPIO_PH3, .gpio_wp = 0, .gpio_hold = 0,
		.mux = 4, .driv_level = GPIO_DRIVING_LEVEL2,
	#ifdef CONFIG_DRIVERS_DMA
		.drq_tx = DRQDST_SPI1_TX, .drq_rx = DRQSRC_SPI1_RX,
	#endif
		.rx_fifosize = 64, .tx_fifosize = 64, .dma_force_fixed = true,
	},
	/* SPI2 */
	{	.port = 2,
		.reg_base = SUNXI_SPI2_PBASE, .irq_num = SUNXI_IRQ_SPI2, .gpio_num = 4,
		.pclk_pll_type = HAL_SUNXI_CCU, .pclk_pll_id = HAL_CLK_PLL_PERI0,
		.pclk_hosc_type = HAL_SUNXI_CCU, .pclk_hosc_id = HAL_CLK_SRC_HOSC24M,
		.bus_type = HAL_SUNXI_CCU, .bus_id = 0,
		.mclk_type = HAL_SUNXI_CCU, .mclk_id = HAL_CLK_PERIPH_SPI2,
		.reset_type = HAL_SUNXI_RESET, .reset_id = 0,
		.gpio_clk = GPIO_PE18, .gpio_mosi = GPIO_PE19, .gpio_miso = GPIO_PE20,
		.gpio_cs0 = GPIO_PE21, .gpio_wp = 0, .gpio_hold = 0,
		.mux = 4, .driv_level = GPIO_DRIVING_LEVEL2,
	#ifdef CONFIG_DRIVERS_DMA
		.drq_tx = DRQDST_SPI2_TX, .drq_rx = DRQSRC_SPI2_RX,
	#endif
		.rx_fifosize = 64, .tx_fifosize = 64, .dma_force_fixed = true,
	},
};

static struct sunxi_spi_params_t g_sunxi_spi_params_sip[] = {
	{},
};

#define SPI_MAX_NUM ARRAY_SIZE(g_sunxi_spi_params)
#define SPI_SIP_MAX_NUM ARRAY_SIZE(g_sunxi_spi_params_sip)

#endif /*__SPI_SUN8IW19_H__  */
