#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "sunxi_secure_storage.h"
#include "hal_mem.h"
#include <hal_cmd.h>

static int hexstr_to_byte(const char* source, char* dest, int sourceLen)
{
    short i;
    char highByte, lowByte;

    for (i = 0; i < sourceLen; i += 2) {
            highByte = toupper(source[i]);
            lowByte  = toupper(source[i + 1]);
            if (highByte < '0' || (highByte > '9' && highByte < 'A' ) || highByte > 'F') {
                    printf("input buf[%d] is %c, not in 0123456789ABCDEF\n", i, source[i]);
                    return -1;
            }
            if (lowByte < '0' || (lowByte > '9' && lowByte < 'A' ) || lowByte > 'F') {
                    printf("input buf[%d] is %c, not in 0123456789ABCDEF\n", i+1, source[i+1]);
                    return -1;
            }

            if (highByte > 0x39)
                    highByte -= 0x37;
            else
                    highByte -= 0x30;

            if (lowByte > 0x39)
                    lowByte -= 0x37;
            else
                    lowByte -= 0x30;
            dest[i / 2] = (highByte << 4) | lowByte;
    }
    return 0;
}

static void sunxi_dump(const void *addr, unsigned int len)
{
	unsigned int i;
	const unsigned char *p = addr;
	len++;

	for (i = 1; i < len; ++i) {
		printf("%02x ", *p++);
		if (i % 16 == 0) {
			printf("\r\n");
		}
	}
	printf("\r\n");
}

static void printChars(const char* buffer, int length) {
	for (int i = 0; i < length; i++) {
		printf("%c", buffer[i]);
	}
	printf("\n");
}

/*
static int sunxi_secure_storage_list(void)
{
	int ret;
	int index = 1;
	int data_len = 0;
	char name[64];
	char *buffer = hal_malloc(4096);

	if(!buffer) {
		printf("<%s:%d> hal_malloc failed!\n", __func__, __LINE__);
		return -1;
	}

	printf("[secure storage]\n");

	while(!sunxi_secure_storage_get_name_by_index(index, name, 64)){
		printf("%d: %s = ", index, name);
		memset(buffer, 0xff, 4096);
		ret = sunxi_secure_storage_read(name, buffer, 4096, &data_len);
        if(!ret) {
		//	printChars(buffer, data_len);
			sunxi_dump(buffer, data_len);
		} else {
			printf("read %s failed!\n", name);
			goto exit;
		}
		index++;
	}

exit:
	printf("end index: %d\n", index - 1);
	hal_free(buffer);
	return ret;
}
*/

static void usage(void)
{
	printf("usage: mikey write|read|erase <key_name> <key_file>\n");
	printf(" \tseckey write <key_name> <key_string>\t write key named [key_name] with key string\n");
	printf(" \tseckey read <key_name>\t read key named [key_name]\n");
	printf(" \tseckey erase <key_name/all>\t erase key named <key_name/all> in secure storage\n");
}

int cmd_secure_storage_test(int argc, char *const argv[])
{
	int ret = -1;
	int data_len = 0;

	if (argc > 4 || argc < 2) {
		printf("wrong argc\n");
		usage();
		return -1;
	}

	if (sunxi_secure_storage_init() < 0) {
		printf("%s secure storage init err\n", __func__);
		return -1;
	}

	if (argc == 3 && !strncmp("erase", argv[1], strlen("erase"))) {
		if (!strncmp("all", argv[2], strlen("all"))) {
			ret = sunxi_secure_storage_erase_all();
			if (!ret) {
				printf("secure storage erase all ok!\n");
			}
		}
		else {
			ret = sunxi_secure_storage_erase_data_only(argv[2]);
			if (ret < 0) {
				printf("%s secure storage erase err\n", __func__);
				return -1;
			}
		}
	} else if (argc == 3 && !strncmp("read", argv[1], strlen("read"))) {
		char *buffer = hal_malloc(4096);
		store_object_t *obj_r;

		if(!buffer) {
			printf("<%s:%d> hal_malloc failed!\n", __func__, __LINE__);
			ret = -1;
			goto exit;
		}
		memset(buffer, 0xff, 4096);
		ret = sunxi_secure_storage_read(argv[2], buffer, 4096, &data_len);
		obj_r = (store_object_t *)buffer;
        if(!ret) {
			sunxi_dump(obj_r->data, obj_r->actual_len);
			//printChars(buffer, data_len);
		} else {
			printf("read %s failed!\n", argv[2]);
		}
		hal_free(buffer);
    } else if (argc == 4 && !strncmp("write", argv[1], strlen("write"))) {
		char buffer_bak[4096];
		char data_buf[4096];

		ret = hexstr_to_byte(argv[3], data_buf, strlen(argv[3]));
		if (ret == -1) {
			printf("ERROR: input buf is hexString\n");
            return -1;
        }

		store_object_t *obj_w = hal_malloc(sizeof(store_object_t));
		if(!obj_w) {
			printf("<%s:%d> hal_malloc failed!\n", __func__, __LINE__);
			ret = -1;
			goto exit;
		}
		memset(obj_w, 0, sizeof(store_object_t));

		obj_w->magic = STORE_OBJECT_MAGIC;
		strncpy(obj_w->name, argv[2], 64);
		obj_w->re_encrypt    = 0;
		obj_w->version       = SUNXI_SECSTORE_VERSION;
		obj_w->id            = 0;
		obj_w->write_protect = 0;
		memset(obj_w->reserved, 0, 4);
		obj_w->actual_len = strlen(argv[3])/2;
		printf("string_len:%d\n", obj_w->actual_len);
		memcpy(obj_w->data, data_buf, strlen(argv[3]));

		memcpy(buffer_bak, obj_w, sizeof(store_object_t));

		ret = sunxi_secure_storage_write(argv[2], buffer_bak, sizeof(store_object_t));

		if(!ret)
			printf("set %s key success!\n", argv[2]);
		else
			printf("set %s key failed!\n", argv[2]);

		hal_free(obj_w);
	} else {
		usage();
	}

	if (sunxi_secure_storage_exit() < 0) {
		printf("secure storage exit fail\n");
		goto exit;
	}

exit:
	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_secure_storage_test, seckey, set_secure_storage test);
