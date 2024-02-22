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

#include "nand_osal.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aw_common.h>
#include <hal_mutex.h>

/*
*NAND_DRV_VERSION_0:	Version number of the driver
*NAND_DRV_VERSION_1:	Version number of the driver
*NAND_DRV_DATE:			Time when the driver changes(Year-month-day)
*NAND_DRV_TIME:			Time when the driver changes(Specific time)
*If the flash driver is modified
*NAND_DRV_DATE and NAND_DRV_VERSION_X must be modified simultaneously
 */
#define  NAND_DRV_VERSION_0		0x03
#define  NAND_DRV_VERSION_1		0x6069
#define  NAND_DRV_DATE			0x20230823
#define  NAND_DRV_TIME			0x18211952

extern void hal_udelay(unsigned int us);

void NAND_Panic(const char *fmt, ...)
{
	// TODO
}

int NAND_Snprintf(char *str, unsigned int size, const char *fmt, ...)
{
	int rtn = 0;
	va_list args;

	va_start(args, fmt);

	rtn = vsnprintf(str, size, fmt, args);

	va_end(args);

	return rtn;
}

int NAND_Sprintf(char *str, const char *fmt, ...)
{
	int rtn = 0;
	va_list args;

	va_start(args, fmt);

	rtn = vsprintf(str, fmt, args);

	va_end(args);

	return rtn;
}

int NAND_Print(const char *fmt, ...)
{
	int rtn = 0;
	va_list args;

	va_start(args, fmt);

	rtn = vprintf(fmt, args);

	va_end(args);
	return rtn;
}

int NAND_Print_DBG(const char *fmt, ...)
{
	int rtn = 0;
	va_list args;

	va_start(args, fmt);


	rtn = vprintf(fmt, args);

	va_end(args);
	return rtn;
}

void NAND_Mdelay(unsigned long ms)
{
	hal_udelay(ms * 1000);
}

void NAND_Memset(void *pAddr, unsigned char value, unsigned int len)
{
	memset(pAddr, value, len);
}

int NAND_Memcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}

int NAND_Strcmp(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}

void NAND_Memcpy(void *pAddr_dst, void *pAddr_src, unsigned int len)
{
	memcpy(pAddr_dst, pAddr_src, len);
}

void *NAND_Malloc(unsigned int Size)
{
	return malloc(Size);
}

void NAND_Free(void *pAddr)
{
	free(pAddr);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static hal_mutex_t nand_physic_mutex;

int NAND_PhysicLockInit(void)
{
	nand_physic_mutex = hal_mutex_create();
	if (nand_physic_mutex) {
		return 0;
	}
	return 1;
}

int NAND_PhysicLock(void)
{
	return hal_mutex_lock(nand_physic_mutex);
}

int NAND_PhysicUnLock(void)
{
	return hal_mutex_unlock(nand_physic_mutex);
}

int NAND_PhysicLockExit(void)
{
	hal_mutex_delete(nand_physic_mutex);
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
__u32 NAND_GetMaxChannelCnt(void)
{
	return 1;
}

int NAND_get_storagetype(void)
{
	return 0;
}

void NAND_Print_Version(void)
{
	int val[4] = { 0 };

	val[0] = NAND_DRV_VERSION_0;
	val[1] = NAND_DRV_VERSION_1;
	val[2] = NAND_DRV_DATE;
	val[3] = NAND_DRV_TIME;

	NAND_Print("kernel: nand version: %x %x %x %x\n", val[0],val[1],val[2],val[3]);
	NAND_Print("kernel: flash driver compilation time %s %s\n", __DATE__, __TIME__);
}

int NAND_Get_Version(int *ver_main, int *ver_sub, int *date, int *time)
{
	*ver_main = NAND_DRV_VERSION_0;
	*ver_sub = NAND_DRV_VERSION_1;
	*date = NAND_DRV_DATE;
	*time = NAND_DRV_TIME;
	return 0;
}


