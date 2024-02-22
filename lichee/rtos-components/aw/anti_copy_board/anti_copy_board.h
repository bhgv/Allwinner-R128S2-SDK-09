#ifndef _SUNXI_ANTI_COPY_BOARD_H_
#define _SUNXI_ANTI_COPY_BOARD_H_

/* SID CONFIGURATION */
#ifdef CONFIG_ARCH_SUN20IW2
/* get from sid spec */
/* write protect */
#define EFUSE_WRITE_PROTECT	(0x48)
/* read  protect */
#define EFUSE_READ_PROTECT	(0x4C)
#define SSK_WRITE_PROTECT_FLAG	(12)
#define SSK_READ_PROTECT_FLAG	(12)

#define SSK_KEYLEN		(16)	// Bytes
#endif

#define PLAINTEXT_PATH "/data/plaintext"
#define CIPHERTEXT_PATH "/data/ciphertext"
#define FILE_LENGTH (128)

/* CE CONFIGUREATION */
#define AES_MODE_ECB		(0)
#define AES_DIR_DECRYPT		(1)

/* ACB return value */
#define ACB_VERIFY_RET_OK			(0)
#define ACB_VERIFY_RET_NO_SSK_ERR		(-1)
#define ACB_VERIFY_RET_NO_PLAIN_CIPHER_ERR	(-2)
#define ACB_VERIFY_RET_VERIFY_ERR		(-3)
#define ACB_VERIFY_RET_NO_ALL_ERR		(-4)
#define ACB_VERIFY_RET_MALLOC_ERR		(-5)
#define ACB_VERIFY_RET_READ_DATA_ERR		(-6)

#define ACB_BURN_RET_OK				(0)
#define ACB_BURN_RET_ARG_ERR			(-1)
#define ACB_BURN_RET_MALLOC_ERR			(-2)
#define ACB_BURN_RET_SSK_WRITE_ERR		(-3)
#define ACB_BURN_RET_SSK_CMP_ERR		(-4)
#define ACB_BURN_RET_SSK_SET_RB_FBD_ERR		(-5)
#define ACB_BURN_RET_NO_PLAINTEXT_ERR		(-6)
#define ACB_BURN_RET_NO_CIPHERTEXT_ERR		(-7)
#define ACB_BURN_RET_PLAINTEXT_SIZE_ERR		(-8)
#define ACB_BURN_RET_CIPHERTEXT_SIZE_ERR	(-9)
#define ACB_BURN_RET_VERIFY_ERR			(-10)

int acb_verify(char *plain_path, char *cipher_path);

#endif
