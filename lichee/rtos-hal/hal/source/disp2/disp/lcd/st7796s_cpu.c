/*
 * drivers/video/fbdev/sunxi/disp2/disp/lcd/s2003t46/s2003t46g.c
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
**/
/*
lcd_used            = 1

lcd_driver_name     = "st7796s_cpu"
lcd_backlight       = 150
lcd_if              = 1
lcd_x               = 320
lcd_y               = 480
lcd_width           = 108
lcd_height          = 64
lcd_rb_swap         = 0
lcd_pwm_used        = 1
lcd_pwm_ch          = 4
lcd_pwm_freq        = 5000
lcd_pwm_pol         = 1
lcd_cpu_mode = 0
lcd_cpu_te = 0

;st7796
;lcd_cpu_if = 14
;lcd_dclk_freq = 24
;lcd_hbp             = 100
;lcd_ht              = 778
;lcd_hspw            = 50
;lcd_vbp             = 8
;lcd_vt              = 496
;lcd_vspw            = 4

lcd_cpu_if = 12
lcd_dclk_freq = 32
lcd_hbp             = 75
lcd_ht              = 1060
lcd_hspw            = 40
lcd_vbp             = 6
lcd_vt              = 490
lcd_vspw            = 2

;ili9488
;lcd_cpu_if = 12
;lcd_dclk_freq = 32
;lcd_hbp             = 100
;lcd_ht              = 320
;lcd_hspw            = 40
;lcd_vbp             = 4
;lcd_vt              = 480
;lcd_vspw            = 2

lcd_lvds_if         = 0
lcd_lvds_colordepth = 1
lcd_lvds_mode       = 0
lcd_frm             = 0
lcd_io_phase        = 0x0000
lcd_gamma_en        = 0
lcd_bright_curve_en = 0
lcd_cmap_en         = 0

deu_mode            = 0
lcdgamma4iep        = 22
smart_color         = 90

lcd_gpio_0               = port:PB3<1><0><3><0>
lcd_gpio_1               = port:PA12<1><0><3><0>
lcd_gpio_2               = port:PA1<8><0><3><0>
lcd_gpio_3               = port:PA2<8><0><3><0>
lcd_gpio_4               = port:PA3<8><0><3><0>
lcd_gpio_5               = port:PA4<8><0><3><0>
lcd_gpio_6               = port:PA5<8><0><3><0>
lcd_gpio_7               = port:PA8<8><0><3><0>
lcd_gpio_8               = port:PA10<8><0><3><0>
lcd_gpio_9               = port:PA11<8><0><3><0>
lcd_gpio_10              = port:PA6<7><0><3><0>
lcd_gpio_11              = port:PA7<7><0><3><0>
lcd_gpio_12              = port:PA9<7><0><3><0>
 */

#include "st7796s_cpu.h"

//#define CPU_TRI_MODE

#define DBG_INFO(format, args...) (printf("[ST7796S LCD INFO] LINE:%04d-->%s:"format, __LINE__, __func__, ##args))
#define DBG_ERR(format, args...) (printf("[ST7796S LCD ERR] LINE:%04d-->%s:"format, __LINE__, __func__, ##args))
#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 0, val)
#define lcd_cs(val)  sunxi_lcd_gpio_set_value(sel, 1, val)

static void lcd_panel_st7796s_init(u32 sel, struct disp_panel_para *info);
static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

static void LCD_cfg_panel_info(struct panel_extend_para *info)
{
#if 1
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
		//{input value, corrected value}
		{0, 0},     {15, 15},   {30, 30},   {45, 45},   {60, 60},
		{75, 75},   {90, 90},   {105, 105}, {120, 120}, {135, 135},
		{150, 150}, {165, 165}, {180, 180}, {195, 195}, {210, 210},
		{225, 225}, {240, 240}, {255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
		{
		{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
		{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
		{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
		{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
		{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
		},
	};

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value =
			    lcd_gamma_tbl[i][1] +
			    ((lcd_gamma_tbl[i + 1][1] - lcd_gamma_tbl[i][1]) *
			     j) /
				num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
			    (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) +
				   (lcd_gamma_tbl[items - 1][1] << 8) +
				   lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));
#endif
}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 120);
#ifdef CPU_TRI_MODE
	LCD_OPEN_FUNC(sel, LCD_panel_init, 100);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);
#else
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 100);
	LCD_OPEN_FUNC(sel, LCD_panel_init, 50);
