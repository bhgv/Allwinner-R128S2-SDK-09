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

#ifndef SPINAND_PHYSIC_INTERFACE_H
#define SPINAND_PHYSIC_INTERFACE_H

typedef struct {
	__u8        ChipCnt;                            //the count of the total nand flash chips are currently connecting on the CE pin
	__u8        ConnectMode;                        //the rb connect  mode
	__u8        BankCntPerChip;                     //the count of the banks in one nand chip, multiple banks can support Inter-Leave
	__u8        DieCntPerChip;                      //the count of the dies in one nand chip, block management is based on Die
	__u8        PlaneCntPerDie;                     //the count of planes in one die, multiple planes can support multi-plane operation
	__u8        SectorCntPerPage;                   //the count of sectors in one single physic page, one sector is 0.5k
	__u16       ChipConnectInfo;                    //chip connect information, bit == 1 means there is a chip connecting on the CE pin
	__u32       PageCntPerPhyBlk;                   //the count of physic pages in one physic block
	__u32       BlkCntPerDie;                       //the count of the physic blocks in one die, include valid block and invalid block
	__u32       OperationOpt;                       //the mask of the operation types which current nand flash can support support
	__u32       FrequencePar;                       //the parameter of the hardware access clock, based on 'MHz'
	__u32       SpiMode;                            //spi nand mode, 0:mode 0, 3:mode 3
	__u8        NandChipId[8];                      //the nand chip id of current connecting nand chip
	__u32       pagewithbadflag;                    //bad block flag was written at the first byte of spare area of this page
	__u32       MultiPlaneBlockOffset;              //the value of the block number offset between the two plane block
	__u32       MaxEraseTimes;                      //the max erase times of a physic block
	__u32       MaxEccBits;                         //the max ecc bits that nand support
	__u32       EccLimitBits;                       //the ecc limit flag for tne nand
	__u32       uboot_start_block;
	__u32       uboot_next_block;
	__u32       logic_start_block;
	__u32       nand_specialinfo_page;
	__u32       nand_specialinfo_offset;
	__u32       physic_block_reserved;
	__u32       Reserved[4];
} boot_spinand_para_t;

typedef struct boot_flash_info {
	__u32 chip_cnt;
	__u32 blk_cnt_per_chip;
	__u32 blocksize;
	__u32 pagesize;
	__u32 pagewithbadflag; /*bad block flag was written at the first byte of spare area of this page*/
} boot_flash_info_t;

struct boot_physical_param {
	__u32  chip; //chip no
	__u32  block; // block no within chip
	__u32  page; // apge no within block
	__u32  sectorbitmap;
	void   *mainbuf; //data buf
	void   *oobbuf; //oob buf
};

struct _nand_info *SpiNandHwInit(void);
__s32 SpiNandHwExit(void);

#endif /*SPINAND_PHYSIC_INTERFACE_H*/
