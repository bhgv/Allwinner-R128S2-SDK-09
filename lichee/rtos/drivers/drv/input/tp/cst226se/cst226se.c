#include <stdio.h>
#include <FreeRTOS.h>
#include <hal_timer.h>
#include <boot_reason.h>
#include <sunxi_hal_watchdog.h>

#include "hal_gpio.h"
#include "sunxi_hal_twi.h"
#include "aw_types.h"
#include "sunxi-input.h"
#include "input-event-codes.h"
#include "CST21680_F_WGJ10276_226SE.h"
#include "console.h"

#define SWAP_X_Y
#define SCREEN_MAX_X 480
#define SCREEN_MAX_Y 480

#define CST2XX_I2C_ADDR 0x5A
#define CST2XX_TWI TWI_MASTER_0
#define CST2XX_RESET_PIN GPIOA(29)
#define CST2XX_IRQ_PIN GPIOA(25)
#define INT_GPIO_MUX	0

#define PRESS_MAX 255
#define MAX_FINGERS 10
#define MAX_CONTACTS 10

#define DRIVER_SEND_CFG 1

#define DRIVER_NAME "touchscreen"
// #define CONFIG_TP_ESD_PROTECT
// #ifdef CONFIG_TP_ESD_PROTECT
// #define SWITCH_ESD_OFF 0
// #define SWITCH_ESD_ON 1
// static struct workqueue_struct* cst3xx_esd_workqueue;
// static void cst3xx_esd_switch(s32 on);
// #endif

int cst226se_xp = 0;
int cst226se_yp = 0;
int cst226se_wp = 0;

#define config_CTP_NORMAL_PRIORITY configMAX_PRIORITIES

struct ts_event {
	u16 x1;
	u16 y1;
	u16 x2;
	u16 y2;
	u16 x3;
	u16 y3;
	u16 x4;
	u16 y4;
	u16 x5;
	u16 y5;
	u16 pressure;
	u8 touch_point;
};

struct hyn_ts {
	u8 twi_id;
	int addr;
	int irq_pin;
	int rst_pin;
    int irq;
	osal_timer_t tp_timer;

	xSemaphoreHandle	irq_sem;
	SemaphoreHandle_t	mutex;

	struct sunxi_input_dev *input_dev;
	struct ts_event		event;
};
static int h_wake_pin = 0;
static struct hyn_ts* g_ts;
static unsigned char report_finger_flag = 0;

static void enable_irq(int irq)
{
	int ret = hal_gpio_irq_enable(irq);
	if (ret < 0) {
		printf("irq request err\n");
	}
}

static void disable_irq(int irq)
{
    hal_gpio_irq_disable(irq);
}

static int cst2xx_i2c_read(struct hyn_ts* ts, unsigned char* buf, int len)
{
    int ret = -1, retries = 0;
	struct twi_msg msgs;

	while (retries < 5) {
		msgs.addr = ts->addr;
		msgs.flags = TWI_M_RD;
		msgs.len = len;
		msgs.buf = buf;

		ret = hal_twi_xfer(ts->twi_id, &msgs, 1);
		if (ret < 0) {
			printf("[IIC]: i2c_transfer(read) error, ret=%d!!\n", ret);
            retries++;
		} else
            break;
	}
    return ret;
}

static int cst2xx_i2c_write(struct hyn_ts* ts, unsigned char* buf, int len)
{
    int ret = -1, retries = 0;
	struct twi_msg msgs;

	while (retries < 5) {
		msgs.addr = ts->addr;
		msgs.flags = 0;
		msgs.len = len;
		msgs.buf = buf;

		ret = hal_twi_xfer(ts->twi_id, &msgs, 1);
		if (ret < 0) {
			printf("[IIC]: i2c_transfer(write) error, ret=%d!!\n", ret);
            retries++;
		} else
            break;
	}
    return ret;
}

static int cst2xx_i2c_read_register(struct hyn_ts* ts, unsigned char* buf, int len)
{
    int ret = -1;

    ret = cst2xx_i2c_write(ts, buf, 2);

    ret = cst2xx_i2c_read(ts, buf, len);

    return ret;
}

static int cst2xx_test_i2c(struct hyn_ts* ts)
{
    u8 retry = 0, ret;
    u8 buf[2] = {0xD1, 0x06};

    while (retry++ < 5) {
        ret = cst2xx_i2c_write(ts, buf, 2);
        if (ret == TWI_STATUS_OK)
            return ret;
        vTaskDelay(2/portTICK_RATE_MS);
    }

    if (retry == 5)
        printf("hyn iic test error.ret:%d.\n", ret);

    return ret;
}

