#include "turbojpeg.h"
#include "hal_lcd_fb.h"
#include "hal_cache.h"
#include "video/sunxi_display2.h"
#include "dec_display.h"
#include "xvid.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

display_context *g_dpi = NULL;

#ifdef CONFIG_DRIVERS_SPILCD
#define FB_NUM 1
/* Color Depth: 16 (RGB565), 24 (RGB888), 32 (ARGB8888) */
/* SPI only support RGB565 or ARGB8888*/
/* MJPEG NOT SUPPORT RGB565 */
#define MJPEG_COLOR_DEPTH 32
#define MJPEG_DEC_FORMAT  TJPF_BGRA
#define MJPEG_LCD_FORMAT  LCDFB_FORMAT_ARGB_8888

#define XVID_COLOR_DEPTH 32                     //16
#define XVID_DEC_FORMAT  XVID_CSP_BGRA          //XVID_CSP_BGRA//XVID_CSP_RGB565
#define XVID_LCD_FORMAT  LCDFB_FORMAT_ARGB_8888 //LCDFB_FORMAT_ARGB_8888//LCDFB_FORMAT_RGB_565
static struct fb_info g_fb_info;
#else
#define FB_NUM            2
/* Color Depth: 16 (RGB565), 24 (RGB888), 32 (ARGB8888) */
#define MJPEG_COLOR_DEPTH 24
#define LCD_ID            0
#define MJPEG_DEC_FORMAT  TJPF_BGR
#define MJPEG_LCD_FORMAT  DISP_FORMAT_RGB_888

#define XVID_COLOR_DEPTH 16
#define XVID_DEC_FORMAT  XVID_CSP_RGB565
#define XVID_LCD_FORMAT  DISP_FORMAT_RGB_565
extern int disp_ioctl(int cmd, void *arg);
#endif

/* for data dummy test*/
void V_write_buffer2file(char *filename, unsigned char *buffer, int size)
{
    FILE *fd = fopen(filename, "wb");
    if (NULL == fd) {
        return;
    }
    fwrite(buffer, 1, size, fd);
    fclose(fd);
}

#ifdef CONFIG_DRIVERS_SPILCD
static void spi_fb_init(void *buff_addr, unsigned width, unsigned height, unsigned format)
{
    g_fb_info.screen_base = buff_addr;
    g_fb_info.var.xres = width;
    g_fb_info.var.yres = height;
    g_fb_info.var.xoffset = 0;
    g_fb_info.var.yoffset = 0;
    g_fb_info.var.lcd_pixel_fmt = format;
    int bpp = 4; //default ARGB8888
    if (format == LCDFB_FORMAT_RGB_565) {
        bpp = 2;
    }
    g_fb_info.fix.line_length = width * bpp;
}
#else
static void bsp_rgb_display(void *buff_addr, unsigned width, unsigned height,
                            __disp_pixel_fmt_t format)
{
    struct disp_layer_config config;
    unsigned long arg[3];

    memset(&config, 0, sizeof(struct disp_layer_config));
    config.channel = 0;
    config.layer_id = 0;
    config.enable = 1;
    config.info.mode = LAYER_MODE_BUFFER;
    config.info.fb.addr[0] = (unsigned long long)buff_addr;
    config.info.fb.size[0].width = width;
    config.info.fb.size[0].height = height;
    config.info.fb.format = format;
    config.info.fb.crop.x = 0;
    config.info.fb.crop.y = 0;
    config.info.fb.crop.width = ((unsigned long)width) << 32;
    config.info.fb.crop.height = ((unsigned long)height) << 32;
    config.info.fb.flags = DISP_BF_NORMAL;
    config.info.fb.scan = DISP_SCAN_PROGRESSIVE;
    config.info.alpha_mode = 0;
    config.info.alpha_value = 0xff;
    config.info.screen_win.width = width;
    config.info.screen_win.height = height;
    config.info.id = 0;

    arg[0] = 0;
    arg[1] = (unsigned long)&config;
    arg[2] = 1;
    disp_ioctl(DISP_LAYER_SET_CONFIG, (void *)arg);
    /* 理论上，可以通过设置dclk的频率，用硬件控制帧率 */
    /* 但是由于代码架构原因，目前没有接口可以直接改这个帧率，所以先不用 */
    // disp_ioctl(DISP_WAIT_VSYNC, (void *)arg);
}

static void lcd_test(void)
{
    struct disp_color bk;
    unsigned long arg[3];
    bk.red = 0xff;
    bk.green = 0x00;
    bk.blue = 0x00;
    arg[0] = 0;
    arg[1] = (unsigned long)&bk;
    disp_ioctl(DISP_SET_BKCOLOR, (void *)arg);
}
#endif

