#include "console.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "hal_time.h"
#include "avi.h"
#include "hal_sem.h"
#include "hal_queue.h"
#include "sunxi_hal_timer.h"
#include <AudioSystem.h>
#include "aw-alsa-lib/common.h"
#include "hal_timer.h"
#include "dec_display.h"

/* 需要长时间播放的情况下开启这个宏
   注意开启这个宏后,audio线程的CPU占用率会提高
* */
// #define AUDIO_PLAY_BY_AUDIO_SYSTEM

#define AVI_VIDEO_BUF_SIZE 1024 * 60
#define TIMER_PER_FRAME SUNXI_TMR2

typedef struct _mail_context {
    unsigned char *buff;
    unsigned size;
} mail_context;

// avi var
static Avi_Info g_avix = { 0 };
// system var
static hal_thread_t t_play_handle = NULL;
static hal_thread_t t_display_handle = NULL;
static hal_thread_t t_audio_handle = NULL;
static hal_sem_t s_display_start = NULL;
static hal_sem_t s_audio_start = NULL;
static hal_queue_t q_display_data = NULL;
static hal_queue_t q_audio_data = NULL;
// other var
static char *g_video_patch = NULL;
static unsigned g_thread_quick = 0;

static void video_play(void *arg)
{
    FILE *VideoFile = NULL;
    unsigned char *framebuf = NULL;

    if (g_video_patch == NULL)
        goto video_play_exit;

    VideoFile = fopen(g_video_patch, "rb");
    if (VideoFile == NULL) {
        printf("%s open fail\n", g_video_patch);
        goto video_play_exit;
    } else {
        printf("...paly %s\n", g_video_patch);
    }

    framebuf = malloc(AVI_VIDEO_BUF_SIZE);
    if (framebuf == NULL)
        goto video_play_exit;
    memset(framebuf, 0, AVI_VIDEO_BUF_SIZE);
    fread(framebuf, AVI_VIDEO_BUF_SIZE, 1, VideoFile);
    if (avi_demuxer(&g_avix, framebuf, AVI_VIDEO_BUF_SIZE) < 0) {
        printf("avi parser failed\n");
        goto video_play_exit;
    }
    if ((g_avix.VideoType != AVI_FORMAT_MJPG) && (g_avix.VideoType != AVI_FORMAT_XVID)) {
        printf("only support MJPEG or XVID\n");
        goto video_play_exit;
    }

    int offset = avi_search_id(framebuf, AVI_VIDEO_BUF_SIZE, "movi"); //不可能返回异常值,不做判断
    fseek(VideoFile, (long)(offset + 4), SEEK_SET);
    free(framebuf);
    framebuf = NULL;
    // start video
    hal_sem_post(s_display_start);
    // start audio
    hal_sem_post(s_audio_start);

    int np = 0;
    moviInfo streamInfo;
    mail_context *video_mail = NULL;
    mail_context *audio_mail = NULL;
    while (np < g_avix.TotalFrame) {
        if (g_thread_quick == 1) {
            g_thread_quick = 0;
            goto video_play_finish;
        }
        fread(&streamInfo, sizeof(moviInfo), 1, VideoFile);
        avi_get_streaminfo(&g_avix, &streamInfo);

        /* 实测某些情况下，会用header来对齐 */
        if (g_avix.StreamSize == 0) {
            continue;
        }

        if (g_avix.StreamID == AVI_VIDS_FLAG) {
            video_mail = malloc(sizeof(mail_context));
            if (video_mail == NULL)
                goto video_play_finish;
            video_mail->size = g_avix.StreamSize;
            video_mail->buff = malloc(g_avix.StreamSize);
            if (video_mail->buff == NULL) {
                // maybe is zero or toooo big!
                free(video_mail);
                goto video_play_finish;
            }
            memset(video_mail->buff, 0, g_avix.StreamSize);
            fread(video_mail->buff, g_avix.StreamSize, 1, VideoFile);
            if (0 > hal_queue_send_wait(q_display_data, (void *)&video_mail, 500)) {
                printf("video send fail\n");
                goto video_play_finish;
            }
            video_mail = NULL;
            np++;
        } else if (g_avix.StreamID == AVI_AUDS_FLAG) {
            audio_mail = malloc(sizeof(mail_context));
            if (audio_mail == NULL)
                goto video_play_finish;
            audio_mail->size = g_avix.StreamSize;
            audio_mail->buff = malloc(g_avix.StreamSize);
            if (audio_mail->buff == NULL) {
                // maybe is zero or toooo big!
                free(audio_mail);
                goto video_play_finish;
            }
            memset(audio_mail->buff, 0, g_avix.StreamSize);
            fread(audio_mail->buff, g_avix.StreamSize, 1, VideoFile);
            if (0 > hal_queue_send_wait(q_audio_data, (void *)&audio_mail, 500)) {
                printf("audio send fail\n");
                goto video_play_finish;
            }
            audio_mail = NULL;
        } else {
            printf("unsupport frame ID : %#x\n", g_avix.StreamID);
            printf("maybe last, close\n");
            goto video_play_finish;
        }
    }
    int time_out_count = 0;
video_play_finish:
    printf("play close\n");
    video_mail = malloc(sizeof(mail_context));
    video_mail->buff = NULL;
    video_mail->size = 0;
    if (0 > hal_queue_send_wait(q_display_data, (void *)&video_mail, 500)) {
        printf("ative close display thread fail!\n");
    }

    audio_mail = malloc(sizeof(mail_context));
    audio_mail->buff = NULL;
    audio_mail->size = 0;
    if (0 > hal_queue_send_wait(q_audio_data, (void *)&audio_mail, 500)) {
        printf("ative close audio thread fail!\n");
    }

video_play_exit:
    printf("video_play exit\n");
    if (g_video_patch != NULL) {
        free(g_video_patch);
        g_video_patch = NULL;
    }

    if (VideoFile != NULL) {
        fclose(VideoFile);
    }

    if (framebuf != NULL) {
        free(framebuf);
    }

    time_out_count = 300;
    while (hal_is_queue_empty(q_display_data) != 1) {
        if (hal_is_queue_empty(q_display_data) == -1) {
            printf("somthing error happen!!!\n");
            break;
        }
        hal_msleep(10);
        if ((time_out_count--) <= 0) {
            // maybe close at first
            break;
        }
    }
    hal_queue_delete(q_display_data);
    q_display_data = NULL;

    hal_sem_delete(s_display_start);
    s_display_start = NULL;

    time_out_count = 300;
    while (hal_is_queue_empty(q_audio_data) != 1) {
        if (hal_is_queue_empty(q_audio_data) == -1) {
            printf("somthing error happen!!!\n");
            break;
        }
        hal_msleep(10);
        if ((time_out_count--) <= 0) {
            // maybe close at first
            break;
        }
    }
    hal_queue_delete(q_audio_data);
    q_audio_data = NULL;

    hal_sem_delete(s_audio_start);
    s_audio_start = NULL;

    hal_thread_stop(t_display_handle);
    t_display_handle = NULL;

    hal_thread_stop(t_audio_handle);
    t_audio_handle = NULL;

    hal_thread_stop(t_play_handle);
    t_play_handle = NULL;
}

