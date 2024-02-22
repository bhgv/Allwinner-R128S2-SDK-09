#ifndef _wv_LOG_H_
#define _wv_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef enum {
    L_NONE = 0,
    L_ERROR,
    L_WARNING,
    L_INFO,
    L_DEBUG,
    L_MSGDUMP,
} wv_log_level_t;

extern wv_log_level_t log_print_level;

#define LOG_M(fmt, ...)                                                                     \
    {                                                                                              \
        if (log_print_level >= L_MSGDUMP)                                                              \
            printf("WV_M[%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);                   \
    }

#define LOG_D(fmt, ...)                                                                       \
    {                                                                                              \
        if (log_print_level >= L_DEBUG)                                                                \
            printf("WV_D[%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);                   \
    }

#define LOG_I(fmt, ...)                                                                        \
    {                                                                                              \
        if (log_print_level >= L_INFO)                                                                 \
            printf("WV_I[%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);                   \
    }

#define LOG_W(fmt, ...)                                                                       \
    {                                                                                              \
        if (log_print_level >= L_WARNING)                                                              \
            printf("WV_W[%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);                   \
    }

#define LOG_E(fmt, ...)                                                                       \
    {                                                                                              \
        if (log_print_level >= L_ERROR)                                                                \
            printf("WV_E[%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);                   \
    }

int wv_log_set_level(wv_log_level_t level);
wv_log_level_t wv_log_get_level(void);

#if __cplusplus
}; // extern "C"
#endif

#endif
