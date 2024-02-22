/*
 * drivers/video/fbdev/sunxi/lcd_fb/panels/jlt35031c.c
 *
 * Copyright (c) 2007-2023 Allwinnertech Co., Ltd.
 * Author: zhangyuanjing <zhangyuanjingss@allwinnertech.com>
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
 * [lcd_fb0]
 * lcd_used            = 1
 * lcd_model_name      = "spilcd"
 * lcd_driver_name     = "jlt35031c"
 * lcd_x               = 320
 * lcd_y               = 480
 * lcd_width           = 49
 * lcd_height          = 74
 * lcd_data_speed      = 60
 * lcd_pwm_used        = 1
 * lcd_pwm_ch          = 1
 * lcd_pwm_freq        = 5000
 * lcd_pwm_pol         = 0
 * lcd_if              = 0
 * lcd_pixel_fmt       = 11
 * lcd_dbi_fmt         = 2
 * lcd_dbi_clk_mode    = 1
 * lcd_dbi_te          = 1
 * fb_buffer_num       = 2
 * lcd_dbi_if          = 4
 * lcd_rgb_order       = 0
 * lcd_fps             = 60
 * lcd_spi_bus_num     = 1
 * lcd_frm             = 2
 * lcd_gamma_en        = 1
 * lcd_backlight       = 100
 *
 * lcd_power_num       = 0
 * lcd_gpio_regu_num   = 0
 * lcd_bl_percent_num  = 0
 *
 * lcd_spi_dc_pin      = port:PA19<1><0><3><0>
 * ;RESET Pin
 * lcd_gpio_0          = port:PA20<1><0><2><0>
 */

#include "jlt35031c.h"

#define RESET(s, v) sunxi_lcd_gpio_set_value(s, 0, v)
#define power_en(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);
static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

static uint16_t lcd_dev_xoffset = 0;
static uint16_t lcd_dev_yoffset = 0;
static struct disp_panel_para info[LCD_FB_MAX];

static void address(unsigned int sel, int x, int y, int width, int height)
{
	sunxi_lcd_cmd_write(sel, 0x2a); /* Set coloum address */
	sunxi_lcd_para_write(sel, (((x + lcd_dev_xoffset) >> 8) & 0xff));
	sunxi_lcd_para_write(sel, ((x + lcd_dev_xoffset) & 0xff));
	sunxi_lcd_para_write(sel, ((width + lcd_dev_xoffset >> 8) & 0xff));
	sunxi_lcd_para_write(sel, ((width + lcd_dev_xoffset) & 0xff));

	sunxi_lcd_cmd_write(sel, 0x2b); /* Set row address */
	sunxi_lcd_para_write(sel, ((y + lcd_dev_yoffset >> 8) & 0xff));
	sunxi_lcd_para_write(sel, ((y + lcd_dev_yoffset) & 0xff));
	sunxi_lcd_para_write(sel, ((height + lcd_dev_yoffset >> 8) & 0xff));
	sunxi_lcd_para_write(sel, ((height + lcd_dev_yoffset) & 0xff));

	sunxi_lcd_cmd_write(sel, 0x2c);
}

