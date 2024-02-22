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

#ifndef __NAND_STRUCT_H
#define __NAND_STRUCT_H

#include "nand_osal.h"
#include "nand_global.h"

struct _nand_phy_partition;

///////////// fix ok above //////////////////////////

#define MAX_PART_COUNT_PER_FTL		24
#define MAX_PARTITION        		4
#define PARTITION_NAME_SIZE  16


#define ND_MAX_PARTITION_COUNT      (MAX_PART_COUNT_PER_FTL*MAX_PARTITION)

#define MIN_PHY_RESERVED_BLOCK_V2     0

#define FACTORY_BAD_BLOCK_SIZE	       2048
#define PHY_PARTITION_BAD_BLOCK_SIZE	 4096
#define PARTITION_BAD_BLOCK_SIZE	     4096

#define ENABLE_CRC_MAGIC 0x63726365 //crce

#define BYTES_PER_SECTOR                          512
#define SHIFT_PER_SECTOR                          9
#define BYTES_OF_USER_PER_PAGE                    16
#define MIN_BYTES_OF_USER_PER_PAGE                16


#define FTL_PARTITION_TYPE        0x8000
#define FTL_CROSS_TALK            0x4000

struct _uboot_info {
	unsigned int  sys_mode;
	unsigned int  use_lsb_page;
	unsigned int  copys;

	unsigned int  uboot_len;
	unsigned int  total_len;
	unsigned int  uboot_pages;
	unsigned int  total_pages;

	unsigned int  blocks_per_total;
	unsigned int  page_offset_for_nand_info;
	unsigned int  byte_offset_for_nand_info;
	unsigned char  uboot_block[120];

	unsigned int  nboot_copys;
	unsigned int  nboot_len;
	unsigned int  nboot_data_per_page;   //43

	unsigned int  nouse[64 - 43];
};


//==============================================================================
//  define the data structure for physic layer module
//==============================================================================
struct boot_physical_param;
struct spi_nand_function {
	__s32(* spi_nand_reset)(__u32 spi_no, __u32 chip);
	__s32(* spi_nand_read_status)(__u32 spi_no, __u32 chip, __u8 status, __u32 mode);
	__s32(* spi_nand_setstatus)(__u32 spi_no, __u32 chip, __u8 reg);
	__s32(* spi_nand_getblocklock)(__u32 spi_no, __u32 chip, __u8 *reg);
	__s32(* spi_nand_setblocklock)(__u32 spi_no, __u32 chip, __u8 reg);
	__s32(* spi_nand_getotp)(__u32 spi_no, __u32 chip, __u8 *reg);
	__s32(* spi_nand_setotp)(__u32 spi_no, __u32 chip, __u8 reg);
	__s32(* spi_nand_getoutdriver)(__u32 spi_no, __u32 chip, __u8 *reg);
	__s32(* spi_nand_setoutdriver)(__u32 spi_no, __u32 chip, __u8 reg);
	__s32(* erase_single_block)(struct boot_physical_param *eraseop);
	__s32(* write_single_page)(struct boot_physical_param *writeop);
	__s32(* read_single_page)(struct boot_physical_param *readop, __u32 spare_only_flag);
};

