#include "sunxi_secure_storage_warpper.h"

extern int nand_secure_storage_read(int item, unsigned char *buf, unsigned int len);
extern int nand_secure_storage_write(int item, unsigned char *buf, unsigned int len);

int sunxi_secstorage_read(int item, unsigned char *buf, unsigned int len)
{
	return nand_secure_storage_read(item, buf, len);
}

int sunxi_secstorage_write(int item, unsigned char *buf, unsigned int len)
{
	return nand_secure_storage_write(item, buf, len);
}

int sunxi_secstorage_flush(void)
{
	return 0;
}

#if 1
#define CRCPOLY_LE 0xedb88320
unsigned int crc32(unsigned int crc, unsigned char const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
	}
	return crc;
}
#endif
