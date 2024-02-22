#ifndef __SECURE_STORAGE_WARPPER_H__
#define __SECURE_STORAGE_WARPPER_H__

#include <stddef.h>

int sunxi_secstorage_read(int item, unsigned char *buf, unsigned int len);
int sunxi_secstorage_write(int item, unsigned char *buf, unsigned int len);
int sunxi_secstorage_flush(void);
unsigned int crc32(unsigned int crc, unsigned char const *p, size_t len);

#endif /* __SECURE_STORAGE_H__ */