static inline void input_report_key_t(struct sunxi_input_dev *dev, unsigned int code, int value)
{
	sunxi_input_event(dev, EV_KEY, code, !!value);
}

static inline void input_report_abs_t(struct sunxi_input_dev *dev, unsigned int code, int value)
{
	sunxi_input_event(dev, EV_ABS, code, value);
}

static inline void input_sync_t(struct sunxi_input_dev *dev)
{
	sunxi_input_event(dev, EV_SYN, SYN_REPORT, 0);
}

static inline void input_mt_sync_t(struct sunxi_input_dev *dev)
{
	sunxi_input_event(dev, EV_SYN, SYN_MT_REPORT, 0);
}

static void hard_reset_chip(struct hyn_ts* ts, u16 ms)
{
    if (h_wake_pin != 0) {
        hal_gpio_set_data(ts->rst_pin, 0);
        vTaskDelay(10/portTICK_RATE_MS);
        hal_gpio_set_data(ts->rst_pin, 1);
    }
    vTaskDelay(ms/portTICK_RATE_MS);
}

// #ifdef CONFIG_TP_ESD_PROTECT

// static int esd_work_cycle = 1000;
// static struct delayed_work esd_check_work;
// static int esd_running;
// static struct mutex esd_lock;
// static void cst3xx_esd_check_func(struct work_struct*);
// static unsigned int pre_counter = 0;
// static unsigned int cur_counter = 0;
// static char i2c_lock_flag = 0;

// static void cst3xx_init_esd_protect(void)
// {
//     esd_work_cycle = 1 * HZ; /*HZ: clock ticks in 1 second generated by system*/
//     printf(" linc Clock ticks for an esd cycle: %d", esd_work_cycle);
//     INIT_DELAYED_WORK(&esd_check_work, cst3xx_esd_check_func);
//     mutex_init(&esd_lock);
// }

// static void cst3xx_esd_switch(s32 on)
// {
//     mutex_lock(&esd_lock);
//     if (SWITCH_ESD_ON == on) { /* switch on esd check */
//         if (!esd_running) {
//             esd_running = 1;
//             pre_counter = 0;
//             cur_counter = 0;
//             printf(" linc Esd protector started!");
//             queue_delayed_work(cst3xx_esd_workqueue, &esd_check_work, esd_work_cycle);
//         }
//     } else { /* switch off esd check */
//         if (esd_running) {
//             esd_running = 0;
//             printf(" linc Esd protector stopped!");
//             cancel_delayed_work(&esd_check_work);
//         }
//     }
//     mutex_unlock(&esd_lock);
// }

// static void cst3xx_esd_check_func(struct work_struct* work)
// {

//     int retry = 0;
//     int ret;
//     int check_sum;
//     static int count_test = 0;
//     unsigned char buf[8];

//     if (!esd_running) {
//         printf(" linc Esd protector suspended!");
//         return;
//     }

//     if (i2c_lock_flag != 0)
//         goto END;
//     else
//         i2c_lock_flag = 1;

//     buf[0] = 0xD1;
//     buf[1] = 0x09;
//     cst2xx_i2c_write(g_ts, buf, 2);

//     while (retry++ < 5) {
//         buf[0] = 0xD0;
//         buf[1] = 0x40;
//         ret = cst2xx_i2c_read_register(g_ts, buf, 6);
//         if (ret > 0) {
//             check_sum = buf[0] + buf[1] + buf[2] + buf[3] + 0xA5;
//             if (check_sum != ((buf[4] << 8) + buf[5])) {
//                 printf("[HYN]cst3xx_esd_check_func read check_sum error. check_sum = 0x%x.\n", check_sum);
//             } else {
//                 // printf("cst3xx_esd_check_func   succ -------\n");
//                 goto END;
//             }
//         }
//         mdelay(4);
//     }

//     if ((retry == 6) || (ret < 0)) {
//         goto hyn_esd_check_init;
//     }

//     cur_counter = buf[3] + (buf[2] << 8) + (buf[1] << 16) + (buf[0] << 24);
//     // g_ts->clk_tick_cnt = cur_counter;
//     if (((cur_counter < pre_counter) || ((cur_counter - pre_counter) < 12)) && (pre_counter > 100))
//     // if(((cur_counter<pre_counter) || ((cur_counter-pre_counter)<20)) && (pre_counter>100)) fpwang5 --
//     {
//         printf("[HYN]cst3xx_esd_check_func cur_counter is :0x%x. pre_counter is:0x%x.------\n", cur_counter,
//             pre_counter);
//         goto hyn_esd_check_init;
//     }
//     goto END;

// hyn_esd_check_init:
//     printf("[HYN]cst3xx_esd_check_func reset .\n");