#endif
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 20);
#ifdef CPU_TRI_MODE
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);
	LCD_CLOSE_FUNC(sel, LCD_panel_exit, 50);
#else
	LCD_CLOSE_FUNC(sel, LCD_panel_exit, 10);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);
#endif
	LCD_CLOSE_FUNC(sel, LCD_power_off, 0);

	return 0;
}

static void LCD_power_on(u32 sel)
{
	/*config lcd_power pin to open lcd power0 */
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	/*lcd_cs, active low */
	lcd_cs(1);
	sunxi_lcd_delay_ms(10);
	/*lcd_rst, active hight */
	panel_reset(1);
	sunxi_lcd_delay_ms(10);

	sunxi_lcd_pin_cfg(sel, 0);
	/*config lcd_power pin to close lcd power0 */
	sunxi_lcd_power_disable(sel, 0);
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	/*config lcd_bl_en pin to open lcd backlight */
	sunxi_lcd_backlight_enable(sel);
}

static void LCD_bl_close(u32 sel)
{
	/*config lcd_bl_en pin to close lcd backlight */
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
}

/*static int bootup_flag = 0;*/
static void LCD_panel_init(u32 sel)
{
	struct disp_panel_para *info =
	    malloc(sizeof(struct disp_panel_para));

	DBG_INFO("\n");
	bsp_disp_get_panel_info(sel, info);
	lcd_panel_st7796s_init(sel, info);

	disp_sys_free(info);
	return;
}

static void LCD_panel_exit(u32 sel)
{
	sunxi_lcd_cpu_write_index(0, 0x28);
	sunxi_lcd_cpu_write_index(0, 0x10);
}

static void address(unsigned int sel, int x, int y, int width, int height)
{
	sunxi_lcd_cpu_write_index(sel, 0x2B); /* Set row address */
	sunxi_lcd_cpu_write_data(sel, (y >> 8) & 0xff);
	sunxi_lcd_cpu_write_data(sel, y & 0xff);
	sunxi_lcd_cpu_write_data(sel, (height >> 8) & 0xff);
	sunxi_lcd_cpu_write_data(sel, height & 0xff);
	sunxi_lcd_cpu_write_index(sel, 0x2A); /* Set coloum address */
	sunxi_lcd_cpu_write_data(sel, (x >> 8) & 0xff);
	sunxi_lcd_cpu_write_data(sel, x & 0xff);
	sunxi_lcd_cpu_write_data(sel, (width >> 8) & 0xff);
	sunxi_lcd_cpu_write_data(sel, width & 0xff);
}

