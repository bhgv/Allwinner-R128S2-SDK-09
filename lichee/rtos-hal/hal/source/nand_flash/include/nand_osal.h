/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _NAND_OSAL_H__
#define _NAND_OSAL_H__

#include <stdio.h>

/* memory allocate */
extern void *NAND_Malloc(unsigned int size);
extern void NAND_Free(void *pAddr);

#ifndef __s8
typedef signed char         __s8;
#endif
#ifndef _s8
typedef signed char         _s8;
#endif
#ifndef CONFIG_OS_NUTTX
#ifndef s8
typedef signed char         s8;
#endif
#endif
#ifndef __u8
typedef unsigned char       __u8;
#endif
#ifndef _u8
typedef unsigned char       _u8;
#endif
#ifndef  u8
typedef unsigned char       u8;
#endif
#ifndef  uchar
typedef unsigned char       uchar;
#endif
#ifndef  __s16
typedef signed short        __s16;
#endif
#ifndef _s16
typedef signed short        _s16;
#endif
#ifndef  s16
typedef signed short        s16;
#endif
#ifndef __u16
typedef unsigned short      __u16;
#endif
#ifndef _u16
typedef unsigned short      _u16;
#endif
#ifndef  u16
typedef unsigned short      u16;
#endif
#ifndef  uint16
typedef unsigned short      uint16;
#endif
#ifndef DATA_TYPE_X___s32
typedef signed int          __s32;
#define DATA_TYPE_X___s32
#endif
#ifndef  _s32
typedef signed int          _s32;
#endif
#if !defined(CONFIG_OS_NUTTX) && !defined(CONFIG_OS_MELIS)
#ifndef   s32
typedef signed int          s32;
#endif
#endif
#ifndef DATA_TYPE_X___u32
typedef unsigned int        __u32;
#define DATA_TYPE_X___u32
#endif
#ifndef _u32
typedef unsigned int        _u32;
#endif
#if !defined(CONFIG_OS_NUTTX) && !defined(CONFIG_OS_MELIS)
#ifndef  u32
typedef unsigned int        u32;
#endif
#endif
#ifndef uint32
typedef unsigned int        uint32;
#endif
#ifndef  __s64
typedef int64_t    __s64;
#endif
#ifndef _s64
typedef signed long long    _s64;
#endif
#ifndef  s64
typedef int64_t    s64;
#endif
#ifndef __u64
typedef uint64_t  __u64;
#endif
#ifndef _u64
typedef unsigned long long  _u64;
#endif
#ifndef  u64
typedef uint64_t  u64;
#endif
#ifndef uint64
typedef unsigned long long  uint64;
#endif

extern void NAND_Memset(void* pAddr, unsigned char value, unsigned int len);
extern void NAND_Memcpy(void* pAddr_dst, void* pAddr_src, unsigned int len);
extern int NAND_Memcmp(const void* pAddr_dst, const void* pAddr_src, size_t len);
extern int NAND_Strcmp(const char *s1, const char *s2);
extern int NAND_Sprintf(char *str, const char *format, ...);
extern int NAND_Snprintf(char *str, unsigned int size, const char *format, ...);
extern int NAND_Print(const char *fmt, ...);
extern int NAND_Print_DBG(const char *fmt, ...);

extern int NAND_PhysicLockInit(void);
extern int NAND_PhysicLock(void);
extern int NAND_PhysicUnLock(void);
extern int NAND_PhysicLockExit(void);

extern void NAND_Panic(const char *fmt, ...);
extern void NAND_Mdelay(unsigned long ms);

extern void NAND_Print_Version(void);
extern void Dump_Gpio_Reg_Show(void);
extern void Dump_Ccmu_Reg_Show(void);
//define the memory alocate interface
#define MALLOC(x)                       	NAND_Malloc((x))

//define the memory release interface
#define FREE(x,size)                    	NAND_Free((x))

//define the memory set interface
#define MEMSET(x,y,z) NAND_Memset((x),(y),(z))

//define the memory copy interface
#define MEMCPY(x,y,z) NAND_Memcpy((x),(y),(z))

#define MEMCMP(x,y,z) NAND_Memcmp((x),(y),(z))