//define the nand flash storage system information
struct __NandStorageInfo_t {
	__u8 ChipCnt;                                // the count of the total nand flash chips are currently connecting on the CE pin
	__u16 ChipConnectInfo;                       // chip connect information, bit == 1 means there is a chip connecting on the CE pin
	__u8 ConnectMode;                            // the rb connect  mode
	__u8 BankCntPerChip;                         // the count of the banks in one nand chip, multiple banks can support Inter-Leave
	__u8 DieCntPerChip;                          // the count of the dies in one nand chip, block management is based on Die
	__u8 PlaneCntPerDie;                         // the count of planes in one die, multiple planes can support multi-plane operation
	__u8 SectorCntPerPage;                       // the count of sectors in one single physic page, one sector is 0.5k
	__u16 PageCntPerPhyBlk;                      // the count of physic pages in one physic block
	__u32 BlkCntPerDie;                          // the count of the physic blocks in one die, include valid block and invalid block
	__u32 OperationOpt;                          // the mask of the operation types which current nand flash can support support
	__u16 FrequencePar;                          // the parameter of the hardware access clock, based on 'MHz'
	__u32 SpiMode;                               // spi nand mode, 0:mode 0, 3:mode 3
	__u8 NandChipId[8];                          // the nand chip id of current connecting nand chip
	__u32 pagewithbadflag;                       // bad block flag was written at the first byte of spare area of this page
	__u32 MultiPlaneBlockOffset;                 // the value of the block number offset between the two plane block
	__u32 MaxEraseTimes;                         // the max erase times of a physic block
	__u32 MaxEccBits;                            // the max ecc bits that nand support
	__u32 EccLimitBits;                          // the ecc limit flag for tne nand
	__u32 Idnumber;
	__u32 EccType;                               // Just use in spinand2, select different ecc status type.
	__u32 EccProtectedType;                      // just use in spinand2, select different ecc protected type.
	const char *Model;
	struct spi_nand_function *spi_nand_function; // erase,write,read function for spi nand
};

struct _spinand_config_para_info {
	unsigned int    super_chip_cnt;
	unsigned int    super_block_nums;
	unsigned int    support_two_plane;
	unsigned int    support_v_interleave;
	unsigned int    support_dual_channel;
	unsigned int    plane_cnt;
	unsigned int    support_dual_read;
	unsigned int    support_dual_write;
	unsigned int    support_quad_read;
	unsigned int    support_quad_write;
	unsigned int    frequence;
};

//define the page buffer pool for nand flash driver
struct __NandPageCachePool_t {
	__u8        	*PageCache0;                        //the pointer to the first page size ram buffer
	__u8		*SpareCache;
	__u8		*TmpPageCache;
	__u8        	*SpiPageCache;
	__u8 		*SpareCache1; // 64bytes, used by mX_spi_nand_read_xY
};

//define the paramter structure for physic operation function
struct __PhysicOpPara_t {
	__u32       BankNum;                            //the number of the bank current accessed, bank NO. is different of chip NO.
	__u32       PageNum;                            //the number of the page current accessed, the page is based on single-plane or multi-plane
	__u32       BlkNum;                             //the number of the physic block, the block is based on single-plane or multi-plane
	__u64       SectBitmap;                         //the bitmap of the sector in the page which need access data
	void        *MDataPtr;                          //the pointer to main data buffer, it is the start address of a page size based buffer
	void        *SDataPtr;                          //the pointer to spare data buffer, it will be set to NULL if needn't access spare data
};


//define the nand flash physical information parameter type, for id table
struct __NandPhyInfoPar_t {
	__u8 NandID[8];                              //the ID number of the nand flash chip
	__u8 DieCntPerChip;                          //the count of the Die in one nand flash chip
	__u8 SectCntPerPage;                         //the count of the sectors in one single physical page
	__u16 PageCntPerBlk;                         //the count of the pages in one single physical block
	__u16 BlkCntPerDie;                          //the count fo the physical blocks in one nand flash Die
	__u32 OperationOpt;                          //the bitmap that marks which optional operation that the nand flash can support
	__u16 AccessFreq;                            //the highest access frequence of the nand flash chip, based on MHz
	__u32 SpiMode;                               //spi nand mode, 0:mode 0, 3:mode 3
	__u32 pagewithbadflag;                       //bad block flag was written at the first byte of spare area of this page
	struct spi_nand_function *spi_nand_function; //erase,write,read function for spi nand
	__u32 MultiPlaneBlockOffset;                 //the value of the block number offset between the two plane block
	__u32 MaxEraseTimes;                         //the max erase times of a physic block
	__u32 MaxEccBits;                            //the max ecc bits that nand support
	__u32 EccLimitBits;                          //the ecc limit flag for tne nand
	__u32 Idnumber;
	__u32 EccType;                               //Just use in spinand2, select different ecc status type.
	__u32 EccProtectedType;                      //just use in spinand2, select different ecc protected type.
	const char *Model;
	//__u8 reserved[4];                                                              // reserved for 32bit align
};