static void timer_irq_video_frame(void *arg)
{
    hal_sem_t sem_h = (hal_sem_t)arg;
    hal_sem_post(sem_h);
}

static void dispaly_thread(void *arg)
{
    static unsigned g_fbindex = 0;
    /* 等待解码完成，获取播放参数 */
    hal_sem_wait(s_display_start);
    format_param fp;
    fp.Height = g_avix.Height;
    fp.Width = g_avix.Width;
    if (g_avix.VideoType == AVI_FORMAT_MJPG) {
        fp.type = FORMAT_MJPEG;
    } else if (g_avix.VideoType == AVI_FORMAT_XVID) {
        fp.type = FORMAT_XVID;
    } else {
        goto dispaly_thread_exit;
    }

    display_context *dpi = display_init(&fp);
    if (dpi == NULL) {
        goto dispaly_thread_exit;
    }

    osal_timer_t v_timer;
    v_timer = osal_timer_create("frame per", timer_irq_video_frame, s_display_start,
                                (g_avix.SecPerFrame / 1000),
                                (OSAL_TIMER_FLAG_SOFT_TIMER | OSAL_TIMER_FLAG_PERIODIC));
    osal_timer_start(v_timer);
    mail_context *video_mail = NULL;
    while (1) {
        if (0 > hal_queue_recv(q_display_data, (void *)&video_mail, 500))
            continue;
        if ((video_mail->buff == NULL) && (video_mail->size == 0)) {
            free(video_mail);
            break;
        }

        dpi->dec(video_mail->buff, video_mail->size);
        free(video_mail->buff);
        free(video_mail);
        video_mail = NULL;
        hal_sem_wait(s_display_start);
    }
    // hal_timer_uninit(TIMER_PER_FRAME);
    osal_timer_delete(v_timer);
dispaly_thread_exit:
    printf("dispaly_thread exit\n");
    if (dpi != NULL) {
        display_deinit();
        dpi = NULL;
    }

    while (1) {
        // 等待统一线程回收
        hal_msleep(10);
    }
}