static void lcd_panel_st7796s_init(u32 sel, struct disp_panel_para *info)
{
	unsigned int rotate;
	DBG_INFO("\n");
	/*lcd_cs, active low */
	lcd_cs(0);
	sunxi_lcd_delay_ms(10);
	panel_reset(1);
	sunxi_lcd_delay_ms(20);
	panel_reset(0);
	sunxi_lcd_delay_ms(20);
	panel_reset(1);
	sunxi_lcd_delay_ms(20);

	sunxi_lcd_delay_ms(120);
	sunxi_lcd_cpu_write_index(sel, 0x11);
	sunxi_lcd_delay_ms(120);

	sunxi_lcd_cpu_write_index(sel, 0xF0);
	sunxi_lcd_cpu_write_data(sel, 0xC3);

	sunxi_lcd_cpu_write_index(sel, 0xF0);
	sunxi_lcd_cpu_write_data(sel, 0x96);

	sunxi_lcd_cpu_write_index(sel, 0x36);
	sunxi_lcd_cpu_write_data(sel, 0x48);

	sunxi_lcd_cpu_write_index(sel, 0x3A);
	if (info->lcd_cpu_if == 14)
		sunxi_lcd_cpu_write_data(sel, 0x55);
	else
		sunxi_lcd_cpu_write_data(sel, 0x66);

	sunxi_lcd_cpu_write_index(sel, 0xB4); /* Display Inversion Control */
	sunxi_lcd_cpu_write_data(sel, 0x01);  /* 1-dot */

	sunxi_lcd_cpu_write_index(sel, 0xB7);
	sunxi_lcd_cpu_write_data(sel, 0xC6);

	sunxi_lcd_cpu_write_index(sel, 0xE8);
	sunxi_lcd_cpu_write_data(sel, 0x40);
	sunxi_lcd_cpu_write_data(sel, 0x8A);
	sunxi_lcd_cpu_write_data(sel, 0x00);
	sunxi_lcd_cpu_write_data(sel, 0x00);
	sunxi_lcd_cpu_write_data(sel, 0x29);
	sunxi_lcd_cpu_write_data(sel, 0x19);
	sunxi_lcd_cpu_write_data(sel, 0xA5);
	sunxi_lcd_cpu_write_data(sel, 0x33);

	sunxi_lcd_cpu_write_index(sel, 0xC1);
	sunxi_lcd_cpu_write_data(sel, 0x06);

	sunxi_lcd_cpu_write_index(sel, 0xC2);
	sunxi_lcd_cpu_write_data(sel, 0xa7);

	sunxi_lcd_cpu_write_index(sel, 0xC5);
	sunxi_lcd_cpu_write_data(sel, 0x18);

	sunxi_lcd_cpu_write_index(sel, 0xE0);
	sunxi_lcd_cpu_write_data(sel, 0xF0);
	sunxi_lcd_cpu_write_data(sel, 0x09);
	sunxi_lcd_cpu_write_data(sel, 0x0B);
	sunxi_lcd_cpu_write_data(sel, 0x06);
	sunxi_lcd_cpu_write_data(sel, 0x04);
	sunxi_lcd_cpu_write_data(sel, 0x15);
	sunxi_lcd_cpu_write_data(sel, 0x2F);
	sunxi_lcd_cpu_write_data(sel, 0x54);
	sunxi_lcd_cpu_write_data(sel, 0x42);
	sunxi_lcd_cpu_write_data(sel, 0x3C);
	sunxi_lcd_cpu_write_data(sel, 0x17);
	sunxi_lcd_cpu_write_data(sel, 0x14);
	sunxi_lcd_cpu_write_data(sel, 0x18);
	sunxi_lcd_cpu_write_data(sel, 0x1B);

	sunxi_lcd_cpu_write_index(sel, 0xE1);
	sunxi_lcd_cpu_write_data(sel, 0xF0);
	sunxi_lcd_cpu_write_data(sel, 0x09);
	sunxi_lcd_cpu_write_data(sel, 0x0B);
	sunxi_lcd_cpu_write_data(sel, 0x06);
	sunxi_lcd_cpu_write_data(sel, 0x04);
	sunxi_lcd_cpu_write_data(sel, 0x03);
	sunxi_lcd_cpu_write_data(sel, 0x2D);
	sunxi_lcd_cpu_write_data(sel, 0x43);
	sunxi_lcd_cpu_write_data(sel, 0x42);
	sunxi_lcd_cpu_write_data(sel, 0x3B);
	sunxi_lcd_cpu_write_data(sel, 0x16);
	sunxi_lcd_cpu_write_data(sel, 0x14);
	sunxi_lcd_cpu_write_data(sel, 0x17);
	sunxi_lcd_cpu_write_data(sel, 0x1B);

	sunxi_lcd_cpu_write_index(sel, 0xF0);
	sunxi_lcd_cpu_write_data(sel, 0x3C);

	sunxi_lcd_cpu_write_index(sel, 0xF0);
	sunxi_lcd_cpu_write_data(sel, 0x69);
	sunxi_lcd_delay_ms(120);

#if defined(CPU_TRI_MODE)
	/* enable te, mode 0 */
	sunxi_lcd_cpu_write_index(0, 0x35);
	sunxi_lcd_cpu_write_data(0, 0x00);

	sunxi_lcd_cpu_write_index(0, 0x44);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x80);
#endif

	sunxi_lcd_cpu_write_index(sel, 0x29);

	sunxi_lcd_cpu_write_index(sel, 0x36);
	/*MY MX MV ML RGB MH 0 0*/
	if (info->lcd_x > info->lcd_y)
		rotate = 0x20;
	else
		rotate = 0x48;
	sunxi_lcd_cpu_write_data(sel, rotate |= 0x08); /*horizon scrren*/

	address(sel, 0, 0, info->lcd_x - 1, info->lcd_y - 1);
	sunxi_lcd_cpu_write_index(sel, 0x2c); /* Display ON */

	if (info->lcd_cpu_mode == LCD_CPU_AUTO_MODE)
		sunxi_lcd_cpu_set_auto_mode(sel);
}


/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
struct __lcd_panel st7796s_cpu_panel = {
	.name = "st7796s_cpu",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
	},
};