/****************************************
 * 
 * MJPEG DECODE
 * 
****************************************/
static int dp_mjpeg_init(void *arg)
{
    display_context *dpi = arg;
    tjhandle tjhandle = tjInitDecompress();
    if (tjhandle == NULL) {
        return -1;
    }

    unsigned display_size = dpi->fp.Height * dpi->fp.Width * (MJPEG_COLOR_DEPTH >> 3);
    unsigned char *displaybuf = hal_malloc_coherent(display_size * FB_NUM);
    if (displaybuf == NULL) {
        tjDestroy(tjhandle);
        return -1;
    }
    memset(displaybuf, 0, (display_size * FB_NUM));
    hal_dcache_clean((unsigned long)displaybuf, (display_size * FB_NUM));

#ifdef CONFIG_DRIVERS_SPILCD
    spi_fb_init(displaybuf, dpi->fp.Width, dpi->fp.Height, MJPEG_LCD_FORMAT);
#endif

    dpi->handle = tjhandle;
    dpi->displaybuff = displaybuf;
    dpi->displaysize = display_size;
    return 0;
}

static int dp_mjpeg_deinit(void *arg)
{
    display_context *dpi = arg;

    tjDestroy(dpi->handle);
    dpi->handle = NULL;
    hal_free_coherent(dpi->displaybuff);
    dpi->displaybuff = NULL;
    return 0;
}

static unsigned char *dp_get_buff(void *arg)
{
    display_context *dpi = arg;
#ifdef CONFIG_DRIVERS_SPILCD
    return dpi->displaybuff;
#else
    static int display_fb;
    display_fb++;
    if (display_fb >= FB_NUM)
        display_fb = 0;
    return (dpi->displaybuff + dpi->displaysize * display_fb);
#endif
}

static int dp_mjpeg_dec(unsigned char *inbuff, int insize)
{
    if (g_dpi == NULL) {
        return -1;
    }
    unsigned char *display_buff = g_dpi->get_display_buff(g_dpi);
    tjDecompress2(g_dpi->handle, inbuff, insize, display_buff, g_dpi->fp.Width, 0, g_dpi->fp.Height,
                  MJPEG_DEC_FORMAT, TJFLAG_FASTUPSAMPLE);

#ifdef CONFIG_DRIVERS_SPILCD
    hal_dcache_clean((unsigned long)display_buff, g_dpi->displaysize);
    bsp_disp_lcd_set_layer(0, &g_fb_info);
#else
    hal_dcache_clean((unsigned long)(display_buff), g_dpi->displaysize);
    bsp_rgb_display(display_buff, g_dpi->fp.Width, g_dpi->fp.Height, MJPEG_LCD_FORMAT);
#endif
}

/****************************************
 * 
 * XVID DECODE
 * 
****************************************/
static int dp_xvid_init(void *arg)
{
    display_context *dpi = arg;
    xvid_gbl_init_t xvid_gbl_init;
    xvid_dec_create_t xvid_dec_create;
    xvid_gbl_info_t xvid_gbl_info;

    /* Reset the structure with zeros */
    memset(&xvid_gbl_init, 0, sizeof(xvid_gbl_init_t));
    memset(&xvid_dec_create, 0, sizeof(xvid_dec_create_t));
    memset(&xvid_gbl_info, 0, sizeof(xvid_gbl_info));
    /*------------------------------------------------------------------------
     * Xvid core initialization
     *----------------------------------------------------------------------*/
    xvid_gbl_info.version = XVID_VERSION;
    xvid_global(NULL, XVID_GBL_INFO, &xvid_gbl_info, NULL);

    if (xvid_gbl_info.build != NULL) {
        printf("xvidcore build version: %s\n", xvid_gbl_info.build);
    }
    /* Version */
    xvid_gbl_init.version = XVID_VERSION;
    xvid_gbl_init.cpu_flags = XVID_CPU_FORCE;
    xvid_gbl_init.debug = 0;
    xvid_global(NULL, 0, &xvid_gbl_init, NULL);

    /*------------------------------------------------------------------------
     * Xvid decoder initialization
     *----------------------------------------------------------------------*/

    /* Version */
    xvid_dec_create.version = XVID_VERSION;

    /*
     * Image dimensions -- set to 0, xvidcore will resize when ever it is
     * needed
     */
    xvid_dec_create.width = dpi->fp.Width;
    xvid_dec_create.height = dpi->fp.Height;

    xvid_dec_create.num_threads = xvid_gbl_info.num_threads;
    printf("thread num %d\n", xvid_dec_create.num_threads);
    if (xvid_decore(NULL, XVID_DEC_CREATE, &xvid_dec_create, NULL) < 0) {
        return -1;
    }

    unsigned display_size = dpi->fp.Height * dpi->fp.Width * (XVID_COLOR_DEPTH >> 3);
    unsigned char *displaybuf = hal_malloc_coherent(display_size * FB_NUM);
    if (displaybuf == NULL) {
        xvid_decore(xvid_dec_create.handle, XVID_DEC_DESTROY, NULL, NULL);
        return -1;
    }
    memset(displaybuf, 0, (display_size * FB_NUM));
    hal_dcache_clean((unsigned long)displaybuf, (display_size * FB_NUM));

#ifdef CONFIG_DRIVERS_SPILCD
    spi_fb_init(displaybuf, dpi->fp.Width, dpi->fp.Height, XVID_LCD_FORMAT);
#endif

    dpi->handle = xvid_dec_create.handle;
    dpi->displaybuff = displaybuf;
    dpi->displaysize = display_size;
    return 0;
}

