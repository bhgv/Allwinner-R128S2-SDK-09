#include <stdlib.h>
#include "hal_gpio.h"
#include "jpeg_reg.h"

unsigned int jpeg_ini_get_status(void)
{
	return JPEG->VE_INT_STA_REG;
}

void jpeg_int_clear_status(unsigned int state)
{
	JPEG->VE_INT_STA_REG = state;
}

void hal_ve_rst_ctl_release(void)
{
	JPEG->VE_RESET_REG = 0x0000000f;
}

void hal_ve_rst_ctl_reset(void)
{
	JPEG->VE_RESET_REG = 0x00000000;
}

void hal_jpeg_clk_en(void)
{
	JPEG->VE_MODE_REG = 0x000000c0;

#if defined(CONFIG_ARCH_SUN20IW2)
	if (CONFIG_ARCH_START_ADDRESS >= 0x0C000000) {
		JPEG->CSI_OUTPUT_MBUS_START_ADDR = CONFIG_ARCH_START_ADDRESS;
		JPEG->CSI_OUTPUT_MBUS_END_ADDR = CONFIG_ARCH_START_ADDRESS + CONFIG_ARCH_MEM_LENGTH;
		JPEG->CSI_OUTPUT_MBUS_EN = 1;
	}
#endif
}

void jpeg_enc_start(void)
{
	JPEG->VE_START_REG = 0x08;
}

void jpeg_mempart_take(void)
{
	HAL_SET_BIT(JPEG->VE_MODE_REG, 0x20000);
}


