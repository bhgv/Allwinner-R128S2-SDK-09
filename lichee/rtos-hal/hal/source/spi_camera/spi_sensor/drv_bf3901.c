#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sunxi_hal_twi.h"

#include "hal_timer.h"
#include "hal_time.h"
#include "hal_log.h"

#include "hal/sdmmc/hal/hal_def.h"
#include "hal_atomic.h"
#include "hal_gpio.h"
#include "hal_mutex.h"

#include "spi_sensor.h"
#include "drv_bf3901.h"

#define ADDR_WIDTH        TWI_BITS_8
#define DATA_WIDTH        TWI_BITS_8

#define MCLK                   (24*1000*1000)
#define BF3901_SCCB_ID         0x6E
#define BF3901_CHIP_ID         0x3901
#define BF3A03_IIC_CLK_FREQ    10000

struct sensor_win_size hal_bf3901_win_sizes = {
    .width = 240,
    .height = 320,
    .hoffset = 0,
    .voffset = 0,
};

struct regval_list sensor_default_regs[] = {
// {0x12,0x80},
//MTK  SPI Mode  Only Y
//XCLK:24M  SCLK:24M  MCLK:3M
//行长：326  帧长：310
//max fps:30fps
// {0x0a,0x21},
// {0x10,0x21},
// {0x3d,0x22},
{0x11,0xb0},//b0
{0x1b,0x80},//SCLK:48M.MCLK:6M.//80
//{0x1b,0x06},//SCLK:48M.MCLK:6M
{0x6b,0x01},//0x6b[6]=1,打开CCIR656编码方式。
//{0x12,0x21},//0x12[5:4]=10,MTK 模式
{0x12,0x01},//0x12[5:4]=10,MTK 模式//01
{0x3a,0x00},
{0x15,0x12}, //0x02
//{0x15,0x10},

{0x62,0x81},//0x62[1:0]:2'01,SPI mode
{0x08,0xa0},
{0x06,0xf8},
{0x2b,0x20},
{0x27,0x97},
{0x17,0x01},//01
{0x18,0x79},//79
{0x19,0x00},
{0x1a,0xa0},
{0x03,0x00},
{0x13,0x00},
{0x01,0x0D},
{0x02,0x0D},
{0x9D,0x0C},
{0x8c,0x00},
{0x8d,0xff},
{0x33,0x10},
{0x34,0x1d},
{0x35,0x46},
{0x36,0x40},
{0x37,0xa4},
{0x38,0x7c},
{0x65,0x46},
{0x66,0x46},
{0x6e,0x20},
{0x9b,0xa4},
{0x9c,0x7c},
{0xbc,0x0c},
{0xbd,0xa4},
{0xbe,0x7c},
{0x20,0x09},
{0x09,0x03},
{0x72,0x2f},
{0x73,0x2f},
{0x74,0xa7},
{0x75,0x12},
{0x79,0x8d},
{0x7a,0x00},
{0x7e,0xfa},
{0x70,0x00},
{0x7c,0x84},
{0x7d,0xba},
{0x5b,0xc2},
{0x76,0x90},
{0x7b,0x55},
{0x71,0x46},
{0x77,0xdd},
{0x13,0x08},
{0x8a,0x10},
{0x8b,0x20},
{0x8e,0x21},
{0x8f,0x81},
{0x94,0x41},
{0x95,0x7e},
{0x96,0x7f},
{0x97,0xf3},
{0x24,0x58},
{0x97,0x48},
{0x25,0x08},
{0x94,0xb5},
{0x95,0xc0},
{0x80,0xf6},
{0x81,0xe0},
{0x82,0x1b},
{0x83,0x37},
{0x84,0x39},
{0x85,0x58},
{0x86,0x77},
{0x89,0x1d},
{0x8a,0x5c},
{0x8b,0x4c},
{0x39,0x98},
{0x3f,0x98},
{0x90,0xa0},
{0x91,0xe0},
{0x40,0x20},
{0x41,0x28},
{0x42,0x26},
{0x43,0x25},
{0x44,0x1f},
{0x45,0x1a},
{0x46,0x16},
{0x47,0x12},
{0x48,0x0f},
{0x49,0x0d},
{0x4b,0x0b},
{0x4c,0x0a},
{0x4e,0x08},
{0x4f,0x06},
{0x50,0x06},
{0x5a,0x56},
{0x51,0x1b},
{0x52,0x04},
{0x53,0x4a},
{0x54,0x26},
{0x57,0x75},
{0x58,0x2b},
{0x5a,0xd6},
{0x51,0x28},
{0x52,0x1e},
{0x53,0x9e},
{0x54,0x70},
{0x57,0x50},
{0x58,0x07},
{0x5c,0x28},
{0xb0,0xe0},
{0xb1,0xc0},
{0xb2,0xb0},
{0xb3,0x4f},
{0xb4,0x63},
{0xb4,0xe3},
{0xb1,0xf0},
{0xb2,0xa0},
{0x55,0x00},
{0x56,0x40},
{0x96,0x50},
{0x9a,0x30},
{0x6a,0x81},
{0x23,0x33},
{0xa0,0xd0},
{0xa1,0x31},
{0xa6,0x04},
{0xa2,0x0f},
{0xa3,0x2b},
{0xa4,0x0f},
{0xa5,0x2b},
{0xa7,0x9a},
{0xa8,0x1c},
{0xa9,0x11},
{0xaa,0x16},
{0xab,0x16},
{0xac,0x3c},
{0xad,0xf0},
{0xae,0x57},
{0xc6,0xaa},
{0xd2,0x78},
{0xd0,0xb4},
{0xd1,0x00},
{0xc8,0x10},
{0xc9,0x12},
{0xd3,0x09},
{0xd4,0x2a},
{0xee,0x4c},
{0x7e,0xfa},
{0x74,0xa7},
{0x78,0x4e},
{0x60,0xe7},
{0x61,0xc8},
{0x6d,0x70},
{0x1e,0x39},
{0x98,0x1a},
{0xF1,0x6B},
{0x69,0x20},
//{0x0b,0x01},
//{0xb9,0x80},
};

