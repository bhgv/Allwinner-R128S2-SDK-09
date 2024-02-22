#include "avi.h"
#include <stdio.h>
#include <string.h>

unsigned char *const AVI_VIDS_FLAG_TBL[2] = { "00dc", "01dc" }; //视频编码标志字符串,00dc/01dc
unsigned char *const AVI_AUDS_FLAG_TBL[2] = { "00wb", "01wb" }; //音频编码标志字符串,00wb/01wb

unsigned avi_search_id(unsigned char *buf, unsigned size, unsigned char *const id)
{
    size -= 4;
    for (int i = 0; i < size; i++) {
        if (memcmp((buf + i), id, 4) == 0) {
            return i;
        }
    }
    return 0;
}

int avi_get_list(unsigned type, unsigned char *buf, unsigned size)
{
    Avi_list *a_list = NULL;
    int offset = 0;
    do {
        a_list = (Avi_list *)(buf + offset);
        if (a_list->type == type) {
            break;
        } else {
            offset += (a_list->size + 8);
        }
    } while (offset < size);
    return offset;
}

int avi_get_chunk(unsigned id, unsigned char *buf, unsigned size)
{
    Avi_chunk *a_chunk = NULL;
    int offset = 0;
    do {
        a_chunk = (Avi_chunk *)(buf + offset);
        if (a_chunk->id == id) {
            break;
        } else {
            offset += (a_chunk->size + 8);
        }
    } while (offset < size);
    return offset;
}

// 返回已经处理的数据量
int avi_analy_strl(Avi_Info *avix, unsigned char *buf, unsigned size)
{
    int offset = 0;
    Avi_list *a_list = (Avi_list *)(buf + offset);
    if ((a_list->id != AVI_LIST_ID) && (a_list->type != AVI_STRL_ID)) {
        return -1;
    }

    if (a_list->size > (size - AVI_CHUNK_SIZE)) {
        // no enough size
        return -1;
    }

    offset += AVI_CHUNK_SIZE;
    offset += avi_get_chunk(AVI_STRH_ID, (buf + offset), (size - offset));
    AVIStreamHeader *avi_stream = (AVIStreamHeader *)(buf + offset);
    if (avi_stream->StreamType == AVI_VIDS_STREAM) {
        avix->VideoType = avi_stream->Handler;
        if ((avix->VideoFLAG == NULL) && (avix->AudioFLAG == NULL)) {
            avix->VideoFLAG = (unsigned char *)AVI_VIDS_FLAG_TBL[0];
            avix->AudioFLAG = (unsigned char *)AVI_AUDS_FLAG_TBL[1];
        }
        offset += avi_get_chunk(AVI_STRF_ID, (buf + offset), (size - offset));
        BitmapInfo *bmpheader = (BitmapInfo *)(buf + offset);
        avix->Width = bmpheader->bmiHeader.Width;
        avix->Height = bmpheader->bmiHeader.Height;
    } else if (avi_stream->StreamType == AVI_AUDS_STREAM) {
        avix->AudioType = avi_stream->Handler;
        if ((avix->VideoFLAG == NULL) && (avix->AudioFLAG == NULL)) {
            avix->VideoFLAG = (unsigned char *)AVI_VIDS_FLAG_TBL[1];
            avix->AudioFLAG = (unsigned char *)AVI_AUDS_FLAG_TBL[0];
        }
        avix->AudioBufSize = avi_stream->RefBufSize;
        offset += avi_get_chunk(AVI_STRF_ID, (buf + offset), (size - offset));
        WaveFormatEx *wavheader = (WaveFormatEx *)(buf + offset);
        avix->SampleRate = wavheader->SampleRate;
        avix->Channels = wavheader->Channels;
        // 并非所有的avi格式，都是会从最后一个字获取采样位深
        // avix->Bits = wavheader->Bits;
        avix->Bits = (wavheader->BlockAlign / avix->Channels * 8);
        avix->AudioType = wavheader->FormatTag;
    } else {
        printf("not support now\n");
    }

    Avi_chunk *a_chunk = (Avi_chunk *)(buf + offset);
    offset += (a_chunk->size + 8);
    return offset;
}
// 返回已经处理的数据量
int avi_analy_hdrl(Avi_Info *avix, unsigned char *buf, unsigned size)
{
    int offset = 0;
    Avi_list *a_list = (Avi_list *)(buf + offset);
    if ((a_list->id != AVI_LIST_ID) && (a_list->type != AVI_HDRL_ID)) {
        return -1;
    }

    if (a_list->size > (size - AVI_CHUNK_SIZE)) {
        // no enough size
        return -1;
    }
    offset += AVI_CHUNK_SIZE;
    offset += avi_get_chunk(AVI_AVIH_ID, (buf + offset), (size - offset));
    AVIMainHeader *avih = (AVIMainHeader *)(buf + offset);
    avix->SecPerFrame = avih->SecPerFrame; //得到帧间隔时间,注意，该值不可靠
    avix->TotalFrame = avih->TotalFrame;   //得到总帧数

    int try_offset = 0;
    do {
        offset += avi_get_list(AVI_STRL_ID, (buf + offset), (size - offset));
        offset += avi_analy_strl(avix, (buf + offset), (size - offset));
        try_offset = avi_get_list(AVI_STRL_ID, (buf + offset), (size - offset));
        if ((try_offset + offset + AVI_CHUNK_SIZE) >= a_list->size) {
            break;
        }
    } while ((offset - AVI_CHUNK_SIZE) < a_list->size);

    // a_list = (Avi_list *)(buf + offset);
    // offset += a_list->size;
    return offset;
}

