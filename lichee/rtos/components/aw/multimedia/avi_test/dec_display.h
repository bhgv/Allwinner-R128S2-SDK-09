#ifndef __DISPLAY_H
#define __DISPLAY_H

enum dec_type {
    FORMAT_MJPEG,
    FORMAT_XVID
};

typedef struct _format_param {
    enum dec_type type;
    int Height;
    int Width;
} format_param;

typedef struct _display_context {
    void *handle;
    unsigned char *displaybuff;
    int displaysize;
    int (*dec)(unsigned char *inbuff, int insize);
    unsigned char *(*get_display_buff)(void *arg);
    format_param fp;
} display_context;

display_context *display_init(format_param *fp);
int display_deinit(void);
#endif