//     if (report_finger_flag == 0) {
//         disable_irq(g_ts->irq);
//         // fpwang5 -- esd power
//         // cst3xx_poweron_ic(10);
//         hard_reset_chip(g_ts, 30);
//         enable_irq(g_ts->irq);
//     } else {
//         report_finger_flag--;
//     }
//     cur_counter = 0;
//     pre_counter = 0;

// END:
//     i2c_lock_flag = 0;
//     pre_counter = cur_counter;
//     mutex_lock(&esd_lock);
//     if (esd_running)
//         queue_delayed_work(cst3xx_esd_workqueue, &esd_check_work, esd_work_cycle);
//     else
//         printf(" linc Esd protector suspended!");
//     mutex_unlock(&esd_lock);

//     // printf("count_test === %d\n", count_test);
//     if (count_test++ > 3600 * 2) {
//         printf("count_test in+++++++++ %d\n", count_test);
//         count_test = 0;
//         disable_irq(g_ts->irq);
//         // fpwang5 -- esd power
//         // cst3xx_poweron_ic(10);
//         hard_reset_chip(g_ts, 30);
//         enable_irq(g_ts->irq);
//     }
// }

// #endif

static int cst2xx_enter_download_mode(struct hyn_ts* ts)
{
    int ret;
    int i;
    unsigned char buf[3];

    hard_reset_chip(ts, 10);

    for (i = 0; i < 30; i++) {

        buf[0] = 0xA0;
        buf[1] = 0x01;
        buf[2] = 0xAA;
        ret = cst2xx_i2c_write(ts, buf, 3);
        if (ret < 0) {
            vTaskDelay(1/portTICK_RATE_MS);
            continue;
        }

        vTaskDelay(1/portTICK_RATE_MS); // wait enter download mode

        buf[0] = 0xA0;
        buf[1] = 0x03; // check whether into program mode
        ret = cst2xx_i2c_read_register(ts, buf, 1);
        if (ret < 0) {
            vTaskDelay(1/portTICK_RATE_MS);
            continue;
        }

        if (buf[0] == 0x55) {
            break;
        }
    }

    if (buf[0] != 0x55) {
        printf("hyn reciev 0x55 failed.\n");
        return -1;
    } else {
        buf[0] = 0xA0;
        buf[1] = 0x06;
        buf[2] = 0x00;
        ret = cst2xx_i2c_write(ts, buf, 3);
    }

    return 0;
}

static int cst2xx_download_program(unsigned char* pdata, int len, struct hyn_ts* ts)
{
    int i, ret, j, retry;
    unsigned char* i2c_buf;
    unsigned char temp_buf[8];
    unsigned short eep_addr, iic_addr;
    int total_kbyte;

    i2c_buf = malloc(sizeof(unsigned char) * (512 + 2));
    if (i2c_buf == NULL) {
        printf("download malloc fail\n");
        return -1;
    }

    // make sure fwbin len is N*1K

    total_kbyte = len / 512;

    for (i = 0; i < total_kbyte; i++) {
        i2c_buf[0] = 0xA0;
        i2c_buf[1] = 0x14;
        eep_addr = i << 9; // i * 512
        i2c_buf[2] = eep_addr;
        i2c_buf[3] = eep_addr >> 8;
        ret = cst2xx_i2c_write(ts, i2c_buf, 4);
        if (ret < 0)
            goto error_out;

        memcpy(i2c_buf, pdata + eep_addr, 512);
        for (j = 0; j < 128; j++) {
            iic_addr = (j << 2);
            temp_buf[0] = (iic_addr + 0xA018) >> 8;
            temp_buf[1] = (iic_addr + 0xA018) & 0xFF;
            temp_buf[2] = i2c_buf[iic_addr + 0];
            temp_buf[3] = i2c_buf[iic_addr + 1];
            temp_buf[4] = i2c_buf[iic_addr + 2];
            temp_buf[5] = i2c_buf[iic_addr + 3];
            ret = cst2xx_i2c_write(ts, temp_buf, 6);
            if (ret < 0)
                goto error_out;
        }

        i2c_buf[0] = 0xA0;
        i2c_buf[1] = 0x04;
        i2c_buf[2] = 0xEE;
        ret = cst2xx_i2c_write(ts, i2c_buf, 3);
        if (ret < 0)
            goto error_out;

        vTaskDelay(600/portTICK_RATE_MS);

        for (retry = 0; retry < 10; retry++) {
            i2c_buf[0] = 0xA0;
            i2c_buf[1] = 0x05;
            ret = cst2xx_i2c_read_register(ts, i2c_buf, 1);
            if (ret < 0) {
                vTaskDelay(100/portTICK_RATE_MS);
                continue;
            } else {
                if (i2c_buf[0] != 0x55) {
                    vTaskDelay(100/portTICK_RATE_MS);
                    continue;
                } else {
                    break;
                }
            }
        }
        if (retry == 10) {
            goto error_out;
        }
    }

    i2c_buf[0] = 0xA0;
    i2c_buf[1] = 0x01;
    i2c_buf[2] = 0x00;
    ret = cst2xx_i2c_write(ts, i2c_buf, 3);
    if (ret < 0)
        goto error_out;

    i2c_buf[0] = 0xA0;
    i2c_buf[1] = 0x03;
    i2c_buf[2] = 0x00;
    ret = cst2xx_i2c_write(ts, i2c_buf, 3);

    if (i2c_buf != NULL) {
        free(i2c_buf);
        i2c_buf = NULL;
    }

    return 0;

error_out:
    if (i2c_buf != NULL) {
        free(i2c_buf);
        i2c_buf = NULL;
    }
    return -1;
}

