#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sunxi_secure_storage_warpper.h"
#include "sunxi_secure_storage.h"
#include "sunxi_hal_common.h"
#include "hal_mem.h"

#define pr_err printf
#define pr_msg(...) do { } while(0)
#define pr_force printf

static int __probe_name_in_map(unsigned char *buffer, const char *item_name, int *len)
{
	unsigned char *buf_start = buffer;
	int index		 = 1;
    unsigned char name[MAP_KEY_NAME_SIZE];
    unsigned char length[MAP_KEY_DATA_SIZE];
	int i, j;

	while (*buf_start != '\0' && (buf_start - buffer) < SEC_BLK_SIZE) {
		memset(name, 0, MAP_KEY_NAME_SIZE);
		memset(length, 0, MAP_KEY_DATA_SIZE);
		i = j = 0;
		while (buf_start[i] != ':' && (buf_start[i] != '\0') &&
		       (&buf_start[i] - buffer) < SEC_BLK_SIZE &&
		       j < MAP_KEY_NAME_SIZE) {
			name[j] = buf_start[i];
			i++;
			j++;
		}

		if (j >= MAP_KEY_NAME_SIZE)
			return -1;

		i++;
		j = 0;
		while ((buf_start[i] != ' ') && (buf_start[i] != '\0') &&
		       (&buf_start[i] - buffer) < SEC_BLK_SIZE &&
		       j < MAP_KEY_DATA_SIZE) {
			length[j] = buf_start[i];
			i++;
			j++;
		}

		/* deal dirty data */
		if ((&buf_start[i] - buffer) >= SEC_BLK_SIZE ||
		    j >= MAP_KEY_DATA_SIZE) {
			return -1;
		}

		if (!strcmp(item_name, (const char *)name)) {
			buf_start += strlen(item_name) + 1;
			*len = strtoul((const char *)length, NULL, 10);

			if (strlen(item_name) ==
				    strlen(SECURE_STORAGE_DUMMY_KEY_NAME) &&
			    !memcmp(item_name, SECURE_STORAGE_DUMMY_KEY_NAME,
				    strlen(SECURE_STORAGE_DUMMY_KEY_NAME)) &&
			    *len == 0) {
				/*
				 * if *len == 0 it is a actual DUMMY_KEY,
				 * a key happen to has same name should have a non-zero len
				 */
			} else {
				pr_msg("name in map %s\r\n", name);
				return index;
			}
		}
		index++;
		buf_start += strlen((const char *)buf_start) + 1;
	}

	return -1;
}

static int __fill_name_in_map(unsigned char *buffer, const char *item_name, int length)
{
	unsigned char *buf_start = buffer;
	int index		 = 1;
	int name_len;
	uint8_t write_back_buf[sizeof(struct map_info)];
	int dummy_key_index		= -1;
	unsigned char *dummy_key_start	= NULL;
	unsigned char *write_back_start = NULL;

	while (*buf_start != '\0' && (buf_start - buffer) < SEC_BLK_SIZE) {
		name_len = 0;
		while (buf_start[name_len] != ':' &&
		       (&buf_start[name_len] - buffer) < SEC_BLK_SIZE &&
		       name_len < MAP_KEY_NAME_SIZE)
			name_len++;

		/* deal dirty data */
		if ((&buf_start[name_len] - buffer) >= SEC_BLK_SIZE ||
		    name_len >= MAP_KEY_NAME_SIZE) {
			pr_msg("__fill_name_in_map: dirty map, memset 0\r\n");
			memset(buffer, 0x0, SEC_BLK_SIZE);
			buf_start = buffer;
			index     = 1;
			break;
		}

		if (!memcmp((const char *)buf_start, item_name, name_len) &&
		    strlen(item_name) == name_len) {
			pr_msg("name in map %s\r\n", buf_start);
			return index;
		}

		if (name_len == strlen(SECURE_STORAGE_DUMMY_KEY_NAME) &&
		    !memcmp((const char *)buf_start,
			    SECURE_STORAGE_DUMMY_KEY_NAME, name_len) &&
		    dummy_key_index == -1) {
			/*
			 * DUMMY_KEY could be replaced with the input key,
			 * but we dont know whether to do so at this point,
			 * save relate info first
			 */
			pr_msg("found dummy_key %s\r\n", buf_start);
			dummy_key_index	 = index;
			dummy_key_start	 = buf_start;
			write_back_start = dummy_key_start +
					   strlen((const char *)buf_start) + 1;
			memset(write_back_buf, 0, sizeof(write_back_buf));
			memcpy((char *)write_back_buf, write_back_start,
			       4096 - (write_back_start - buffer));
		}
		index++;
		buf_start += strlen((const char *)buf_start) + 1;
	}
	if ((index >= 32) && (dummy_key_index == -1))
		return -1;

	if (dummy_key_index != -1) {
		/*use index reserved by DUMMY_KEY*/
		sprintf((char *)dummy_key_start, "%s:%d", item_name, length);
		write_back_start = dummy_key_start +
				   strlen((const char *)dummy_key_start) + 1;
		memcpy(write_back_start, write_back_buf,
		       4096 - (write_back_start - buffer));
		return dummy_key_index;
	} else {
		/* add new index */
		sprintf((char *)buf_start, "%s:%d", item_name, length);
		return index;
	}
}

