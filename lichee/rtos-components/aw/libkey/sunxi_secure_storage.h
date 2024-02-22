#ifndef __SECURE_STORAGE_H__
#define __SECURE_STORAGE_H__

#include <stdint.h>
#include <stddef.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

#define SUNXI_SECSTORE_VERSION 1

#define MAX_STORE_LEN 0xc00 /*3K payload*/
#define STORE_OBJECT_MAGIC 0x17253948
#define STORE_REENCRYPT_MAGIC 0x86734716
#define STORE_WRITE_PROTECT_MAGIC 0x8ad3820f

#define SUNXI_SECURE_STORTAGE_INFO_HEAD_LEN (64 + 4 + 4 + 4)
#define SUNXI_HDCP_KEY_LEN (288)
typedef struct
{
		char     name[64];      //key name
		uint32_t len;           //the len fo key_data
		uint32_t encrypted;
		uint32_t write_protect;
		char    key_data[4096 - SUNXI_SECURE_STORTAGE_INFO_HEAD_LEN];//the raw data of key
}
sunxi_secure_storage_info_t;

#define SECURE_STORAGE_DUMMY_KEY_NAME "reserved_after_delete"

typedef struct {
		unsigned int magic; /* store object magic*/
		int id; /*store id, 0x01,0x02.. for user*/
		char name[64]; /*OEM name*/
		unsigned int re_encrypt; /*flag for OEM object*/
		unsigned int version;
		unsigned int write_protect; /*can be placed or not, =0, can be write_protectd*/
		unsigned int reserved[3];
		unsigned int actual_len; /*the actual len in data buffer*/
		unsigned char data[MAX_STORE_LEN]; /*the payload of secure object*/
		unsigned int crc; /*crc to check the sotre_objce valid*/
} store_object_t;

#define SUNXI_SECURE_MODE_WITH_SECUREOS              1

/* secure storage map, have the key info in the keysecure storage */
#define SEC_BLK_SIZE (4096)
#define MAP_KEY_NAME_SIZE	(64)
#define MAP_KEY_DATA_SIZE	(32)
struct map_info {
		unsigned char data[SEC_BLK_SIZE - sizeof(int) * 2];
		unsigned int magic;
		unsigned int crc;
};

int sunxi_secure_storage_is_init(void);
int sunxi_secure_storage_init(void);
int sunxi_secure_storage_get_name_by_index(const int index, char *buffer, const int buffer_size);
int sunxi_secure_storage_read(const char *item_name, char *buffer, int buffer_len, int *data_len);
int sunxi_secure_storage_write(const char *item_name, const char *buffer, int length);
int sunxi_secure_storage_exit(void);
int sunxi_secure_storage_erase_data_only(const char *item_name);
int sunxi_secure_storage_erase_all(void);

#endif /* __SECURE_STORAGE_H__ */