static sensor_private gsensor_private;
static sensor_private *sensor_getpriv()
{
    return &gsensor_private;
}

static HAL_Status bf3901_s_stream(void)
{
    sensor_private *priv = sensor_getpriv();
    uint16_t chip_id = 0;
    uint16_t val = 0;
    uint16_t i = 0;
    int cnt = 0;

    spi_sensor_read(priv, 0xfc, &val);
    chip_id |= (val << 8);
    spi_sensor_read(priv, 0xfd, &val);
    chip_id |= (val);

    if(chip_id != BF3901_CHIP_ID) {
        sensor_err("BF3901 get chip id wrong 0x%02x\n", chip_id);
        return HAL_ERROR;
    }

    spi_sensor_write_array(priv, sensor_default_regs, ARRAY_SIZE(sensor_default_regs));

    sensor_info("BF3901 Init Done \r\n");

    return HAL_OK;
}

static void sensor_power(struct sensor_power_cfg *cfg, unsigned int on)
{
    sensor_private *priv = sensor_getpriv();
    int ret;

    if (on) {
        ret = hal_gpio_pinmux_set_function(cfg->pwdn_pin, GPIO_MUXSEL_OUT);
        if (ret)
            sensor_err("hal_gpio_pinmux_set_function error!\n");

        ret = hal_gpio_set_driving_level(cfg->pwdn_pin, GPIO_DRIVING_LEVEL1);
        if (ret)
            sensor_err("hal_gpio_set_driving_level error!\n");

        ret = hal_gpio_set_pull(cfg->pwdn_pin, GPIO_PULL_DOWN_DISABLED);
        if (ret)
            sensor_err("hal_gpio_set_pull error!\n");

        hal_msleep(30);

        hal_gpio_set_data(cfg->pwdn_pin, GPIO_DATA_LOW);

        hal_msleep(30);

    } else {
        hal_gpio_set_data(cfg->pwdn_pin, GPIO_DATA_HIGH);

        spi_set_mclk(priv, 0);
        hal_msleep(100);
    }
}


HAL_Status hal_bf3901_init(struct sensor_config *cfg)
{
    sensor_private *priv = sensor_getpriv();

    priv->twi_port = cfg->twi_port;
    priv->addr_width = ADDR_WIDTH;
    priv->data_width = DATA_WIDTH;
    priv->slave_addr = cfg->twi_addr;

    spi_sensor_twi_init(priv->twi_port, priv->slave_addr);

    sensor_power(&cfg->pwcfg, 1);

    if (bf3901_s_stream() != HAL_OK) {
        sensor_err("BF3901  Init error!!\n");
        return HAL_ERROR;
    }

    hal_msleep(100);
    return HAL_OK;
}

HAL_Status hal_bf3901_deinit(struct sensor_config *cfg)
{
    sensor_private *priv = sensor_getpriv();

    sensor_power(&cfg->pwcfg, 0);
    spi_sensor_twi_exit(priv->twi_port);
    sensor_info("BF3901  Deinit\n");

    return HAL_OK;
}