static int cst2xx_read_checksum(struct hyn_ts* ts)
{
    int ret;
    int i;
    unsigned int checksum;
    unsigned int bin_checksum;
    unsigned char buf[4];
    const unsigned char* pData;

    for (i = 0; i < 10; i++) {
        buf[0] = 0xA0;
        buf[1] = 0x00;
        ret = cst2xx_i2c_read_register(ts, buf, 1);
        if (ret < 0) {
            vTaskDelay(2/portTICK_RATE_MS);
            continue;
        }

        if (buf[0] != 0)
            break;
        else
            vTaskDelay(2/portTICK_RATE_MS);
    }
    vTaskDelay(4/portTICK_RATE_MS);

    if (buf[0] == 0x01) {
        buf[0] = 0xA0;
        buf[1] = 0x08;
        ret = cst2xx_i2c_read_register(ts, buf, 4);
        if (ret < 0)
            return -1;

        // handle read data  --> checksum
        checksum = buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);

        pData = (unsigned char*)fwbin + 7680 - 4; // 7*1024 +512
        bin_checksum = pData[0] + (pData[1] << 8) + (pData[2] << 16) + (pData[3] << 24);

        printf("hyn checksum ic:0x%x. bin:0x%x------\n", checksum, bin_checksum);

        if (checksum != bin_checksum) {
            printf("hyn check sum error.\n");
            return -1;
        }
    } else {
        printf("hyn No checksum. buf[0]:%d.\n", buf[0]);
        return -1;
    }
    return 0;
}

static int cst2xx_update_firmware(struct hyn_ts* ts, unsigned char* pdata, int data_len)
{
    int ret;
    int retry;
    unsigned char buf[4];

    retry = 0;
// #ifdef CONFIG_TP_ESD_PROTECT
//     if (cst3xx_esd_workqueue != NULL)
//         cst3xx_esd_switch(SWITCH_ESD_OFF);
// #endif

start_flow:
    printf("hyn enter the update firmware.\n");
    disable_irq(ts->irq);
    vTaskDelay(20/portTICK_RATE_MS);
    ret = cst2xx_enter_download_mode(ts);
    if (ret < 0) {
        printf("hyn enter download mode failed.\n");
        goto fail_retry;
    }

    ret = cst2xx_download_program(pdata, data_len, ts);
    if (ret < 0) {
        printf("hyn download program failed.\n");
        goto fail_retry;
    }

    vTaskDelay(10/portTICK_RATE_MS);

    ret = cst2xx_read_checksum(ts);
    if (ret < 0) {
        printf("hyn check the updating checksum error.\n");
        return ret;
    } else {
        buf[0] = 0xA0; // exit program
        buf[1] = 0x06;
        buf[2] = 0xEE;
        ret = cst2xx_i2c_write(ts, buf, 3);
        if (ret < 0)
            goto fail_retry;
    }

    printf("hyn download firmware succesfully.\n");

    vTaskDelay(100/portTICK_RATE_MS);

    hard_reset_chip(ts, 30);

    enable_irq(ts->irq);

// #ifdef CONFIG_TP_ESD_PROTECT
//     if (cst3xx_esd_workqueue != NULL)
//         cst3xx_esd_switch(SWITCH_ESD_ON);
// #endif

    return 0;

fail_retry:
    if (retry < 4) {
        retry++;
        goto start_flow;
    }
    return -1;
}

static int cst2xx_boot_update_fw(struct hyn_ts* ts)
{
    return cst2xx_update_firmware(ts, fwbin, FW_BIN_SIZE);
}