//define the memory alocate interface
#define N_mdelay(x) NAND_Mdelay((x))
//define the memory release interface

//define the strcmp release interface
#define STRCMP(x, y) NAND_Strcmp((x), (y))
//define the message print interface
#define PRINT NAND_Print
#define PRINT_DBG NAND_Print_DBG

#ifdef CONFIG_NAND_FLASH_DEBUG
    #define NFTL_DBG(...) PRINT_DBG(__VA_ARGS__)
    #define NFTL_ERR(...) PRINT(__VA_ARGS__)
#else
    #define NFTL_DBG(...)
    #define NFTL_ERR(...)
#endif

#ifdef CONFIG_NAND_FLASH_DEBUG
    #define PHY_DBG_MESSAGE_ON                  	(1)
    #define PHY_ERR_MESSAGE_ON                      (1)

    #define SCAN_DBG_MESSAGE_ON                     (1)
    #define SCAN_ERR_MESSAGE_ON                     (1)

    #define FORMAT_DBG_MESSAGE_ON                   (1)
    #define FORMAT_ERR_MESSAGE_ON                   (1)

    #define MAPPING_DBG_MESSAGE_ON                  (1)
    #define MAPPING_ERR_MESSAGE_ON                  (1)

    #define LOGICCTL_DBG_MESSAGE_ON             	(1)
    #define LOGICCTL_ERR_MESSAGE_ON             	(1)
#else
    #define PHY_DBG_MESSAGE_ON                  	(0)
    #define PHY_ERR_MESSAGE_ON                 	    (0)

    #define SCAN_DBG_MESSAGE_ON                     (0)
    #define SCAN_ERR_MESSAGE_ON                     (0)

    #define FORMAT_DBG_MESSAGE_ON          	        (0)
    #define FORMAT_ERR_MESSAGE_ON                   (0)

    #define MAPPING_DBG_MESSAGE_ON                  (0)
    #define MAPPING_ERR_MESSAGE_ON                  (0)

    #define LOGICCTL_DBG_MESSAGE_ON                 (0)
    #define LOGICCTL_ERR_MESSAGE_ON                 (0)
#endif

#if PHY_DBG_MESSAGE_ON
#define	   PHY_DBG(...)        			PRINT_DBG(__VA_ARGS__)
#else
#define     PHY_DBG(...)
#endif

#if PHY_ERR_MESSAGE_ON
#define     PHY_ERR(...)        		PRINT(__VA_ARGS__)
#else
#define     PHY_ERR(...)
#endif


#if SCAN_DBG_MESSAGE_ON
#define     SCAN_DBG(...)          		PRINT_DBG(__VA_ARGS__)
#else
#define     SCAN_DBG(...)
#endif

#if SCAN_ERR_MESSAGE_ON
#define     SCAN_ERR(...)         		PRINT(__VA_ARGS__)
#else
#define     SCAN_ERR(...)
#endif


#if FORMAT_DBG_MESSAGE_ON
#define     FORMAT_DBG(...)         	PRINT_DBG(__VA_ARGS__)
#else
#define     FORMAT_DBG(...)
#endif

#if FORMAT_ERR_MESSAGE_ON
#define     FORMAT_ERR(...)        		PRINT(__VA_ARGS__)
#else
#define     FORMAT_ERR(...)
#endif


#if MAPPING_DBG_MESSAGE_ON
#define     MAPPING_DBG(...)        	PRINT_DBG(__VA_ARGS__)
#else
#define     MAPPING_DBG(...)
#endif

#if MAPPING_ERR_MESSAGE_ON
#define     MAPPING_ERR(...)       		PRINT(__VA_ARGS__)
#else
#define     MAPPING_ERR(...)
#endif


#if LOGICCTL_DBG_MESSAGE_ON
#define     LOGICCTL_DBG(...)       	PRINT_DBG(__VA_ARGS__)
#else
#define     LOGICCTL_DBG(...)
#endif

#if LOGICCTL_ERR_MESSAGE_ON
#define     LOGICCTL_ERR(...)       	PRINT(__VA_ARGS__)
#else
#define     LOGICCTL_ERR(...)
#endif

#endif //__NAND_OSAL_H__