#ifdef AUDIO_PLAY_BY_AUDIO_SYSTEM
static void audio_thread(void *arg)
{
    hal_sem_wait(s_audio_start);
    tAudioTrack *at = AudioTrackCreateWithStream("default", AUDIO_STREAM_MUSIC);
    if (at == NULL) {
        goto audio_thread_exit;
    }
    AudioTrackSetup(at, g_avix.SampleRate, g_avix.Channels, g_avix.Bits);
    // softvol_control_with_streamtype(AUDIO_STREAM_MUSIC, )
    mail_context *audio_mail = NULL;
    while (1) {
        if (0 > hal_queue_recv(q_audio_data, (void *)&audio_mail, 500))
            continue;
        if ((audio_mail->buff == NULL) && (audio_mail->size == 0)) {
            free(audio_mail);
            break;
        }
        AudioTrackWrite(at, (void *)audio_mail->buff, audio_mail->size);
        free(audio_mail->buff);
        free(audio_mail);
        audio_mail = NULL;
    }
audio_thread_exit:
    if (at != NULL) {
        AudioTrackDestroy(at);
    }

    while (1) {
        // 等待线程统一被回收
        hal_msleep(10);
    }
}
#else
static void audio_thread(void *arg)
{
    hal_sem_wait(s_audio_start);

    snd_pcm_t *a_pcm_handle = NULL;
    snd_pcm_hw_params_t *alsa_hwparams = NULL;
    snd_pcm_format_t pcm_format = SND_PCM_FORMAT_UNKNOWN;
    snd_pcm_uframes_t period_size = 0;
    snd_pcm_uframes_t buffer_size = 0;
    unsigned bytes_per_sample = 0;
    int ret = -1;

    ret = snd_pcm_open(&a_pcm_handle, "hw:audiocodecdac", SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0)
        goto audio_thread_exit;

    ret = snd_pcm_hw_params_malloc(&alsa_hwparams);
    if (ret < 0)
        goto audio_thread_exit;

    ret = snd_pcm_hw_params_any(a_pcm_handle, alsa_hwparams);
    if (ret < 0)
        goto audio_thread_exit;

    ret = snd_pcm_hw_params_set_access(a_pcm_handle, alsa_hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0)
        goto audio_thread_exit;
    /* 一般情况下，avi只支持小端 */
    if (g_avix.Bits == 8) {
        pcm_format = SND_PCM_FORMAT_S8;
    } else if (g_avix.Bits == 16) {
        pcm_format = SND_PCM_FORMAT_S16_LE;
    } else if (g_avix.Bits == 24) {
        pcm_format = SND_PCM_FORMAT_S24_LE;
    } else if (g_avix.Bits == 32) {
        pcm_format = SND_PCM_FORMAT_S32_LE;
    } else {
        goto audio_thread_exit;
    }
    ret = snd_pcm_hw_params_set_format(a_pcm_handle, alsa_hwparams, pcm_format);
    if (ret < 0)
        goto audio_thread_exit;

    ret = snd_pcm_hw_params_set_channels(a_pcm_handle, alsa_hwparams, g_avix.Channels);
    if (ret < 0)
        goto audio_thread_exit;

    ret = snd_pcm_hw_params_set_rate(a_pcm_handle, alsa_hwparams, g_avix.SampleRate, 0);
    if (ret < 0)
        goto audio_thread_exit;

    // /2是为了方便移位
    bytes_per_sample = (g_avix.Bits * g_avix.Channels / 8 / 2);

    period_size = g_avix.AudioBufSize >> bytes_per_sample;
    ret = snd_pcm_hw_params_set_period_size(a_pcm_handle, alsa_hwparams, period_size, 0);
    if (ret < 0)
        goto audio_thread_exit;
    buffer_size = period_size * 2;
    ret = snd_pcm_hw_params_set_buffer_size(a_pcm_handle, alsa_hwparams, buffer_size);
    if (ret < 0)
        goto audio_thread_exit;

    ret = snd_pcm_hw_params(a_pcm_handle, alsa_hwparams);
    if (ret < 0)
        goto audio_thread_exit;

    mail_context *audio_mail = NULL;
    while (1) {
        if (0 > hal_queue_recv(q_audio_data, (void *)&audio_mail, 500))
            continue;
        if ((audio_mail->buff == NULL) && (audio_mail->size == 0)) {
            free(audio_mail);
            break;
        }
        do {
            ret = snd_pcm_writei(a_pcm_handle, audio_mail->buff,
                                 (audio_mail->size >> bytes_per_sample));
            if (ret == -EINTR) {
                ret = 0;
            } else if (ret == -ESTRPIPE) {
                do {
                    ret = snd_pcm_resume(a_pcm_handle);
                    hal_msleep(100);
                } while (ret == -EAGAIN);
            }
            if (ret < 0) {
                ret = snd_pcm_prepare(a_pcm_handle);
            }
        } while (ret == 0);

        free(audio_mail->buff);
        free(audio_mail);
        audio_mail = NULL;
    }
audio_thread_exit:
    printf("audio_thread exit\n");
    if (alsa_hwparams != NULL) {
        free(alsa_hwparams);
    }

    if (a_pcm_handle != NULL) {
        snd_pcm_close(a_pcm_handle);
    }

    while (1) {
        // 等待线程统一被回收
        hal_msleep(10);
    }
}
#endif