static int cst2xx_check_code(struct hyn_ts* ts)
{
    int retry = 0;
    int ret;
    unsigned char buf[4];
    unsigned int fw_checksum, fw_version, fw_customer_id;
    unsigned int bin_checksum, bin_version;
    const unsigned char* pData;

    // buf[0] = 0xD0;
    // buf[1] = 0x4C;
    // while (retry++ < 3) {
    //     ret = cst2xx_i2c_read_register(ts, buf, 1);
    //     if (ret > 0)
    //         break;
    //     vTaskDelay(2/portTICK_RATE_MS);
    // }
    // if ((buf[0] == 226) || (buf[0] == 237) || (buf[0] == 240)) {
    //     // checksum
    //     return 0;
    // } else if (buf[0] == 168) {
        buf[0] = 0xD0;
        buf[1] = 0x49;
        while (retry++ < 3) {
            ret = cst2xx_i2c_read_register(ts, buf, 2);
            if (ret == 0)
                break;
            vTaskDelay(2/portTICK_RATE_MS);
        }
        fw_customer_id = (buf[0] << 8) + buf[1];

        printf("hyn fw_customer_id:%d. \r\n", fw_customer_id);

        // checksum
        buf[0] = 0xD2;
        buf[1] = 0x0C;
        while (retry++ < 5) {
            ret = cst2xx_i2c_read_register(ts, buf, 4);
            if (ret == 0)
                break;
            vTaskDelay(2/portTICK_RATE_MS);
        }

        fw_checksum = buf[3];
        fw_checksum <<= 8;
        fw_checksum |= buf[2];
        fw_checksum <<= 8;
        fw_checksum |= buf[1];
        fw_checksum <<= 8;
        fw_checksum |= buf[0];

        pData = (unsigned char*)fwbin + 7680 - 4; // 7*1024 +512
        bin_checksum = pData[0] + (pData[1] << 8) + (pData[2] << 16) + (pData[3] << 24);

        if (fw_checksum != bin_checksum) {
            printf("hyn checksum is different******bin_checksum:0x%x, fw_checksum:0x%x. \r\n", bin_checksum,
                fw_checksum);

            // chip version
            buf[0] = 0xD2;
            buf[1] = 0x08;
            while (retry++ < 5) {
                ret = cst2xx_i2c_read_register(ts, buf, 4);
                if (ret == 0)
                    break;
                vTaskDelay(2/portTICK_RATE_MS);
            }

            fw_version = buf[3];
            fw_version <<= 8;
            fw_version |= buf[2];
            fw_version <<= 8;
            fw_version |= buf[1];
            fw_version <<= 8;
            fw_version |= buf[0];

            pData = (unsigned char*)fwbin + 7680 - 8; // 7*1024 +512
            bin_version = pData[0] + (pData[1] << 8) + (pData[2] << 16) + (pData[3] << 24);

            printf("hyn bin_version is different******bin_version:0x%x, fw_version:0x%x. \r\n", bin_version,
                fw_version);

            if (bin_version >= fw_version) {
                ret = cst2xx_boot_update_fw(ts);
                if (ret < 0) {
                    printf("hyn update firmware fail  . \r\n");
                    hard_reset_chip(ts, 20);
                    return -2;
                } else
                    return 0;
            } else {
                printf("hyn bin_version is lower ,no need to update firmware.\n");
                return 0;
            }

        } else {
            printf("hyn checksum :0x%x is same,no need to update firmware.\n", fw_checksum);
            return 0;
        }

    // } else {
    //     printf("hyn check code error. buf[0]:%d.\n", buf[0]);
    //     ret = cst2xx_boot_update_fw(ts);
    //     if (ret < 0)
    //         return -2;
    //     else
    //         return 0;
    // }
}