//==============================================================================
//  define the data structure for logic management module
//==============================================================================

//define the logical architecture parameter structure
struct __LogicArchitecture_t {
	__u32 		PageCntPerLogicBlk;         //the number of sectors in one logical page
	__u32 		SectCntPerLogicPage;        //the number of logical page in one logical block
	__u32 		LogicBlkCntPerLogicDie;     //the number of logical block in a logical die
	__u32 		LogicDieCnt;                //the number of logical die
};

struct _NAND_CRC32_DATA {
	unsigned  int CRC;
	unsigned  int CRC_32_Tbl[256];
};

/* part info */
typedef struct _NAND_PARTITION {
	unsigned  char      classname[PARTITION_NAME_SIZE];
	unsigned  int       addr;
	unsigned  int       len;
	unsigned  int       user_type;
	unsigned  int       keydata;
	unsigned  int       ro;
} NAND_PARTITION;   //36bytes

/* mbr info */
typedef struct _PARTITION_MBR {
	unsigned  int		CRC;
	unsigned  int       PartCount;
	NAND_PARTITION      array[ND_MAX_PARTITION_COUNT];	//
} PARTITION_MBR;


struct _nand_lib_cfg{
	unsigned int		    phy_interface_cfg;

	unsigned int		    phy_support_two_plane;
	unsigned int		    phy_nand_support_vertical_interleave;
	unsigned int		    phy_support_dual_channel;

	unsigned int		    phy_wait_rb_before;
	unsigned int		    phy_wait_rb_mode;
	unsigned int		    phy_wait_dma_mode;
};

struct _nand_super_block{
	unsigned short  Block_NO;
	unsigned short  Chip_NO;
};

struct _nand_disk{
	unsigned int		size;
	//unsigned int offset;
	unsigned int		type;
	unsigned  char      name[PARTITION_NAME_SIZE];
};

struct _partition {
	struct _nand_disk nand_disk[MAX_PART_COUNT_PER_FTL];
	unsigned int size;
	unsigned int cross_talk;
	unsigned int attribute;
	struct _nand_super_block start;
	struct _nand_super_block end;
	//unsigned int offset;
};

typedef union {
	unsigned char ndata[4096];
	PARTITION_MBR data;
} _MBR;

typedef union {
	unsigned char ndata[2048 + 512];
	struct _partition data[MAX_PARTITION];
} _PARTITION;

typedef union {
	unsigned char ndata[512];
	struct _spinand_config_para_info config;
} _NAND_STORAGE_INFO;

typedef union {
	unsigned char ndata[2048];
	struct _nand_super_block data[512];
} _FACTORY_BLOCK;

typedef union {
	unsigned char ndata[256];
	struct _uboot_info data;
} _UBOOT_INFO;

typedef union {
	unsigned char ndata[1024];
	unsigned char data[1024];
} _NAND_SPECIAL_INFO;


struct _boot_info {
	unsigned int magic;
	unsigned int len;
	unsigned int sum;

	unsigned int no_use_block;
	unsigned int uboot_start_block;
	unsigned int uboot_next_block;
	unsigned int logic_start_block;
	unsigned int nand_specialinfo_page;
	unsigned int nand_specialinfo_offset;
	unsigned int physic_block_reserved;
	unsigned int nand_ddrtype;
	unsigned int ddr_timing_cfg;
	unsigned int enable_crc;// ENABLE_CRC_MAGIC
	unsigned int nouse[128 - 13];

	_MBR mbr;                           //4k               offset 0.5k
	_PARTITION partition;               //2.5k             offset 4.5k
	_NAND_STORAGE_INFO storage_info;    //0.5k             offset 7k
	_FACTORY_BLOCK factory_block;       //2k               offset 7.5k
	//_UBOOT_INFO uboot_info;             //0.25K
	_NAND_SPECIAL_INFO  nand_special_info;  //1k               offset 9.5k
};

