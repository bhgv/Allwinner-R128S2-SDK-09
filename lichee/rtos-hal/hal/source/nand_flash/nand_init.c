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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <aw_common.h>
#include <blkpart.h>
#include <part_efi.h>
#include "nand_cfg.h"
#include "nand_nftl.h"
#include "nand_osal.h"
#include "nand_struct.h"
#include "build_nand_partition.h"

#include "nand_physic_interface.h"
#include "nand_inc.h"
#include "nand_info_init.h"
#include "blkpart.h"
#include "nand_log.h"

#ifdef CONFIG_KERNEL_FREERTOS
#include <awlog.h>
#elif defined(CONFIG_OS_MELIS)
#include <log.h>
#endif

#include <hal_mutex.h>
#include <hal_thread.h>

#ifndef SECTOR_SHIFT
#define SECTOR_SHIFT 9
#endif

#ifndef SECTOR_SIZE
#define SECTOR_SIZE (1 << 9)
#endif

#define check_align(sz, align_sz) (!(sz & (align_sz - 1)))
#define byte_to_sector(sz) ((sz) >> SECTOR_SHIFT)
#define sector_to_byte(sz) ((sz) << SECTOR_SHIFT)
#define dev_to_blk(dev) (dev->nftl_blk)
#define time_after(a,b) ((long)((b) - (a)) < 0)

uint16 get_partitionNO(struct _nand_phy_partition *phy_partition);
struct _nand_disk *get_disk_from_phy_partition(struct _nand_phy_partition *phy_partition);

static struct nand_dev {
	struct _nand_info *nand_info;
	struct _nftl_blk *nftl_blk;
	hal_mutex_t lock;
	long gc_timer;
	long active_timer;
	long read_active_timer;
} *nand_dev;

static int inline nand_lock_init(void)
{
    nand_dev->lock = hal_mutex_create();
	if (nand_dev->lock) {
		return 0;
	}
	return -1;
}

static int inline nand_lock_exit(void)
{
	return hal_mutex_delete(nand_dev->lock);
}

static int inline nand_lock(void)
{
	return hal_mutex_lock(nand_dev->lock);
}

static int inline nand_unlock(void)
{
	return hal_mutex_unlock(nand_dev->lock);
}

static int nand_create_task(const char *name, void (*fn)(void *), void *arg)
{
	if (hal_thread_create(fn, arg, name, 4096, 20)) {
		return 0;
	}
	return -1;
}

static int nand_delete_task(char *name)
{
	return hal_thread_stop(name);
}

static void nand_task_exit(int exit)
{
	return hal_thread_stop(NULL);
}

static void nand_sleep(unsigned int sec)
{
	sleep(sec);
}

static int nand_static_wear_leveling(void)
{
	static int swl_done = 0, swl_time = 0, first_miss_swl = 0;
	struct _nftl_blk *blk = nand_dev->nftl_blk;
	int need_swl;

	if (swl_done) {
		if (time_after(nand_dev->gc_timer, swl_time + 64))
			need_swl = 1;	/* next static WL is over 64s */
	} else if (time_after(nand_dev->gc_timer,
				nand_dev->active_timer + 4)) {
		/* nftl is idle over 4s */
		need_swl = 1;
	} else if (first_miss_swl) {
		if (time_after(nand_dev->gc_timer, swl_time + 64 ))
			need_swl = 1;	/* next static WL is over 64s */
	} else {
		first_miss_swl = 1;
		swl_time = nand_dev->gc_timer;
	}

	if (need_swl) {
		first_miss_swl = 0;
		need_swl = 0;
		swl_done = blk->static_wear_leveling(blk);
		if (!swl_done) {
			swl_done = 1;
			swl_time = nand_dev->gc_timer;
		} else
			swl_done = 0;
	}
	return 0;
}