static int video_task_create(void)
{
    s_display_start = hal_sem_create(0);
    if (s_display_start == NULL)
        goto creat_error_exit;

    s_audio_start = hal_sem_create(0);
    if (s_audio_start == NULL)
        goto creat_error_exit;

    q_display_data = hal_queue_create("display_queue", sizeof(void *), 10);
    if (q_display_data == NULL)
        goto creat_error_exit;

    q_audio_data = hal_queue_create("audio_queue", sizeof(void *), 10);
    if (q_audio_data == NULL)
        goto creat_error_exit;

    /* 创建一个解封装线程 */
    t_play_handle =
            hal_thread_create(video_play, NULL, "video_thread", 1024, HAL_THREAD_PRIORITY_APP);
    if (t_play_handle == NULL)
        goto creat_error_exit;

    /* 创建一个显示线程 */
    t_display_handle = hal_thread_create(dispaly_thread, NULL, "dispaly_thread", 1024,
                                         HAL_THREAD_PRIORITY_APP);
    if (t_display_handle == NULL)
        goto creat_error_exit;

    /* 创建一个音频播放线程 */
    t_audio_handle =
            hal_thread_create(audio_thread, NULL, "audio_thread", 1024, HAL_THREAD_PRIORITY_APP);
    if (t_audio_handle == NULL)
        goto creat_error_exit;

    return 0;
creat_error_exit:
    if (t_play_handle != NULL) {
        hal_thread_stop(t_play_handle);
        t_play_handle = NULL;
    }

    if (t_display_handle != NULL) {
        hal_thread_stop(t_display_handle);
        t_display_handle = NULL;
    }

    if (t_audio_handle != NULL) {
        hal_thread_stop(t_audio_handle);
        t_audio_handle = NULL;
    }

    if (s_display_start != NULL) {
        hal_sem_delete(s_display_start);
        s_display_start = NULL;
    }

    if (s_audio_start != NULL) {
        hal_sem_delete(s_audio_start);
        s_audio_start = NULL;
    }

    if (q_display_data != NULL) {
        hal_queue_delete(q_display_data);
        q_display_data = NULL;
    }

    if (q_audio_data != NULL) {
        hal_queue_delete(q_audio_data);
        q_audio_data = NULL;
    }

    return -1;
}

int cmd_video_test(int argc, char *argv[])
{
    int c = 0;
    optind = 0;
    while ((c = getopt(argc, argv, "qhp:")) != -1) {
        switch (c) {
        case 'q':
            g_thread_quick = 1;
            break;
        case 'p':
            if (g_video_patch != NULL) {
                printf("maybe is palying?\n");
                return -1;
            }
            g_video_patch = malloc(128);
            if (g_video_patch == NULL) {
                printf("g_video_patch malloc fail\n");
                return -1;
            }
            strncpy(g_video_patch, optarg, 128);
            g_thread_quick = 0;
            video_task_create();
            break;
        case 'h':
            // lcd_test();
            // printf("test\n");
            break;
        default:
            return -1;
        }
    }

    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_video_test, video_test, video test demo);