//全局flash属性
struct _nand_info{
	unsigned short            type;
	unsigned short            SectorNumsPerPage;
	unsigned short            BytesUserData;
	unsigned short            PageNumsPerBlk;
	unsigned short            BlkPerChip;
	unsigned short            ChipNum;
	unsigned short            FirstBuild;
	unsigned short            new_bad_page_addr;
	unsigned long long        FullBitmap;
	struct _nand_super_block  mbr_block_addr;
	struct _nand_super_block  bad_block_addr;
	struct _nand_super_block  new_bad_block_addr;
	struct _nand_super_block  no_used_block_addr;
	struct _nand_super_block* factory_bad_block;
	struct _nand_super_block* new_bad_block;
	unsigned char*		  temp_page_buf;
	unsigned char*		  mbr_data;
	struct _nand_phy_partition* phy_partition_head;
	struct _partition         partition[MAX_PARTITION];
	struct _nand_lib_cfg	  nand_lib_cfg;
	unsigned short		  partition_nums;
	unsigned short		  cache_level;
	unsigned short		  capacity_level;

	unsigned short mini_free_block_first_reserved;
	unsigned short mini_free_block_reserved;

	unsigned int MaxBlkEraseTimes;
	unsigned int EnableReadReclaim;

	unsigned int read_claim_interval;

	struct _boot_info *boot;
};

//==========================================================
//nand phy partition 访问接口

struct _nand_phy_partition{
	unsigned short PartitionNO;
	unsigned short CrossTalk;
	unsigned short SectorNumsPerPage;
	unsigned short BytesUserData;
	unsigned short PageNumsPerBlk;
	unsigned short TotalBlkNum;	  //include bad block
	unsigned short GoodBlockNum;
	unsigned short FullBitmapPerPage;
	unsigned short FreeBlock;
	unsigned int Attribute;
	unsigned int TotalSectors;
	struct _nand_super_block StartBlock;
	struct _nand_super_block EndBlock;
	struct _nand_info *nand_info;
	struct _nand_super_block *factory_bad_block;
	struct _nand_super_block *new_bad_block;
	struct _nand_phy_partition *next_phy_partition;
	struct _nand_disk *disk;

	int (*page_read)(unsigned short nDieNum, unsigned short nBlkNum,
			unsigned short nPage, unsigned short SectBitmap,
			void *pBuf, void *pSpare);
	int (*page_write)(unsigned short nDieNum, unsigned short nBlkNum,
			unsigned short nPage, unsigned short SectBitmap,
			void *pBuf, void *pSpare);
	int (*block_erase)(unsigned short nDieNum, unsigned short nBlkNum);
};

//==========================================================
//nand partition 访问接口

struct _nand_partition_page{
    unsigned short  Page_NO;
    unsigned short  Block_NO;
};

struct _physic_par{
    struct _nand_partition_page	 phy_page;
    unsigned short	       page_bitmap;
    unsigned char*	       main_data_addr;
    unsigned char*	       spare_data_addr;
};

//nand partition
struct _nand_partition{
	char			    name[32];
	unsigned short		  sectors_per_page;
	unsigned short		  spare_bytes;
	unsigned short		  pages_per_block;
	unsigned short		  total_blocks;
	unsigned short		  bytes_per_page;
	unsigned int		    bytes_per_block;
	unsigned short		  full_bitmap;
	unsigned long long	      cap_by_sectors;
	unsigned long long	      cap_by_bytes;
	unsigned long long	      total_by_bytes;
	struct _nand_phy_partition	*phy_partition;

	int (*nand_erase_superblk)(struct _nand_partition* nand,struct _physic_par *p);
	int (*nand_read_page)(struct _nand_partition* nand,struct _physic_par *p);
	int (*nand_write_page)(struct _nand_partition* nand,struct _physic_par *p);
	int (*nand_is_blk_good)(struct _nand_partition* nand,struct _physic_par *p);
	int (*nand_mark_bad_blk)(struct _nand_partition* nand,struct _physic_par *p);
};


#endif