static int nand_dynamic_gc(void)
{
	struct _nftl_blk *blk = nand_dev->nftl_blk;

	/*
	 * deep gc for small spinand
	 * In order to recover small spinand speed with DISCARD NOT OK
	 * YET, we can do deep gc. It means gc more block dynamically
	 * according to how many free blocks. The fewer free blocks,
	 * the more recycled.
	 * As far as we know, the speed problem is only issued on small
	 * nand, for example 128MB. So, it works only when NAND has
	 * samll capacity (128M).
	 * Avoid to reduce speed for userspace, we do it when in idle.
	 */
	/* nand 10 min idle */
	if (time_after(nand_dev->gc_timer,
				nand_dev->active_timer + (10 * 60))) {
		int ret;
		/*
		 * gc with invalid_page_count = (page_per_block / 2)
		 *
		 * gc_all_enhance may take a few seconds. To avoid
		 * reduce read/write speed, do it only when nand is in
		 * idle for 10min
		 */
		ret = blk->dynamic_gc(blk, true);
		if (ret)
			pr_err("nftl_thread: enhance gc all error\n");
	/* read nand 8 sec idle */
	} else if (time_after(nand_dev->gc_timer,
				nand_dev->read_active_timer + 8)) {
		int ret;
		/*
		 * gc according to free block count
		 * invalid page cnt between 32 to 61
		 *
		 * gc_all_base_on_free_blks just needs to check
		 * read_active_time rather than active_time. Because
		 * if some apps, like syslogd, keep writing, it may
		 * never be idle if active_time.
		 * Write operation allways takes time to do gc, it's no
		 * matter to do more here.
		 */
		ret = blk->dynamic_gc(blk, false);
		if (ret)
			pr_err("nftl_thread: dynamic gc all error\n");
	}
	return 0;
}

static void nftl_gc_thread(void *unused)
{
	struct _nftl_blk *blk = nand_dev->nftl_blk;

	if (!blk || !blk->reclaim)
		nand_task_exit(-EINVAL);

	nand_dev->gc_timer = 0;
	while (1) {
		nand_dev->gc_timer++;

		nand_lock();

		nand_static_wear_leveling();
		blk->garbage_collect(blk);
		nand_dynamic_gc();

		nand_unlock();

		nand_sleep(1);
	}

	nand_task_exit(0);
}

static void nand_rc_thread(void *unused)
{
	struct _nftl_blk *blk = nand_dev->nftl_blk;
	unsigned int start_time = 600;

	if (!blk->reclaim)
		nand_task_exit(-EINVAL);

	while (1) {
		nand_sleep(1);

		if (start_time) {
			start_time--;
			continue;
		}

		nand_lock();
		blk->reclaim(blk);
		nand_unlock();
	}

	nand_task_exit(0);
}

int nand_read(unsigned int addr, char *buf, unsigned int len)
{
	int ret;
	struct _nftl_blk *blk;

	pr_debug("try to read addr 0x%x len %u\n", addr, len);
	if (!nand_dev)
		return -EBUSY;
	if (!len)
		return 0;
	if (addr && !check_align(addr, SECTOR_SIZE)) {
		pr_err("addr %u must align to SECTOR(512B)\n", addr);
		goto err;
	}
	if (!check_align(len, SECTOR_SIZE)) {
		pr_err("len %u must align to SECTOR(512B)\n", len);
		goto err;
	}

	blk = dev_to_blk(nand_dev);
	if (byte_to_sector(addr + len) > blk->logic_sects) {
		pr_err("read over boundary\n");
		goto err;
	}

	nand_dev->active_timer = nand_dev->gc_timer;
	nand_dev->read_active_timer = nand_dev->gc_timer;

	nand_lock();
	ret = blk->read_data(blk, byte_to_sector(addr),
			byte_to_sector(len), (unsigned char *)buf);
	nand_unlock();
	if (!ret)
		return ret;
err:
	pr_err("read addr %u len %u failed\n", addr, len);
	return -1;
}