int avi_demuxer(Avi_Info *avix, unsigned char *buf, unsigned size)
{
    unsigned char *rebuf = buf;
    int offset = 0;

    AVI_HEADER *aviheader = (AVI_HEADER *)buf;
    if ((aviheader->RiffID != AVI_RIFF_ID) && (aviheader->AviID != AVI_AVI_ID)) {
        return -1;
    }
    offset += AVI_CHUNK_SIZE;

    offset += avi_get_list(AVI_HDRL_ID, (buf + offset), (size - offset));
    offset += avi_analy_hdrl(avix, (buf + offset), (size - offset));
    // int offset = avi_search_id(rebuf, size, "movi");
    // if (offset == 0) {
    //     return -1;
    // }
    // if (avix->SampleRate) {
    //     rebuf += offset;
    //     offset = avi_search_id(rebuf, (size - offset), avix->AudioFLAG);
    //     rebuf += offset;
    //     rebuf += 4;
    //     avix->AudioBufSize = *((unsigned short *)rebuf);
    // }

    printf("avi init ok\r\n");
    printf("avix->SecPerFrame:%d\r\n", avix->SecPerFrame);
    printf("avix->TotalFrame:%d\r\n", avix->TotalFrame);
    printf("avix->Width:%d\r\n", avix->Width);
    printf("avix->Height:%d\r\n", avix->Height);
    printf("avix->AudioType:%d\r\n", avix->AudioType);
    printf("avix->SampleRate:%d\r\n", avix->SampleRate);
    printf("avix->Channels:%d\r\n", avix->Channels);
    printf("avix->Bits:%d\r\n", avix->Bits);
    printf("avix->AudioBufSize:%d\r\n", avix->AudioBufSize);
    printf("avix->VideoFLAG:%s\r\n", avix->VideoFLAG);
    printf("avix->AudioFLAG:%s\r\n", avix->AudioFLAG);
    return 0;
}

#define MAKEWORD(ptr)                                                                              \
    (unsigned short)(((unsigned short)*((unsigned char *)(ptr)) << 8) |                            \
                     (unsigned short)*(unsigned char *)((ptr) + 1))
#define MAKEDWORD(ptr)                                                                             \
    (unsigned)(((unsigned short)*(unsigned char *)(ptr) |                                          \
                (((unsigned short)*(unsigned char *)(ptr + 1)) << 8) |                             \
                (((unsigned short)*(unsigned char *)(ptr + 2)) << 16) |                            \
                (((unsigned short)*(unsigned char *)(ptr + 3)) << 24)))

// buf:流开始地址(必须是01wb/00wb/01dc/00dc开头)
int avi_get_streaminfo(Avi_Info *avix, moviInfo *chunk)
{
    avix->StreamID = chunk->type;
    if ((avix->StreamID != AVI_VIDS_FLAG) && (avix->StreamID != AVI_AUDS_FLAG)) {
        return -1;
    }
    avix->StreamSize = chunk->size;
    if ((avix->StreamSize & 0x01) != 0x00) {
        avix->StreamSize++;
    }
    return 0;
}
