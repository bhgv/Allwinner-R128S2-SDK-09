# demo说明
1. 该demo仅支持mjpeg/XVID + PCM组合的avi格式，可以直接使用狸窝avi转换器转换；
2. XR875建议分辨率在480 * 320@15fps以下，R128建议分辨率在640 * 480@20fps以下
3. SPI显示屏的情况下，需要手动配置sysconfig.fex里面的宽和高，否则显示异常。

## SPI屏幕配置说明
```
CONFIG_DRIVERS_SPILCD=y
CONFIG_HAL_TEST_SPILCD=y
CONFIG_DRIVERS_DBI=y
选择合适的屏幕驱动
CONFIG_LCD_SUPPORT_KLD2844B=y
```
### 根据转换格式修改配置
1. turbojpeg不支持把mjpeg解码成RGB565，截止20230830，发现转码和显示的顺序是反过来的，所以xvid和mjpeg都是转码成BGRA，播放格式是ARGB。
2. SPI屏幕当前只支持ARGB8888和RGB565。
3. xvid支持转码成RGB565，如果需要转换格式为RGB565，需要修改sysconfi.fex的配置如下：
```
lcd_pixel_fmt       = 10
lcd_dbi_fmt         = 2
```
代码中也需要修改宏定义
```
#define XVID_COLOR_DEPTH 16                     //32/16
#define XVID_DEC_FORMAT  XVID_CSP_RGB565        //XVID_CSP_BGRA/XVID_CSP_RGB565
#define XVID_LCD_FORMAT  LCDFB_FORMAT_RGB_565   //LCDFB_FORMAT_ARGB_8888/LCDFB_FORMAT_RGB_565
```

## RGB/MCU屏幕配置说明
```
CONFIG_DISP2_SUNXI=y
如果是MCU屏，需要选择合适的屏幕驱动
CONFIG_LCD_SUPPORT_CL40BC1019_CPU=y
```
需要根据屏幕规格书，修改sysconfig.fex的配置，否则会出现播放卡顿等问题。举例如下：
```
;--------------------------------------------------
;MCU LCD
;--------------------------------------------------
[lcd0]
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
```