static int dp_xvid_deinit(void *arg)
{
    display_context *dpi = arg;

    xvid_decore(dpi->handle, XVID_DEC_DESTROY, NULL, NULL);
    dpi->handle = NULL;
    hal_free_coherent(dpi->displaybuff);
    dpi->displaybuff = NULL;
    return 0;
}

/* decode one frame  */
static int dec_xvid_main(unsigned char *istream, unsigned char *ostream, int istream_size,
                         xvid_dec_stats_t *xvid_dec_stats)
{
    xvid_dec_frame_t xvid_dec_frame;

    /* Reset all structures */
    memset(&xvid_dec_frame, 0, sizeof(xvid_dec_frame_t));
    memset(xvid_dec_stats, 0, sizeof(xvid_dec_stats_t));

    /* Set version */
    xvid_dec_frame.version = XVID_VERSION;
    xvid_dec_stats->version = XVID_VERSION;

    /* No general flags to set */
    /*
     * XVID_DEBLOCKY 去块滤波
     * XVID_DEBLOCKUV 色度块去滤波
     * XVID_DERINGY 边缘增强
     * XVID_DERINGUV 色度块边缘增强
    */
    xvid_dec_frame.general = 0;

    /* Input stream */
    xvid_dec_frame.bitstream = istream;
    xvid_dec_frame.length = istream_size;

    /* Output frame structure */
    xvid_dec_frame.output.plane[0] = ostream;
    xvid_dec_frame.output.stride[0] = g_dpi->fp.Width * (XVID_COLOR_DEPTH >> 3);
    xvid_dec_frame.output.csp = XVID_DEC_FORMAT;

    return xvid_decore(g_dpi->handle, XVID_DEC_DECODE, &xvid_dec_frame, xvid_dec_stats);
}

extern int hal_msleep(unsigned int msecs);
static int li_cnt = 0;
static int dp_xvid_dec(unsigned char *inbuff, int insize)
{
    if (g_dpi == NULL) {
        return -1;
    }

    unsigned char *mp4_ptr = inbuff;
    int useful_bytes = insize;
    int used_bytes = 0;
    xvid_dec_stats_t xvid_dec_stats;

    unsigned char *display_buff = g_dpi->get_display_buff(g_dpi);
    do {
        used_bytes = dec_xvid_main(mp4_ptr, display_buff, useful_bytes, &xvid_dec_stats);
        if (xvid_dec_stats.type == XVID_TYPE_VOL) {
            if (g_dpi->fp.Width * g_dpi->fp.Height <
                xvid_dec_stats.data.vol.width * xvid_dec_stats.data.vol.height) {
                printf("[error]ohhhh TODO\n");
            }
        }
        // printf("type : %d, used_bytes : %d\n", xvid_dec_stats.type, used_bytes);
        /* Update buffer pointers */
        if (used_bytes > 0) {
            mp4_ptr += used_bytes;
            useful_bytes -= used_bytes;
        }
    } while (xvid_dec_stats.type <= 0 && useful_bytes > 1);

#ifdef CONFIG_DRIVERS_SPILCD
    hal_dcache_clean((unsigned long)display_buff, g_dpi->displaysize);
    bsp_disp_lcd_set_layer(0, &g_fb_info);
#else
    hal_dcache_clean((unsigned long)(display_buff), g_dpi->displaysize);
    bsp_rgb_display(display_buff, g_dpi->fp.Width, g_dpi->fp.Height, XVID_LCD_FORMAT);
#endif
    /* 测试代码，保存数据到文件 */
    // unsigned char filename[50];
    // sprintf(filename, "sdmmc/li%d.rgb", li_cnt);
    // li_cnt++;
    // V_write_buffer2file(filename, display_buff,
    //                     g_dpi->fp.Width * g_dpi->fp.Height * (XVID_COLOR_DEPTH >> 3));
}

display_context *display_init(format_param *fp)
{
    g_dpi = malloc(sizeof(display_context));
    if (g_dpi == NULL) {
        return NULL;
    }
    memset(g_dpi, 0, sizeof(display_context));

    memcpy(&(g_dpi->fp), fp, sizeof(format_param));
    if (fp->type == FORMAT_MJPEG) {
        if (dp_mjpeg_init(g_dpi) < 0) {
            free(g_dpi);
            return NULL;
        }
        g_dpi->dec = dp_mjpeg_dec;
        g_dpi->get_display_buff = dp_get_buff;
    } else if (fp->type == FORMAT_XVID) {
        if (dp_xvid_init(g_dpi) < 0) {
            free(g_dpi);
            return NULL;
        }
        g_dpi->dec = dp_xvid_dec;
        g_dpi->get_display_buff = dp_get_buff;
    } else {
        free(g_dpi);
        return NULL;
    }
    return g_dpi;
}

int display_deinit(void)
{
    if (g_dpi->fp.type = FORMAT_MJPEG) {
        dp_mjpeg_deinit(g_dpi);
    } else if (g_dpi->fp.type = FORMAT_XVID) {
        dp_xvid_deinit(g_dpi);
    }
    free(g_dpi);
    g_dpi = NULL;
}