static int check_secure_storage_map(void *buffer)
{
	struct map_info *map_buf = (struct map_info *)buffer;

	if (map_buf->magic != STORE_OBJECT_MAGIC) {
		pr_err("Item0 (Map) magic is bad\r\n");
		return 2;
	}
	if (map_buf->crc !=
	    crc32(0, (void *)map_buf, sizeof(struct map_info) - 4)) {
		pr_err("Item0 (Map) crc is fail [0x%x]\r\n", map_buf->crc);
		return -1;
	}
	return 0;
}

static unsigned int map_dirty;

static inline void set_map_dirty(void)
{
	map_dirty = 1;
}
static inline void clear_map_dirty(void)
{
	map_dirty = 0;
}
static inline int try_map_dirty(void)
{
	return map_dirty;
}

static __attribute__((aligned(CACHELINE_LEN))) struct map_info secure_storage_map = { { 0 } };
unsigned int sunxi_secure_storage_inited = 0;

int sunxi_secure_storage_is_init(void)
{
    return sunxi_secure_storage_inited;
}

int sunxi_secure_storage_init(void)
{
	int ret;

	if (!sunxi_secure_storage_inited) {
		ret = sunxi_secstorage_read(
			0, (unsigned char *)&secure_storage_map, 4096);
		if (ret < 0) {
			pr_err("get secure storage map err\r\n");

			return -1;
		}

		ret = check_secure_storage_map(&secure_storage_map);
		if (ret == -1) {
			if ((secure_storage_map.data[0] == 0xff) ||
			    (secure_storage_map.data[0] == 0x0))
				memset(&secure_storage_map, 0, SEC_BLK_SIZE);
		} else if (ret == 2) {
			if ((secure_storage_map.data[0] == 0xff) ||
			    (secure_storage_map.data[0] == 0x00)) {
				pr_msg("the secure storage map is empty\r\n");
				memset(&secure_storage_map, 0, SEC_BLK_SIZE);
			} else {
				/* no things */
			}
		}
	}
	sunxi_secure_storage_inited = 1;

	return 0;
}

int sunxi_secure_storage_read(const char *item_name, char *buffer,
			      int buffer_len, int *data_len)
{
	int ret, index, len_in_store;
    //unsigned char __attribute__((aligned(CACHELINE_LEN))) align_buffer[4096];
	unsigned char *align_buffer = hal_malloc_align(4096, CACHELINE_LEN);

	if (!align_buffer) {
		pr_err("%s err: hal_malloc_align failed!\r\n", __func__);
		return -1;
	}
	if (!sunxi_secure_storage_inited) {
		pr_err("%s err: secure storage has not been inited\r\n", __func__);
		ret = -1;
		goto exit;
	}
	index = __probe_name_in_map((unsigned char *)&secure_storage_map,
				    item_name, &len_in_store);
	if (index < 0) {
		pr_err("no item name %s in the map\r\n", item_name);
		ret = -2;
		goto exit;
	}
	memset(align_buffer, 0, 4096);
	ret = sunxi_secstorage_read(index, align_buffer, 4096);
	if (ret < 0) {
		pr_err("read secure storage block %d name %s err\r\n", index, item_name);
		ret = -3;
		goto exit;
	}
	if (len_in_store > buffer_len) {
		memcpy(buffer, align_buffer, buffer_len);
	} else {
		memcpy(buffer, align_buffer, len_in_store);
	}
	*data_len = len_in_store;

	ret = 0;
exit:
	hal_free_align(align_buffer);
	return ret;
}

int sunxi_secure_storage_erase_data_only(const char *item_name)
{
	int	   ret, index, len;
	//unsigned char __attribute__((aligned(CACHELINE_LEN))) align_buffer[4096];
	unsigned char *align_buffer = hal_malloc_align(4096, CACHELINE_LEN);

	if (!align_buffer) {
		pr_err("%s err: hal_malloc_align failed!\r\n", __func__);
		return -1;
	}
	if (!sunxi_secure_storage_inited) {
		pr_err("%s err: secure storage has not been inited\r\n", __func__);
		ret = -1;
		goto exit;
	}
	index = __probe_name_in_map((unsigned char *)&secure_storage_map,
				    item_name, &len);
	if (index < 0) {
		pr_err("no item name %s in the map\r\n", item_name);
		ret = -2;
		goto exit;
	}
	memset(align_buffer, 0xff, 4096);
	ret = sunxi_secstorage_write(index, align_buffer, 4096);
	if (ret < 0) {
		pr_err("erase secure storage block %d name %s err\r\n", index, item_name);
		ret = -1;
		goto exit;
	}
	set_map_dirty();
	pr_force("erase secure storage: %d data only ok\r\n", index);

	ret = 0;
exit:
	hal_free_align(align_buffer);
	return ret;
}

