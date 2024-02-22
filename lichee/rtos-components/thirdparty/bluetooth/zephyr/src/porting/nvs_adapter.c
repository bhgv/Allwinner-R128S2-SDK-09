/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <zephyr.h>
#include <blkpart.h>
#include <../../hal/source/spinor/inter.h>

#define BT_DBG_ENABLED IS_ENABLED(0)
#define LOG_MODULE_NAME nvs_adapter
#include "common/log.h"

#define SETTINGS_NVS_PART_PATH "/dev/settings"

static const struct flash_parameters fix_param = {
	.write_block_size = 32,
	.erase_value = 0xff,
};

int flash_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	int ret;
	ret = nor_read(offset, data, len);
	return ret;
}

int flash_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	int ret;
	ret = nor_write(offset, (char *)data, len);
	return ret;
}

int flash_erase(const struct device *dev, off_t offset, size_t size)
{
	int ret;
	ret = nor_erase(offset, size);
	return ret;
}

/* cannot fetch from flash driver api, so make it const param */
const struct flash_parameters *flash_get_parameters(const struct device *dev)
{
	return &fix_param;
}

/* cannot fetch from flash driver api, so make it const param */
size_t flash_get_write_block_size(const struct device *dev)
{
	return fix_param.write_block_size;
}

/* get flash info: flash min_erased_size(sector size) */
int flash_get_page_info_by_offs(const struct device *dev, off_t offset, struct flash_pages_info *info)
{
	struct part *flash_part = NULL;

	if (strncmp(SETTINGS_NVS_PART_PATH, "/dev/", sizeof("/dev/") - 1)) {
		LOG_ERR("path of device must start with '/dev/': %s\n", SETTINGS_NVS_PART_PATH);
		return -EINVAL;
	}
	flash_part = get_part_by_name(SETTINGS_NVS_PART_PATH + sizeof("/dev/") - 1);
	if (!flash_part) {
		LOG_ERR("not found device %s\n", SETTINGS_NVS_PART_PATH);
		return -ENODEV;
	}
	info->size = flash_part->blk->blk_bytes;

	return 0;
}

/* get flash info: partition_size/partition_off */
int flash_area_open(uint8_t id, const struct flash_area **fa)
{
	struct part *flash_part = NULL;
	static struct flash_area flash_area_inst;

	if (strncmp(SETTINGS_NVS_PART_PATH, "/dev/", sizeof("/dev/") - 1)) {
		LOG_ERR("path of device must start with '/dev/': %s\n", SETTINGS_NVS_PART_PATH);
		return -EINVAL;
	}
	flash_part = get_part_by_name(SETTINGS_NVS_PART_PATH + sizeof("/dev/") - 1);
	if (!flash_part) {
		LOG_ERR("not found device %s\n", SETTINGS_NVS_PART_PATH);
		return -ENODEV;
	}
	flash_area_inst.fa_size = flash_part->bytes;
	flash_area_inst.fa_off = flash_part->off;

	*fa = &flash_area_inst;

	return 0;
}

/* get flash info: sector_size */
int flash_area_get_sectors(int fa_id, uint32_t *count, struct flash_sector *sectors)
{
	struct part *flash_part = NULL;

	if (strncmp(SETTINGS_NVS_PART_PATH, "/dev/", sizeof("/dev/") - 1)) {
		LOG_ERR("path of device must start with '/dev/': %s\n", SETTINGS_NVS_PART_PATH);
		return -EINVAL;
	}
	flash_part = get_part_by_name(SETTINGS_NVS_PART_PATH + sizeof("/dev/") - 1);
	if (!flash_part) {
		LOG_ERR("not found device %s\n", SETTINGS_NVS_PART_PATH);
		return -ENODEV;
	}
	sectors->fs_size = flash_part->blk->blk_bytes;

	return 0;
}