int nand_write(unsigned int addr, char *buf, unsigned int len)
{
	int ret;
	struct _nftl_blk *blk;

	pr_debug("try to write addr 0x%x len %u\n", addr, len);
	if (!nand_dev)
		return -EBUSY;

	if (!len)
		return 0;
	if (addr && !check_align(addr, SECTOR_SIZE)) {
		pr_err("addr %u must align to SECTOR(512B)\n", addr);
		goto err;
	}
	if (!check_align(len, SECTOR_SIZE)) {
		pr_err("len %u must align to SECTOR(512B)\n", len);
		goto err;
	}

	blk = dev_to_blk(nand_dev);
	if (byte_to_sector(addr + len) > blk->logic_sects) {
		pr_err("write over boundary\n");
		goto err;
	}

	nand_dev->active_timer = nand_dev->gc_timer;

	nand_lock();
	ret = blk->write_data(blk, byte_to_sector(addr),
			byte_to_sector(len), (unsigned char *)buf);
	if (!ret)
		blk->flush_write_cache(blk, 0xFFFFFFFF);
	nand_unlock();
	if (!ret)
		return ret;
err:
	pr_err("read addr %u len %u failed\n", addr, len);
	return -1;
}

int nand_erase(unsigned int addr, unsigned int size)
{
	return 0;
}

#define CONFIG_COMPONENTS_AW_BLKPART 1
#ifdef CONFIG_COMPONENTS_AW_BLKPART
static struct blkpart nandblk;
static int nand_blkpart_init(void)
{
	int ret, index;
	struct part *part;
	struct _nftl_blk *blk = nand_dev->nftl_blk;
	struct _nand_info *info = nand_dev->nand_info;
	struct _nand_phy_partition *phy_part;
	struct _nand_disk *disk;

	memset(&nandblk, 0, sizeof(struct blkpart));
	nandblk.name = "nand";
	nandblk.erase = nand_erase;
	nandblk.program = nand_write;
	nandblk.read = nand_read;
	nandblk.total_bytes = sector_to_byte(blk->logic_sects);
#ifdef CONFIG_OS_MELIS
    nandblk.dev = rt_device_find("nandflash");
#endif
	/*
	 * No need to set actual block size here. The upper layer will erase
	 * for block and FS (lfs) may set it as file storage unit. However,
	 * there is NFTL here, no actual block any more.
	 * Take am example: if we set 128K as block size, one 12B file of
	 * FS(lfs) may occupy 128K, that is really wasteful.
	 */
	nandblk.blk_bytes = sector_to_byte(blk->logic_page_sects);
	nandblk.page_bytes = sector_to_byte(blk->logic_page_sects);
	pr_debug("total %u KB\n", nandblk.total_bytes/ 1024);
	pr_debug("blk bytes %u KB\n", nandblk.blk_bytes / 1024);
	pr_debug("page bytes %u KB\n", nandblk.page_bytes / 1024);

	phy_part = get_head_phy_partition_from_nand_info(info);
	disk = get_disk_from_phy_partition(phy_part);
	for (index = 0; disk[index].name[0] != 0xFF; index++)
		nandblk.n_parts++;
	nandblk.parts = malloc(nandblk.n_parts * sizeof(struct part));
	if (!nandblk.parts)
		goto err;

	for (index = 0; index < nandblk.n_parts; index++) {
		part = &nandblk.parts[index];
		snprintf(part->name, MAX_BLKNAME_LEN, "%s", disk[index].name);
		part->bytes = sector_to_byte(disk[index].size);
		part->off = BLKPART_OFF_APPEND;
		pr_debug("add part %s size %u\n", part->name, part->bytes);
	}
	/* set size of the last part to all remaining space */
	nandblk.parts[--index].bytes = BLKPART_SIZ_FULL;

	ret = add_blkpart(&nandblk);
	if (ret)
		goto free_parts;

	/* check bytes align */
	for (index = 0; index < nandblk.n_parts; index++) {
		part = &nandblk.parts[index];
		if (part->bytes % nandblk.blk_bytes) {
			pr_err("part %s with bytes %u should align to block size %u\n",
			       part->name, part->bytes, nandblk.blk_bytes);
			goto del_blk;
		}
	}

	return 0;

del_blk:
	del_blkpart(&nandblk);
free_parts:
	free(nandblk.parts);
err:
	pr_err("init blkpart for nand failed - %d\n", ret);
	return ret;
}
#endif

