#ifndef __AVI_H
#define __AVI_H

#define AVI_VIDS_FLAG 0X6364 //视频流标志
#define AVI_AUDS_FLAG 0X6277 //音频流标志
//////////////////////////////////////////////////////////////////////////////////////////

//AVI 信息结构体
//将一些重要的数据,存放在这里,方便解码
typedef struct _Avi_Info {
    unsigned VideoType;    //视屏类型
    unsigned SecPerFrame;  //视频帧间隔时间(单位为us)
    unsigned TotalFrame;   //文件总帧数
    unsigned Width;        //图像宽
    unsigned Height;       //图像高
    unsigned SampleRate;   //音频采样率
    unsigned Channels;     //声道数,一般为2,表示立体声
    unsigned Bits;         //位深
    unsigned AudioBufSize; //音频缓冲区大小
    unsigned AudioType;    //音频类型:0X0001=PCM;0X0050=MP2;0X0055=MP3;0X2000=AC3;
    unsigned StreamID;     //流类型ID,StreamID=='dc'==0X6463 /StreamID=='wb'==0X7762
    unsigned StreamSize; //流大小,必须是偶数,如果读取到为奇数,则加1.补为偶数.
    unsigned char *VideoFLAG; //视频帧标记,VideoFLAG="00dc"/"01dc"
    unsigned char *AudioFLAG; //音频帧标记,AudioFLAG="00wb"/"01wb"
} Avi_Info;

//////////////////////////////////////////////////////////////////////////////////////////
//AVI CHUNK 结构
#define AVI_CHUNK_SIZE 12
typedef struct _Avi_chunk {
    unsigned id;
    unsigned size;
    unsigned char *data;
} Avi_chunk;

//AVI LIST 结构
typedef struct _Avi_list {
    unsigned id;
#define AVI_LIST_ID 0X5453494C
    unsigned size;
    unsigned type;
#define AVI_HDRL_ID 0X6C726468 //信息块标志
#define AVI_MOVI_ID 0X69766F6D //数据块标志
#define AVI_STRL_ID 0X6C727473 //strl标志
#define AVI_STRD_ID 0X64727473 //strd子块∈AVI_STRL_ID (可选的)
    unsigned char *data;
} Avi_list;

//AVI 文件头部信息
typedef struct _Avi_Header {
    unsigned RiffID; //RiffID=='RIFF'==0X46464952
#define AVI_RIFF_ID 0X46464952
    unsigned FileSize; //AVI文件大小(不包含最初的8字节,也RIFFID和FileSize不计算在内)
    unsigned AviID;    //AviID=='AVI '==0X41564920
#define AVI_AVI_ID 0X20495641
} AVI_HEADER;

// AVI主头部
typedef struct {
    unsigned id;               // 必须为 avih
#define AVI_AVIH_ID 0X68697661 //avih子块∈AVI_HDRL_ID
    unsigned size; // 本数据结构的大小，不包括最初的8个字节（fcc和cb两个域）
    unsigned SecPerFrame;        // 视频帧间隔时间（以微秒为单位）
    unsigned MaxByteSec;         // 这个AVI文件的最大数据率
    unsigned PaddingGranularity; // 数据填充的粒度
    unsigned Flags;              // AVI文件的全局标记，比如是否含有索引块等
    unsigned TotalFrame;         // 总帧数
    unsigned InitFrames;  // 为交互格式指定初始帧数（非交互格式应该指定为0）
    unsigned Streams;     // 本文件包含的流的个数
    unsigned RefBufSize;  // 建议读取本文件的缓存大小（应能容纳最大的块）
    unsigned Width;       // 视频图像的宽（以像素为单位）
    unsigned Height;      // 视频图像的高（以像素为单位）
    unsigned Reserved[4]; // 保留
} AVIMainHeader;

