#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <hal_cmd.h>
#include <hal_mem.h>
#include <sunxi_hal_efuse.h>
#include <sunxi_hal_ce.h>

#include "anti_copy_board.h"

//#define ACB_DEBUG
#ifdef ACB_DEBUG
static void acb_dump(char *str,unsigned char *data, int len, int align)
{
        int i = 0;
        if(str)
                printf("\n%s: ",str);
        for(i = 0; i<len; i++)
        {
                if((i%align) == 0)
                {
                        printf("\n");
                        printf("%p: ", data + i);
                }
                printf("%02x ",*(data++));
        }
        printf("\n");
}
#define ACB_DBG(fmt, arg...) hal_log_err("%s()%d - "fmt, __func__, __LINE__, ##arg)
#else
static void acb_dump(char *str,unsigned char *data, int len, int align)
{

}
#define ACB_DBG(fmt, arg...) do{} while(0)
#endif

static int hexstr_to_byte(const char* source, uint8_t* dest, int sourceLen)
{
	uint32_t i;
	uint8_t highByte, lowByte;

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

static int acb_check_file_length(char *file_path, long file_length)
{
	int ret = 0;
	long length = 0;
	FILE *file = fopen(file_path, "rb");

	if (file == NULL) {
		printf("File %s does not exist.\n", file_path);
		ret = -1;
		return ret;
	}

	if (file_length == 0) {
		ACB_DBG("skip file %s length check\n", file_path);
		fclose(file);
		return ret;
	}

	fseek(file, 0, SEEK_END);
	length = ftell(file);

	if (length == file_length) {
		printf("File %s exists and length is %d bytes.\n", file_path, file_length);
		ret = 0;
	} else {
		printf("File %s exists but length is %d bytes, not equal to %d bytes.\n", file_path, length, file_length);
		ret = -2;
	}

	fclose(file);

	return ret;
}

static int acb_get_file_data(char *file_path, char *buf, long buf_length, long *file_length)
{
	int ret = 0;
	int read_length = 0;
	FILE *file = fopen(file_path, "rb");

	if (file == NULL) {
		printf("File %s does not exist.\n", file_path);
		ret = -1;
		return ret;
	}

	fseek(file, 0, SEEK_END);
	*file_length = ftell(file);

	if (buf_length != *file_length) {
		printf("File %s exists but length is %d bytes, not equal to %d bytes.\n", file_path, *file_length, buf_length);
		ret = -2;
		goto out;
	}

	fseek(file, 0, SEEK_SET);
	read_length = fread(buf, 1, buf_length, file);

	if (read_length != buf_length) {
		printf("read length %d, not equal to file length: %d\n", read_length, buf_length);
		ret = -3;  // read file failed
	}

out:
	fclose(file);

	return ret;
}

int acb_verify(char *plain_path, char *cipher_path)
{
	int ret = ACB_VERIFY_RET_OK;
	int ssk_flag = 0;
	int plain_flag = 0;
	int cipher_flag = 0;
	char *plain_buf = NULL;
	char *cipher_buf = NULL;
	char *decrypt_buf = NULL;
	long plain_length = 0;
	long cipher_length = 0;
	crypto_aes_req_ctx_t *aes_ctx = NULL;

	if (acb_check_file_length(plain_path, 0) == 0)
		plain_flag = 1;
	if (acb_check_file_length(cipher_path, 0) == 0)
		cipher_flag = 1;
	if (efuse_reg_read_key(EFUSE_WRITE_PROTECT) & (1 << SSK_WRITE_PROTECT_FLAG))
		ssk_flag = 1;

	if ((plain_flag == 0) && (plain_flag == 0) && (ssk_flag == 0))
		return ACB_VERIFY_RET_NO_ALL_ERR;

	if ((plain_flag == 0) && (plain_flag == 0))
		return ACB_VERIFY_RET_NO_PLAIN_CIPHER_ERR;

	if (ssk_flag == 0)
		return ACB_VERIFY_RET_NO_SSK_ERR;

	plain_buf = hal_malloc_align(FILE_LENGTH, max(CE_ALIGN_SIZE, CACHELINE_LEN));
	if (plain_buf == NULL) {
		printf("plain_buf malloc error\n");
		ret = ACB_VERIFY_RET_MALLOC_ERR;
		goto out;
	}

	cipher_buf = hal_malloc_align(FILE_LENGTH, max(CE_ALIGN_SIZE, CACHELINE_LEN));
	if (cipher_buf == NULL) {
		printf("cipher_buf malloc error\n");
		ret = ACB_VERIFY_RET_MALLOC_ERR;
		goto out;
	}

	decrypt_buf = hal_malloc_align(FILE_LENGTH, max(CE_ALIGN_SIZE, CACHELINE_LEN));
	if (decrypt_buf == NULL) {
		printf("decrypt_buf malloc error\n");
		ret = ACB_VERIFY_RET_MALLOC_ERR;
		goto out;
	}

	ret = acb_get_file_data(plain_path, plain_buf, FILE_LENGTH, &plain_length);
	if (ret) {
		printf("read file %s data error: %d\n", plain_path, ret);
		ret = ACB_VERIFY_RET_READ_DATA_ERR;
		goto out;
	}

	ret = acb_get_file_data(cipher_path, cipher_buf, FILE_LENGTH, &cipher_length);
	if (ret) {
		printf("read file %s data error: %d\n", plain_path, ret);
		ret = ACB_VERIFY_RET_READ_DATA_ERR;
		goto out;
	}

	aes_ctx = (crypto_aes_req_ctx_t *)hal_malloc_align(sizeof(crypto_aes_req_ctx_t), max(CE_ALIGN_SIZE, CACHELINE_LEN));
	if (aes_ctx == NULL) {
		printf("aes_ctx malloc buffer fail\n");
		ret = ACB_VERIFY_RET_MALLOC_ERR;
		goto out;
	}

	aes_ctx->key = hal_malloc_align(SSK_KEYLEN, max(CE_ALIGN_SIZE, CACHELINE_LEN));
	if (aes_ctx->key == NULL) {
		printf("aes_ctx key malloc buffer fail\n");
		ret = ACB_VERIFY_RET_MALLOC_ERR;
		goto out;
	}

	acb_dump("cipher", cipher_buf, plain_length, 16);

	sunxi_ce_init();

	aes_ctx->src_buffer = cipher_buf;
	aes_ctx->src_length = cipher_length;
	aes_ctx->dst_buffer = decrypt_buf;
	aes_ctx->dst_length = cipher_length;
	aes_ctx->iv = NULL;
	aes_ctx->iv_next = NULL;
	memcpy(aes_ctx->key, "KEY_SEL_SSK", sizeof("KEY_SEL_SSK"));
	aes_ctx->key_length = SSK_KEYLEN;
	aes_ctx->mode = AES_MODE_ECB;
	aes_ctx->dir = AES_DIR_DECRYPT;

	ret = do_aes_crypto(aes_ctx);
	if (ret < 0) {
		printf("aes decrypt fail %d\n", ret);
		ret = ACB_VERIFY_RET_VERIFY_ERR;
		goto out;
	}

	if (memcmp(plain_buf, decrypt_buf, plain_length)) {
		printf("error: plain data and decrypt data compare failed\n");
		acb_dump("plain", plain_buf, plain_length, 16);
		acb_dump("decrypt", decrypt_buf, plain_length, 16);
		ret = ACB_VERIFY_RET_VERIFY_ERR;
		goto out;
	}

out:
	if (plain_buf)
		hal_free_align(plain_buf);
	if (cipher_buf)
		hal_free_align(cipher_buf);
	if (decrypt_buf)
		hal_free_align(decrypt_buf);
	if (aes_ctx) {
		if (aes_ctx->key)
			hal_free_align(aes_ctx->key);

		hal_free_align(aes_ctx);
	}

	sunxi_ce_uninit();

	printf("acb_verify return: %d\n", ret);
	return ret;
}

int cmd_acb_verify_test(int argc, char **argv)
{
	int ret = 0;

	ret = acb_verify(PLAINTEXT_PATH, CIPHERTEXT_PATH);
	if (ret) {
		printf("acb verify test failed: %d\n", ret);
	}

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_acb_verify_test, acb_verify, anti_copy_board verify test);

/* fun: cmd_burn_ssk
 * arg: <ssk_key_hex_string>
 * ret: refs to AVB_BURN_RET_xxx
 * */
int cmd_acb_burn_ssk(int argc, char **argv)
{
	int ret = ACB_BURN_RET_OK;
	int i = 0;
	char *key_hex_string = NULL;
	uint32_t buf_len = 0;
	char *write_key_data = NULL;
	char *read_key_data = NULL;
	efuse_key_map_new_t *ssk_keymap = NULL;

	if (argc != 2) {
		printf("Argument Error!\n");
		printf("Usage: burn_ssk <key_hex_string>\n");
		return ACB_BURN_RET_ARG_ERR;
	}

	printf("== burn ssk begin ==\n");

	/* 0. check plaintext/ciphertext exist */
	ret = acb_check_file_length(PLAINTEXT_PATH, FILE_LENGTH);
	if (ret == -1) {
		return ACB_BURN_RET_NO_PLAINTEXT_ERR;
	} else if (ret == -2) {
		return ACB_BURN_RET_PLAINTEXT_SIZE_ERR;
	}

	ret = acb_check_file_length(CIPHERTEXT_PATH, FILE_LENGTH);
	if (ret == -1) {
		return ACB_BURN_RET_NO_CIPHERTEXT_ERR;
	} else if (ret == -2) {
		return ACB_BURN_RET_CIPHERTEXT_SIZE_ERR;
	}

	/* check ssk whether had burn */
	if (efuse_reg_read_key(EFUSE_WRITE_PROTECT) & (1 << SSK_WRITE_PROTECT_FLAG)) {
		printf("ssk has been burned\n");
		printf("== burn ssk success ==\n");
		return ACB_BURN_RET_OK;
	}

	/* preprate to burn ssk */
	key_hex_string = argv[1];
	buf_len = strlen(argv[1]);
	printf("ssk_hex_string: %s, len: %d\n", key_hex_string, buf_len);
	if (buf_len % 2 != 0) {
		printf("ssk_hex_string len: %d is error!\n", buf_len);
		return ACB_BURN_RET_ARG_ERR;
	}

	ssk_keymap = efuse_search_key_by_name("ssk");
	if (ssk_keymap->size == 0) {
		printf("error: unknow key name\n");
		return ACB_BURN_RET_ARG_ERR;
	}

	/* ssk length should be right */
	if (((ssk_keymap->size / 8) != SSK_KEYLEN) || ((ssk_keymap->size / 8) != (buf_len / 2))) {
		printf("error: ssk key len is error, keymap->size / 8: %d, buf_len / 2: %d\n", ssk_keymap->size / 8, buf_len / 2);
		return ACB_BURN_RET_ARG_ERR;
	}

	write_key_data = malloc(SSK_KEYLEN);
	if (!write_key_data) {
		printf("malloc write buffer %d bytes error!", SSK_KEYLEN);
		return ACB_BURN_RET_MALLOC_ERR;
	}

	hexstr_to_byte(key_hex_string, write_key_data, buf_len);

	// 1. write ssk
	ret = hal_efuse_write("ssk", write_key_data, SSK_KEYLEN << 3);
	if (ret) {
		printf("efuse write ssk error: %d\n", ret);
		ret = ACB_BURN_RET_SSK_WRITE_ERR;
		goto out;
	} else {
		printf("efuse write ssk end\n");
	}

	// 2. read ssk
	read_key_data = malloc(SSK_KEYLEN);
	if (!read_key_data) {
		printf("malloc read buffer %d bytes error!", SSK_KEYLEN);
		ret = ACB_BURN_RET_MALLOC_ERR;
		goto out;
	}

	for(i = 0; i < SSK_KEYLEN / 4; i++) {
		*((uint32_t *)read_key_data + i) = efuse_reg_read_key(ssk_keymap->offset + i * 4);
	}

	printf("efuse reg read ssk success\n");

	// 3. compare write/read ssk
	if (memcmp(write_key_data, read_key_data, SSK_KEYLEN)) {
		printf("error: ssk read data is not equal to ssk write data\n");
		acb_dump("ssk write:", write_key_data, SSK_KEYLEN, 16);
		acb_dump("ssk read:", read_key_data, SSK_KEYLEN, 16);
		ret = ACB_BURN_RET_SSK_CMP_ERR;
		goto out;
	} else {
		printf("compare ssk write/data ssk success\n");
		acb_dump("ssk:", write_key_data, SSK_KEYLEN, 16);
	}

	// 4. verify ssk and ciphertext/plaintext
	ret = acb_verify(PLAINTEXT_PATH, CIPHERTEXT_PATH);
	if (ret) {
		printf("error: verify ssk and ciphertext/plaintext failed\n");
		ret = ACB_BURN_RET_VERIFY_ERR;
		goto out;
	}

	// 5. write ssk rd fbd
	hal_efuse_set_rd_fbd_bit(SSK_READ_PROTECT_FLAG);

	if (efuse_reg_read_key(EFUSE_READ_PROTECT) & (1 << SSK_READ_PROTECT_FLAG)) {
		printf("set ssk rd fbd success\n");
	} else {
		printf("error: set ssk rd fbd failed\n");
		ret = ACB_BURN_RET_SSK_SET_RB_FBD_ERR;
		goto out;
	}

	printf("== burn ssk success ==\n");
out:
	if (write_key_data)
		free(write_key_data);
	if (read_key_data)
		free(read_key_data);

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_acb_burn_ssk, acb_burn_ssk, anti_copy_board burn ssk);
