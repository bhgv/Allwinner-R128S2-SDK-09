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

#ifndef __NAND_PHYSIC_INTERFACE_H
#define __NAND_PHYSIC_INTERFACE_H

extern unsigned short cur_w_lb_no;
extern unsigned short cur_w_pb_no;
extern unsigned short cur_w_p_no;
extern unsigned short cur_e_lb_no;
struct _nand_info *NandHwInit(void);
int NandHwExit(void);
int nand_physic_erase_block(unsigned int chip, unsigned int block);
int nand_physic_read_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf);
int nand_physic_write_page(unsigned int chip, unsigned int block, unsigned int page, unsigned int bitmap, unsigned char *mbuf, unsigned char *sbuf);
int nand_physic_bad_block_check(unsigned int chip, unsigned int block);
int nand_physic_bad_block_mark(unsigned int chip, unsigned int block);
int nand_physic_block_copy(unsigned int chip_s, unsigned int block_s, unsigned int chip_d, unsigned int block_d);
int PHY_VirtualPageRead(unsigned int nDieNum, unsigned int nBlkNum, unsigned int nPage, uint64 SectBitmap, void *pBuf, void *pSpare);
int PHY_VirtualPageWrite(unsigned int nDieNum, unsigned int nBlkNum, unsigned int nPage, uint64 SectBitmap, void *pBuf, void *pSpare);
int PHY_VirtualBlockErase(unsigned int nDieNum, unsigned int nBlkNum);
int PHY_VirtualBadBlockCheck(unsigned int nDieNum, unsigned int nBlkNum);
int PHY_VirtualBadBlockMark(unsigned int nDieNum, unsigned int nBlkNum);
__u32 NAND_GetLsbblksize(void);
__u32 NAND_GetLsbPages(void);
__u32 NAND_GetPhyblksize(void);
__u32 NAND_UsedLsbPages(void);
__u32 NAND_GetPageNo(__u32 lsb_page_no);
__u32 NAND_GetPageCntPerBlk(void);
__u32 NAND_GetPageSize(void);
__u32 nand_get_twoplane_flag(void);

#endif