static void cst2xx_touch_down(struct hyn_ts* ts, s32 id, s32 x, s32 y, s32 w)
{
#if 1
    input_report_abs_t(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_report_abs_t(ts->input_dev, ABS_MT_POSITION_X, y);
    input_report_abs_t(ts->input_dev, ABS_MT_POSITION_Y, x);

    input_report_abs_t(ts->input_dev, ABS_MT_PRESSURE, 15);
    input_report_abs_t(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
    input_report_key_t(ts->input_dev, BTN_TOUCH, 1);
    input_mt_sync_t(ts->input_dev);
    input_sync_t(ts->input_dev);
#else
    if(id != 0)
	return;
    cst226se_xp = y;
    cst226se_yp = x;
    cst226se_wp = 1;
#endif
    //printf("%d,%d\n", x, y);
}

static void cst2xx_touch_up(struct hyn_ts* ts, int id)
{
    if(id != 0)
	return;
#if 1
    input_report_key_t(ts->input_dev, BTN_TOUCH, 0);
    input_sync_t(ts->input_dev);
#else
    cst226se_wp = 0;
#endif
  //  printf("up\n");
}

static int report_flag = 0;
static void cst2xx_ts_worker(struct hyn_ts* ts)
{
    u8 buf[30];
    u8 i2c_buf[8];
    u8 key_status, key_id, finger_id, sw;
    int input_x = 0;
    int input_y = 0;
    int input_w = 0;
    int temp;
    u8 i, cnt_up, cnt_down;
    int ret, idx;
    int cnt, i2c_len, len_1, len_2;

// #ifdef CONFIG_TP_ESD_PROTECT
//     if (i2c_lock_flag != 0)
//         goto END;
//     else
//         i2c_lock_flag = 1;
// #endif

    buf[0] = 0xD0;
    buf[1] = 0x00;
    ret = cst2xx_i2c_read_register(ts, buf, 7);
    if (ret < 0) {
        printf("hyn iic read touch point data failed.\n");
        goto OUT_PROCESS;
    }

    if (buf[6] != 0xAB) {
        // printf("data is not valid..\r\n");
        goto OUT_PROCESS;
    }

    cnt = buf[5] & 0x7F;
    report_finger_flag = cnt;
    if (cnt > MAX_FINGERS)
        goto OUT_PROCESS;
    else if (cnt == 0)
        goto CLR_POINT;

    if (buf[5] == 0x80) {
        key_status = buf[0];
        key_id = buf[1];
        goto KEY_PROCESS;
    } else if (cnt == 0x01) {
        goto FINGER_PROCESS;
    } else {
        if ((buf[5] & 0x80) == 0x80) { // key
            i2c_len = (cnt - 1) * 5 + 3;
            len_1 = i2c_len;
            for (idx = 0; idx < i2c_len; idx += 6) {
                i2c_buf[0] = 0xD0;
                i2c_buf[1] = 0x07 + idx;

                if (len_1 >= 6) {
                    len_2 = 6;
                    len_1 -= 6;
                } else {
                    len_2 = len_1;
                    len_1 = 0;
                }

                ret = cst2xx_i2c_read_register(ts, i2c_buf, len_2);
                if (ret < 0)
                    goto OUT_PROCESS;

                for (i = 0; i < len_2; i++) {
                    buf[5 + idx + i] = i2c_buf[i];
                }
            }

            i2c_len += 5;
            key_status = buf[i2c_len - 3];
            key_id = buf[i2c_len - 2];
        } else {
            i2c_len = (cnt - 1) * 5 + 1;
            len_1 = i2c_len;

            for (idx = 0; idx < i2c_len; idx += 6) {
                i2c_buf[0] = 0xD0;
                i2c_buf[1] = 0x07 + idx;

                if (len_1 >= 6) {
                    len_2 = 6;
                    len_1 -= 6;
                } else {
                    len_2 = len_1;
                    len_1 = 0;
                }

                ret = cst2xx_i2c_read_register(ts, i2c_buf, len_2);
                if (ret < 0)
                    goto OUT_PROCESS;

                for (i = 0; i < len_2; i++) {
                    buf[5 + idx + i] = i2c_buf[i];
                }
            }
            i2c_len += 5;
        }

        if (buf[i2c_len - 1] != 0xAB) {
            goto OUT_PROCESS;
        }
    }

    if ((cnt > 0) && (key_status & 0x80)) // both key and point
    {
        if (report_flag == 0xA5)
            goto KEY_PROCESS;
    }

FINGER_PROCESS:

    i2c_buf[0] = 0xD0;
    i2c_buf[1] = 0x00;
    i2c_buf[2] = 0xAB;
    ret = cst2xx_i2c_write(ts, i2c_buf, 3);
    if (ret < 0) {
        printf("hyn send read touch info ending failed.\r\n");
        hard_reset_chip(ts, 20);
    }

    idx = 0;
    cnt_up = 0;
    cnt_down = 0;
    for (i = 0; i < cnt; i++) {
        input_x = (unsigned int)((buf[idx + 1] << 4) | ((buf[idx + 3] >> 4) & 0x0F));
        input_y = (unsigned int)((buf[idx + 2] << 4) | (buf[idx + 3] & 0x0F));
        input_w = (unsigned int)(buf[idx + 4]);
        sw = (buf[idx] & 0x0F) >> 1;
        finger_id = (buf[idx] >> 4) & 0x0F;

        // printf("Point x:%d, y:%d, id:%d, sw:%d.\r\n", input_x, input_y, finger_id, sw);

        if (sw == 0x03) {
            cst2xx_touch_down(ts, finger_id, input_x, input_y, input_w);
            cnt_down++;
        } else {
            cnt_up++;
            cst2xx_touch_up(ts, finger_id);
        }
        idx += 5;
    }

    if (cnt_down == 0)
        report_flag = 0;
    else
        report_flag = 0xCA;

    // input_sync(ts->input);

    goto END;

KEY_PROCESS:
    i2c_buf[0] = 0xD0;
    i2c_buf[1] = 0x00;
    i2c_buf[2] = 0xAB;
    ret = cst2xx_i2c_write(ts, i2c_buf, 3);
    if (ret < 0) {
        printf("hyn send read touch info ending failed.\r\n");
    }

    // input_sync(ts->input);

    goto END;

CLR_POINT:

OUT_PROCESS:
    i2c_buf[0] = 0xD0;
    i2c_buf[1] = 0x00;
    i2c_buf[2] = 0xAB;
    ret = cst2xx_i2c_write(ts, i2c_buf, 3);
    if (ret < 0) {
        printf("send read touch info ending failed.\n");
        hard_reset_chip(ts, 20);
    }

END:
// #ifdef CONFIG_TP_ESD_PROTECT
//     i2c_lock_flag = 0;
// #endif

    enable_irq(ts->irq);
}

void tp_timer_func(void *parm)
{
	struct hyn_ts* ts = (struct hyn_ts* )parm;
	// cst226se_update_data(ts);
	/*osal_timer_delete(ts->tp_timer);*/
}

static void touch_event_handler(void *parm)
{
	printf("========enter touch_event_handler=====\n");
	struct hyn_ts* ts = (struct hyn_ts* )parm;

	unsigned long time_interval = MS_TO_OSTICK(100);
	ts->tp_timer = osal_timer_create("cst226se_timer", tp_timer_func,
					parm, time_interval,
					OSAL_TIMER_FLAG_PERIODIC);

	xSemaphoreTake(ts->mutex, portMAX_DELAY);

	while (1) {
		//wait irq wakeup.
		xSemaphoreTake(ts->irq_sem, portMAX_DELAY);

		cst2xx_ts_worker(ts);
		// osal_timer_control(ts->tp_timer, OSAL_TIMER_CTRL_SET_TIME, &time_interval);
	}

	xSemaphoreGive(ts->mutex);
}

static hal_irqreturn_t hyn_ts_irq(void *dev_id)
{
    struct hyn_ts* ts = (struct hyn_ts*)dev_id;

    disable_irq(ts->irq);

	BaseType_t sem_ret, taskwoken = pdFALSE;
	hal_irqreturn_t ret;

	// printf("irq\n");
	sem_ret = xSemaphoreGiveFromISR(ts->irq_sem, &taskwoken);
	if (sem_ret == pdTRUE) {
		ret = HAL_IRQ_OK;
		return ret;
	} else {
		printf("irq give sem err\n");
		ret = HAL_IRQ_ERR;
		return ret;
	}
}

static int hyn_ts_suspend(void)
{
//     struct hyn_ts* ts = dev_get_drvdata(dev);
//     int i, rc;
//     u8 buf[2];

//     printf("I'am in hyn_ts_suspend() start\n");

//     disable_irq_nosync(ts->irq);

//     if (h_wake_pin != 0) {
//         hal_gpio_set_data(ts->rst_pin, 0);
//     }
// #ifdef CONFIG_TP_ESD_PROTECT
//     if (cst3xx_esd_workqueue != NULL)
//         cst3xx_esd_switch(SWITCH_ESD_OFF);
// #endif

//     buf[0] = 0xD1;
//     buf[1] = 0x05;
//     rc = cst2xx_i2c_write(ts, buf, 2);

//     return 0;
}

static int hyn_ts_resume(void)
{
//     struct hyn_ts* ts = dev_get_drvdata(dev);
//     int i, rc;
//     u8 buf[2];

//     printf("I'am in hyn_ts_resume() start\n");

// #ifdef CONFIG_TP_ESD_PROTECT
//     if (cst3xx_esd_workqueue != NULL)
//         cst3xx_esd_switch(SWITCH_ESD_ON);
// #endif

//     hard_reset_chip(ts, 30);

//     rc = cst2xx_check_code(ts);
//     if (rc < 0) {
//         printf("hyn check code error.\n");
//         return rc;
//     }

//     buf[0] = 0xD1;
//     buf[1] = 0x06;
//     rc = cst2xx_i2c_write(ts, buf, 2);

//     vTaskDelay(100/portTICK_RATE_MS);
//     enable_irq(ts->irq);
//     return 0;
}

int cst226se_init(void)
{
    struct hyn_ts* ts;
    int rc;
    unsigned long irq_flags;
    struct sunxi_input_dev *input_dev;
    uint16_t addr = CST2XX_I2C_ADDR;
    
    printf("cst226se enter %s\n", __func__);

	hal_twi_init(CST2XX_TWI);

    g_ts = ts = malloc(sizeof(*ts));
    if (!ts)
        return -1;

    ts->twi_id = CST2XX_TWI;
    ts->addr = CST2XX_I2C_ADDR;
    ts->irq_pin = CST2XX_IRQ_PIN;
    ts->rst_pin = CST2XX_RESET_PIN;
	
    rc = hal_gpio_set_direction(ts->rst_pin, GPIO_DIRECTION_OUTPUT);
	if (rc < 0) {
		printf("reset gpio init err\n");
		return -1;
	}
    h_wake_pin = ts->rst_pin;
    hal_gpio_set_data(ts->rst_pin, 1);
    // vTaskDelay(20 / portTICK_RATE_MS);
    // hard_reset_chip(ts, 10);

	rc = hal_gpio_pinmux_set_function(ts->irq_pin, INT_GPIO_MUX);
	if (rc < 0) {
		printf("int gpio init err\n");
		return -1;
	}

	rc = hal_gpio_to_irq(ts->irq_pin, &ts->irq);
	if (rc < 0) {
		printf("get irq num err\n");
		return -1;
	}

    rc = cst2xx_test_i2c(ts);
    if (rc != TWI_STATUS_OK) {
        printf("hyn cst2xx test iic error.\n");
        return rc;
    }

    printf("rc 226se ====> %d\n", rc);

#if DRIVER_SEND_CFG
    vTaskDelay(50 / portTICK_RATE_MS);
    rc = cst2xx_check_code(ts);
    if (rc < 0) {
        printf("hyn check code error.\n");
        return rc;
    }
#endif

    input_dev = sunxi_input_allocate_device();
	if (NULL == input_dev) {
		printf("input dev alloc err\n");
		goto err_free_data;

	}
	input_dev->name = DRIVER_NAME;
	input_set_capability(input_dev, EV_ABS, ABS_MT_POSITION_X);
	input_set_capability(input_dev, EV_ABS, ABS_MT_POSITION_Y);
	input_set_capability(input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR);
	input_set_capability(input_dev, EV_ABS, ABS_MT_WIDTH_MAJOR);
	input_set_capability(input_dev, EV_ABS, ABS_MT_TRACKING_ID);
	input_set_capability(input_dev, EV_KEY, BTN_TOUCH);
	sunxi_input_register_device(input_dev);
	ts->input_dev = input_dev;

	ts->mutex = xSemaphoreCreateMutex();
	if (NULL == ts->mutex) {
		printf("mutex init err\n");
		goto err_free_data;
	}

	ts->irq_sem = xSemaphoreCreateBinary();
	if (NULL == ts->irq_sem) {
		printf("irq_sem init err\n");
		goto err_free_data;
	}

	rc = hal_gpio_to_irq(ts->irq_pin, &ts->irq);
	if (rc < 0) {
		printf("get irq num err\n");
		return -1;
	}

    if (ts->irq) {
        printf("irq = %d\n", ts->irq);
        rc = hal_gpio_irq_request(ts->irq, hyn_ts_irq, IRQ_TYPE_EDGE_RISING, ts);
        if (rc < 0) {
            printf("irq request err\n");
            goto err_free_data;
        }
        rc = hal_gpio_irq_enable(ts->irq);
        if (rc < 0) {
            printf("irq request err\n");
            goto error_req_irq_fail;
        }
        disable_irq(ts->irq);
    } else {
        printf("cst226se irq req fail\n");
        goto error_req_irq_fail;
    }

    portBASE_TYPE task_ret = xTaskCreate(touch_event_handler, "touch_event_handle_task", 
            1024, ts, config_CTP_NORMAL_PRIORITY, NULL);
	if (task_ret != pdPASS) {
		printf("input handler task start fail\n");
		goto err_free_data;

	}

    enable_irq(ts->irq);

// #ifdef CONFIG_TP_ESD_PROTECT
//     cst3xx_esd_workqueue = create_singlethread_workqueue("cst3xx_esd_workqueue");
//     if (cst3xx_esd_workqueue == NULL)
//         printf("linc cst3xxcreate cst2xx_esd_workqueue failed!");
//     else {
//         cst3xx_init_esd_protect();
//         cst3xx_esd_switch(SWITCH_ESD_ON);
//     }
// #endif

    printf("[CST2XX] End %s\n", __func__);

    return 0;

error_req_irq_fail:
    hal_gpio_irq_free(ts->irq);

err_free_data:

error_mutex_destroy:
    free(ts);
    return rc;
}

// void cmd_cst226se_test_input_evdev(void)
// {

// }

// void cmd_cst226se_test(int argc, char **argv)
// {

// }
// FINSH_FUNCTION_EXPORT_CMD(cmd_cst226se_test, tp_test, cst226se-test);