static void LCD_panel_init(unsigned int sel)
{
	unsigned int rotate;

	if (bsp_disp_get_panel_info(sel, &info[sel])) {
		lcd_fb_wrn("get panel info fail!\n");
		return;
	}

	sunxi_lcd_cmd_write(sel, 0x11);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_delay_ms(120);

	sunxi_lcd_cmd_write(sel, 0xf0);
	sunxi_lcd_para_write(sel, 0xc3);

	sunxi_lcd_cmd_write(sel, 0xf0);
	sunxi_lcd_para_write(sel, 0x96);

	sunxi_lcd_cmd_write(sel, 0x36);
	sunxi_lcd_para_write(sel, 0x48);

	sunxi_lcd_cmd_write(sel, 0xb4);
	sunxi_lcd_para_write(sel, 0x01);

	sunxi_lcd_cmd_write(sel, 0xb7);
	sunxi_lcd_para_write(sel, 0xc6);

	sunxi_lcd_cmd_write(sel, 0xe8);
	sunxi_lcd_para_write(sel, 0x40);
	sunxi_lcd_cmd_write(sel, 0x8a);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_cmd_write(sel, 0x00);
	sunxi_lcd_para_write(sel, 0x29);
	sunxi_lcd_cmd_write(sel, 0x19);
	sunxi_lcd_para_write(sel, 0xa5);
	sunxi_lcd_cmd_write(sel, 0x33);

	sunxi_lcd_cmd_write(sel, 0xc1);
	sunxi_lcd_para_write(sel, 0x06);

	sunxi_lcd_cmd_write(sel, 0xc2);
	sunxi_lcd_para_write(sel, 0xa7);

	sunxi_lcd_cmd_write(sel, 0xc5);
	sunxi_lcd_para_write(sel, 0x18);

	sunxi_lcd_cmd_write(sel, 0xe0);
	sunxi_lcd_para_write(sel, 0xf0);
	sunxi_lcd_para_write(sel, 0x09);
	sunxi_lcd_para_write(sel, 0x0b);
	sunxi_lcd_para_write(sel, 0x06);
	sunxi_lcd_para_write(sel, 0x04);
	sunxi_lcd_para_write(sel, 0x15);
	sunxi_lcd_para_write(sel, 0x2f);
	sunxi_lcd_para_write(sel, 0x54);
	sunxi_lcd_para_write(sel, 0x42);
	sunxi_lcd_para_write(sel, 0x3c);
	sunxi_lcd_para_write(sel, 0x17);
	sunxi_lcd_para_write(sel, 0x14);
	sunxi_lcd_para_write(sel, 0x18);
	sunxi_lcd_para_write(sel, 0x1b);

	sunxi_lcd_cmd_write(sel, 0xE1);
	sunxi_lcd_para_write(sel, 0xf0);
	sunxi_lcd_para_write(sel, 0x09);
	sunxi_lcd_para_write(sel, 0x0b);
	sunxi_lcd_para_write(sel, 0x06);
	sunxi_lcd_para_write(sel, 0x04);
	sunxi_lcd_para_write(sel, 0x03);
	sunxi_lcd_para_write(sel, 0x2d);
	sunxi_lcd_para_write(sel, 0x43);
	sunxi_lcd_para_write(sel, 0x42);
	sunxi_lcd_para_write(sel, 0x3b);
	sunxi_lcd_para_write(sel, 0x16);
	sunxi_lcd_para_write(sel, 0x14);
	sunxi_lcd_para_write(sel, 0x17);
	sunxi_lcd_para_write(sel, 0x1b);

	sunxi_lcd_cmd_write(sel, 0xf0);
	sunxi_lcd_para_write(sel, 0x3c);

	sunxi_lcd_cmd_write(sel, 0xf0);
	sunxi_lcd_para_write(sel, 0x69);

	sunxi_lcd_cmd_write(sel, 0x3a);
	sunxi_lcd_para_write(sel, 0x55);
	sunxi_lcd_delay_ms(120);

	/* Display ON */
	sunxi_lcd_cmd_write(sel, 0x29);

	sunxi_lcd_cmd_write(sel, 0x36);
	/* LCD_DISPLAY_ROTATION_0 */
	sunxi_lcd_para_write(sel, 0x48);
	/* LCD_DISPLAY_ROTATION_90
	sunxi_lcd_para_write(sel, 0xe8); */
	/* LCD_DISPLAY_ROTATION_180
	sunxi_lcd_para_write(sel, 0x88); */
	/* LCD_DISPLAY_ROTATION_27
	sunxi_lcd_para_write(sel, 0x28); */

	/* set_screen_size */
	sunxi_lcd_cmd_write(sel, 0x2a);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_para_write(sel, 0x01);
	sunxi_lcd_para_write(sel, 0x3f);
	sunxi_lcd_cmd_write(sel, 0x2b);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_para_write(sel, 0x00);
	sunxi_lcd_para_write(sel, 0x01);
	sunxi_lcd_para_write(sel, 0xdf);

	if (info[sel].lcd_x < info[sel].lcd_y)
		address(sel, 0, 0, info[sel].lcd_x - 1, info[sel].lcd_y - 1);
	else
		address(sel, 0, 0, info[sel].lcd_y - 1, info[sel].lcd_x - 1);
}

static void LCD_panel_exit(unsigned int sel)
{
	sunxi_lcd_cmd_write(sel, 0x28);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_cmd_write(sel, 0x10);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_pin_cfg(sel, 0);
}

static s32 LCD_open_flow(u32 sel)
{
	lcd_fb_here;
	/* open lcd power, and delay 50ms */
	LCD_OPEN_FUNC(sel, LCD_power_on, 50);
	/* open lcd power, than delay 200ms */
	LCD_OPEN_FUNC(sel, LCD_panel_init, 200);

	LCD_OPEN_FUNC(sel, lcd_fb_black_screen, 50);
	/* open lcd backlight, and delay 0ms */
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	lcd_fb_here;
	/* close lcd backlight, and delay 0ms */
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 50);
	/* open lcd power, than delay 200ms */
	LCD_CLOSE_FUNC(sel, LCD_panel_exit, 10);
	/* close lcd power, and delay 500ms */
	LCD_CLOSE_FUNC(sel, LCD_power_off, 10);

	return 0;
}

static void LCD_power_on(u32 sel)
{
	/* config lcd_power pin to open lcd power0 */
	lcd_fb_here;
	power_en(sel, 1);

	sunxi_lcd_power_enable(sel, 0);

	sunxi_lcd_pin_cfg(sel, 1);
	RESET(sel, 1);
	sunxi_lcd_delay_ms(100);
	RESET(sel, 0);
	sunxi_lcd_delay_ms(100);
	RESET(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	lcd_fb_here;
	/* config lcd_power pin to close lcd power0 */
	sunxi_lcd_power_disable(sel, 0);
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	/* config lcd_bl_en pin to open lcd backlight */
	sunxi_lcd_backlight_enable(sel);
	lcd_fb_here;
}

static void LCD_bl_close(u32 sel)
{
	/* config lcd_bl_en pin to close lcd backlight */
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
	lcd_fb_here;
}

static int lcd_set_var(unsigned int sel, struct fb_info *p_info)
{
	return 0;
}

static int lcd_blank(unsigned int sel, unsigned int en)
{
	return 0;
}

static int lcd_set_addr_win(unsigned int sel, int x, int y, int width, int height)
{
	address(sel, x, y, width, height);
	return 0;
}

struct __lcd_panel jlt35031c_panel = {
    /* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "jlt35031c",
	.func = {
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.blank = lcd_blank,
		.set_var = lcd_set_var,
		.set_addr_win = lcd_set_addr_win,
	},
};