int sunxi_secure_storage_erase_all(void)
{
	memset(&secure_storage_map, 0, 4096);
	set_map_dirty();
	return 0;
}

int sunxi_secure_storage_write(const char *item_name, const char *buffer, int length)
{
	int  ret, index;
	int  len = 0;
    //unsigned char __attribute__((aligned(CACHELINE_LEN))) align_buffer[4096];
	unsigned char *align_buffer = hal_malloc_align(4096, CACHELINE_LEN);

	if (!align_buffer) {
		pr_err("%s err: hal_malloc_align failed!\r\n", __func__);
		return -1;
	}
	if (!sunxi_secure_storage_inited) {
		pr_err("%s err: secure storage has not been inited\r\n", __func__);
		ret = -1;
		goto exit;
	}
	index = __probe_name_in_map((unsigned char *)&secure_storage_map,
				    item_name, &len);
	if (index < 0) {
		index = __fill_name_in_map((unsigned char *)&secure_storage_map,
					   item_name, length);
		if (index < 0) {
			pr_err("write secure storage block %d name %s overrage\r\n",
			       index, item_name);
			ret = -1;
			goto exit;
		}
	} else {
		pr_force("There is the same name in the secure storage, try to erase it\r\n");
		if (len != length) {
			pr_err("the length is not match with key has store in secure storage\r\n");
			ret = -1;
			goto exit;
		} else {
			if (sunxi_secure_storage_erase_data_only(item_name) <
			    0) {
				pr_err("Erase item %s fail\r\n", item_name);
				ret = -1;
				goto exit;
			}
		}
	}
	if(length>0 && length <= 4096)
		memcpy(align_buffer, buffer, length);
	else
		pr_err("length(%d) error!\r\n", length);
	if((4096 - length) > 0)
		memset(&align_buffer[length], 0x0, 4096 - length);
	ret = sunxi_secstorage_write(index, (unsigned char *)align_buffer, 4096);
	if (ret < 0) {
		pr_err("write secure storage block %d name %s err\r\n", index,
		       item_name);
		ret = -1;
		goto exit;
	}
	set_map_dirty();
	pr_force("write secure storage: %d ok\r\n", index);

	ret = 0;
exit:
	hal_free_align(align_buffer);
	return ret;
}

int sunxi_secure_storage_exit(void)
{
	int ret;

	if (!sunxi_secure_storage_inited) {
		pr_err("%s err: secure storage has not been inited\r\n", __func__);
		return -1;
	}

	if (try_map_dirty()) {
		secure_storage_map.magic = STORE_OBJECT_MAGIC;
		secure_storage_map.crc   = crc32(0, (void *)&secure_storage_map,
					       sizeof(struct map_info) - 4);
		ret = sunxi_secstorage_write(0, (unsigned char *)&secure_storage_map, 4096);
		if (ret < 0) {
			pr_err("write secure storage map\r\n");
			return -1;
		}
	}
	clear_map_dirty();
	ret = sunxi_secstorage_flush();
	sunxi_secure_storage_inited = 0;

	return ret;
}

int sunxi_secure_storage_get_name_by_index(const int index, char *buffer, const int buffer_size)
{
	int cur_index = 1;
	unsigned char *buf_start = (unsigned char *)&secure_storage_map;

	if (!sunxi_secure_storage_inited) {
		pr_err("%s secure storage not init\r\n", __func__);
		return -1;
	}

	char name[MAP_KEY_NAME_SIZE];
	char length[MAP_KEY_DATA_SIZE];
	int  i, j, len;

	while (*buf_start != '\0' && cur_index <= index) {
		memset(name, 0, MAP_KEY_NAME_SIZE);
		memset(length, 0, MAP_KEY_DATA_SIZE);
		i = 0;
		while (buf_start[i] != ':') {
			name[i] = buf_start[i];
			i++;
		}
		i++;
		j = 0;
		while ((buf_start[i] != ' ') && (buf_start[i] != '\0')) {
			length[j] = buf_start[i];
			i++;
			j++;
		}

		len = strtoul((const char *)length, NULL, 10);

		if (strlen(name) == strlen(SECURE_STORAGE_DUMMY_KEY_NAME) &&
		    !memcmp(name, SECURE_STORAGE_DUMMY_KEY_NAME,
			    strlen(SECURE_STORAGE_DUMMY_KEY_NAME)) &&
		    len == 0) {
			/*dummy key, not used, goto next key*/
			goto next_key;
		}

		if(cur_index == index){
			int name_len = strlen(name) + 1;
			if(name_len > buffer_size)
				return -1;
			memcpy(buffer, name, name_len);
			return 0;
		}

next_key:
		cur_index++;
		buf_start += strlen((const char *)buf_start) + 1;
	}

	return -1;
}