int hal_nand_init(void)
{
	int ret = -EINVAL;
	struct _nand_info *nand_info;
	struct _nftl_blk *nftl_blk;
	unsigned short part_no;

	if (nand_dev)
		return -EBUSY;
	nand_dev = malloc(sizeof(*nand_dev));
	if (!nand_dev)
		return -ENOMEM;

	nand_info = NandHwInit();
	if (!nand_info)
		goto free_dev;

	set_cache_level(nand_info, NAND_CACHE_LEVEL);
	set_capacity_level(nand_info, NAND_CAPACITY_LEVEL);
	ret = nand_info_init(nand_info, 0, 8, NULL);
	if (ret)
		goto hw_exit;

	nftl_blk = malloc(sizeof(*nftl_blk));
	if (!nftl_blk) {
		ret = -ENOMEM;
		goto hw_exit;
	}

	nftl_blk->nand = build_nand_partition(nand_info->phy_partition_head);
	if (!nftl_blk->nand)
		goto free_blk;

	part_no = get_partitionNO(nand_info->phy_partition_head);
	ret = nftl_init(nftl_blk, part_no);
	if (ret)
		goto free_part;

	ret = nand_lock_init();
	if (ret)
		goto nftl_exit;

	nand_dev->nand_info = nand_info;
	nand_dev->nftl_blk = nftl_blk;

	ret = nand_create_task("nftld", nftl_gc_thread, NULL);
	if (ret)
		goto del_lock;

	ret = nand_create_task("nand_rcd", nand_rc_thread, NULL);
	if (ret)
		goto del_nftld_task;

	ret = nand_dbg_init(nftl_blk);
	if (ret)
		goto del_nandrc_task;

#ifdef CONFIG_COMPONENTS_AW_BLKPART
	ret = nand_blkpart_init();
	if (ret)
		goto del_nftld_task;
#endif

	pr_info("Nand Init OK\n");
	return 0;

#if 0 /* reserved for future */
dbg_exit:
	nand_dbg_exit();
#endif
del_nandrc_task:
	nand_delete_task("nand_rcd");
del_nftld_task:
	nand_delete_task("nftld");
del_lock:
	nand_lock_exit();
nftl_exit:
	nftl_exit(nftl_blk);
free_part:
	free_nand_partition(nftl_blk->nand);
free_blk:
	free(nftl_blk);
hw_exit:
	NandHwExit();
free_dev:
	free(nand_dev);
	pr_info("Nand Init FAILED\n");
	return ret;
}

void nand_exit(void)
{
	struct _nftl_blk *blk;

	if (!nand_dev)
		return;
	/*
	 * do not delete lock in case of other thread is holding lock
	 * we ge lock here to wait for other thread end.
	 */
	nand_lock();
	blk = dev_to_blk(nand_dev);
	blk->flush_write_cache(blk, 0xFFFFFFFF);
	nand_dev = NULL;
}

#ifdef AOS_COMP_LITTLEFS
int32_t littlefs_fetch_cfg_param(struct littlefs_cfg_param *cfg_param)
{
	struct part *part = get_part_by_name("UDISK");

	if (!part)
		return -ENODEV;

	cfg_param->read_size = part->blk->page_bytes;
	cfg_param->prog_size = part->blk->page_bytes;
	cfg_param->block_size = part->blk->blk_bytes;
	cfg_param->block_count = part->bytes / cfg_param->block_size;
	cfg_param->cache_size = cfg_param->block_size;
	cfg_param->lookahead_size = 256;
	/* nand with nftl no need block-level wear-leveling */
	cfg_param->block_cycles = -1;
	/* no need to validate data */
	/* cfg_param->validate = -1; */
	return 0;
}
#endif
