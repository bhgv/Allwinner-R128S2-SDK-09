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

#ifndef __NAND_PHY_H
#define __NAND_PHY_H

__u32 _cal_addr_in_chip(__u32 block, __u32 page);
__u32 _cal_first_valid_bit(__u32 secbitmap);
__u32 _cal_valid_bits(__u32 secbitmap);
__s32 PHY_Init(void);
__s32 PHY_Exit(void);
__s32 PHY_ResetChip(__u32 nChip);
__s32 PHY_GetArchInfo_Str(char *buf);
__s32 PHY_ReadNandId_0(__s32 nChip, void *pChipID);
__s32 PHY_ReadNandId_1(__s32 nChip, void *pChipID);
__s32 PHY_ChangeMode(void);
__s32 PHY_SimpleRead(struct boot_physical_param *readop);
__s32 PHY_SimpleWrite(struct boot_physical_param *writeop);
__s32 PHY_SimpleErase(struct boot_physical_param *eraseop);
__s32 PHY_BlockErase(struct __PhysicOpPara_t *pBlkAdr);
__s32 PHY_PageRead(struct __PhysicOpPara_t *pPageAdr);
__s32 PHY_PageReadSpare(struct __PhysicOpPara_t *pPageAdr);
__s32 PHY_PageWrite(struct __PhysicOpPara_t  *pPageAdr);
__s32 PHY_PageCopyback(struct __PhysicOpPara_t *pSrcPage, struct __PhysicOpPara_t *pDstPage);
__s32 PHY_Wait_Status(__u32 nBank);

#endif