// AVI流头部
typedef struct {
    unsigned id;               // 必须为 strh
#define AVI_STRH_ID 0X68727473 //strh(流头)子块∈AVI_STRL_ID
    unsigned size;       // 本数据结构的大小,不包括最初的8个字节(fcc和cb两个域)
    unsigned StreamType; // 流的类型: auds(音频流) vids(视频流) mids(MIDI流) txts(文字流)
#define AVI_VIDS_STREAM 0X73646976 //视频流
#define AVI_AUDS_STREAM 0X73647561 //音频流
    unsigned Handler;              // 指定流的处理者，对于音视频来说就是解码器
#define AVI_FORMAT_MJPG 0X47504A4D
#define AVI_FORMAT_XVID 0X64697678
    unsigned Flags; // 标记：是否允许这个流输出？调色板是否变化？
    unsigned short Priority; // 流的优先级（当有多个相同类型的流时优先级最高的为默认流）
    unsigned short Language; // 语言
    unsigned InitFrames;     // 为交互格式指定初始帧数
    unsigned Scale;          // 每帧视频大小或者音频采样大小
    unsigned Rate;           // dwScale/dwRate，每秒采样率
    unsigned Start;          // 流的开始时间
    unsigned Length;         // 流的长度（单位与dwScale和dwRate的定义有关）
    unsigned RefBufSize;     // 读取这个流数据建议使用的缓存大小
    unsigned Quality;        // 流数据的质量指标（0 ~ 10,000）
    unsigned SampleSize;     // Sample的大小
    struct                   //视频帧所占的矩形
    {
        short Left;
        short Top;
        short Right;
        short Bottom;
    } Frame;
} AVIStreamHeader;

//BMP结构体
typedef struct {
    unsigned BmpSize;        //bmp结构体大小,包含(BmpSize在内)
    unsigned Width;          //图像宽
    unsigned Height;         //图像高
    unsigned short Planes;   //平面数，必须为1
    unsigned short BitCount; //像素位数,0X0018表示24位
    unsigned Compression;    //压缩类型，比如:MJPG/H264等
    unsigned SizeImage;      //图像大小
    unsigned XpixPerMeter;   //水平分辨率
    unsigned YpixPerMeter;   //垂直分辨率
    unsigned ClrUsed;        //实际使用了调色板中的颜色数,压缩格式中不使用
    unsigned ClrImportant;   //重要的颜色
} BitmapInfoHeader;

//颜色表
typedef struct {
    unsigned char rgbBlue;     //蓝色的亮度(值范围为0-255)
    unsigned char rgbGreen;    //绿色的亮度(值范围为0-255)
    unsigned char rgbRed;      //红色的亮度(值范围为0-255)
    unsigned char rgbReserved; //保留，必须为0
} AVIRGBQuad;

// 视频流
typedef struct {
    unsigned id;               //块标志,strf==0X73747266
#define AVI_STRF_ID 0X66727473 //strf(流格式)子块∈AVI_STRL_ID
    unsigned size; //块大小(不包含最初的8字节,也就是BlockID和本BlockSize不计算在内)
    BitmapInfoHeader bmiHeader; //位图信息头
    AVIRGBQuad bmColors[1];     //颜色表
} BitmapInfo;

// 音频流
typedef struct {
    unsigned id; //块标志,strf==0X73747266
    unsigned size; //块大小(不包含最初的8字节,也就是BlockID和本BlockSize不计算在内)
    unsigned short FormatTag;  //格式标志:0X0001=PCM,0X0055=MP3...
    unsigned short Channels;   //声道数,一般为2,表示立体声
    unsigned SampleRate;       //音频采样率
    unsigned BaudRate;         //波特率
    unsigned short BlockAlign; //数据块对齐标志
    unsigned short Bits;       //采样位深
} WaveFormatEx;

// movi数据类型,实际上，数据结构依然是个chunk
typedef struct {
    unsigned short req;
    unsigned short type;
    unsigned size;
} moviInfo;

int avi_demuxer(Avi_Info *avix, unsigned char *buf, unsigned size);
unsigned avi_search_id(unsigned char *buf, unsigned size, unsigned char *const id);
int avi_get_streaminfo(Avi_Info *avix, moviInfo *chunk);
#endif
