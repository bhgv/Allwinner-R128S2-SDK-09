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

#ifndef __SPI_NAND2_H
#define __SPI_NAND2_H

extern __s32 m2_spi_nand_reset(__u32 spi_no, __u32 chip);
extern __s32 m2_spi_nand_read_status(__u32 spi_no, __u32 chip, __u8 status, __u32 mode);
extern __s32 m2_spi_nand_setstatus(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m2_spi_nand_getblocklock(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m2_spi_nand_setblocklock(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m2_spi_nand_getotp(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m2_spi_nand_setotp(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m2_spi_nand_getoutdriver(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m2_spi_nand_setoutdriver(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m2_erase_single_block(struct boot_physical_param *eraseop);
extern __s32 m2_write_single_page(struct boot_physical_param *writeop);
extern __s32 m2_read_single_page(struct boot_physical_param *readop, __u32 spare_only_flag);

#endif
