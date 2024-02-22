/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY��S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS��SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY��S TECHNOLOGY.


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

#include <stdint.h>
#include <stdio.h>
#include <sunxi_hal_common.h>

#include "sunxi_hal_mbus.h"
#include "mbus.h"

hal_mbus_status_t hal_mbus_pmu_get_window(unsigned int *value)
{
	*value = mbus_get_window();
	return HAL_MBUS_STATUS_OK;
}

hal_mbus_status_t hal_mbus_pmu_get_value(enum mbus_pmu type, unsigned int *value)
{
	switch (type) {
		case MBUS_PMU_CPU :
			*value = mbus_get_cpu_ddr();
			break;
		case MBUS_PMU_GPU :
			*value = mbus_get_gpu_ddr();
			break;
		case MBUS_PMU_VE :
			*value = mbus_get_ve_ddr();
			break;
		case MBUS_PMU_CE :
			*value = mbus_get_ce_ddr();
			break;
		case MBUS_PMU_DE :
			*value = mbus_get_de_ddr();
			break;
		case MBUS_PMU_DISP :
			*value = mbus_get_de_ddr();
			break;
		case MBUS_PMU_OTH :
			*value = mbus_get_oth_ddr();
			break;
		case MBUS_PMU_TOTAL :
			*value = mbus_get_total_ddr();
			break;
		case MBUS_PMU_DI :
			*value = mbus_get_di_ddr();
			break;
		case MBUS_PMU_CSI :
			*value = mbus_get_csi_ddr();
			break;
		case MBUS_PMU_TVD :
			*value = mbus_get_tvd_ddr();
			break;
		case MBUS_PMU_G2D :
			*value = mbus_get_g2d_ddr();
			break;
		case MBUS_PMU_IOMMU :
			*value = mbus_get_iommu_ddr();
			break;
		case MBUS_PMU_RV_SYS :
			*value = mbus_get_rv_sys_ddr();
			break;
		case MBUS_PMU_DSP_SYS :
			*value = mbus_get_dsp_sys_ddr();
			break;
		case MBUS_PMU_DMA0:
			*value = mbus_get_dma0_ddr();
			break;
		case MBUS_PMU_DMA1:
			*value = mbus_get_dma1_ddr();
			break;
		case MBUS_PMU_MAHB:
			*value = mbus_get_mahb_ddr();
			break;
		default :
			mbus_err("not support mbus type, %d\n", type);
			return HAL_MBUS_STATUS_ERROR_PARAMETER;
	}

	return HAL_MBUS_STATUS_OK;
}

#ifdef CONFIG_DRIVERS_MBUS_SET_LIMIT
void hal_mbus_set_limit_value(enum hal_mbus_limit type, unsigned int value)
{
	uint32_t reg_val;
	uint32_t max_limit = 3199;

	if (value > max_limit) {
		mbus_err("set limlt error, Please ensure that the numerical range is between 0 and 3199!");
		return;
	}

	switch (type) {
		case MBUS_LIMIT_MAHB :
			reg_val = hal_readl(LIMIT(LIMIT_MAHB));
			reg_val &= ~LIMIT_MASK;
			reg_val |= 256*value/200 << 16;
			reg_val |= LIMIT_ENABLE;
			writel(reg_val, LIMIT(LIMIT_MAHB));
			break;
		case MBUS_LIMIT_DMA0 :
			reg_val = hal_readl(LIMIT(LIMIT_DMA0));
			reg_val &= ~LIMIT_MASK;
			reg_val |= 256*value/200 << 16;
			reg_val |= LIMIT_ENABLE;
			writel(reg_val, LIMIT(LIMIT_DMA0));
			break;
		case MBUS_LIMIT_DMA1 :
			reg_val = hal_readl(LIMIT(LIMIT_DMA1));
			reg_val &= ~LIMIT_MASK;
			reg_val |= 256*value/200 << 16;
			reg_val |= LIMIT_ENABLE;
			writel(reg_val, LIMIT(LIMIT_DMA1));
			break;
		case MBUS_LIMIT_CE :
			reg_val = hal_readl(LIMIT(LIMIT_CE));
			reg_val &= ~LIMIT_MASK;
			reg_val |= 256*value/200 << 16;
			reg_val |= LIMIT_ENABLE;
			writel(reg_val, LIMIT(LIMIT_CE));
			break;
		case MBUS_LIMIT_CPU_C :
			reg_val = hal_readl(LIMIT(LIMIT_CPU_C));
			reg_val &= ~LIMIT_MASK;
			reg_val |= 256*value/200 << 16;
			reg_val |= LIMIT_ENABLE;
			writel(reg_val, LIMIT(LIMIT_CPU_C));
			break;
		case MBUS_LIMIT_G2D :
			reg_val = hal_readl(LIMIT(LIMIT_G2D));
			reg_val &= ~LIMIT_MASK;
			reg_val |= 256*value/200 << 16;
			reg_val |= LIMIT_ENABLE;
			writel(reg_val, LIMIT(LIMIT_G2D));
			break;
		case MBUS_LIMIT_DE :
			reg_val = hal_readl(LIMIT(MBUS_LIMIT_DE));
			reg_val &= ~LIMIT_MASK;
			reg_val |= 256*value/200 << 16;
			reg_val |= LIMIT_ENABLE;
			writel(reg_val, LIMIT(MBUS_LIMIT_DE));
			break;
		case MBUS_LIMIT_DSP :
			reg_val = hal_readl(LIMIT(LIMIT_DSP));
			reg_val &= ~LIMIT_MASK;
			reg_val |= 256*value/200 << 16;
			reg_val |= LIMIT_ENABLE;
			writel(reg_val, LIMIT(LIMIT_DSP));
			break;
		case MBUS_LIMIT_RISCV :
			reg_val = hal_readl(LIMIT(LIMIT_RISCV));
			reg_val &= ~LIMIT_MASK;
			reg_val |= 256*value/200 << 16;
			reg_val |= LIMIT_ENABLE;
			writel(reg_val, LIMIT(LIMIT_RISCV));
			break;
		default :
			mbus_err("not support mbus set limlt: %d\n", type);
	}
}
#endif

hal_mbus_status_t hal_mbus_pmu_enable(void)
{
	mbus_pmu_enable();

	return HAL_MBUS_STATUS_OK;
}

