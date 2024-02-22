#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <console.h>

#define PRINT_FORMAT_NAME(x) printf("%-10s : %s\n", #x, x)
#define PRINT_TYPE_LEN(x) printf("%-20s : %zu byte(s)\n", #x, sizeof(x))

int sunxi_format_print(int argc, char **argv)
{
	/* print inttypes */
	PRINT_FORMAT_NAME(PRId8);
	PRINT_FORMAT_NAME(PRIi8);
	PRINT_FORMAT_NAME(PRIo8);
	PRINT_FORMAT_NAME(PRIu8);
	PRINT_FORMAT_NAME(PRIx8);
	PRINT_FORMAT_NAME(PRIX8);
	PRINT_FORMAT_NAME(PRId16);
	PRINT_FORMAT_NAME(PRIi16);
	PRINT_FORMAT_NAME(PRIo16);
	PRINT_FORMAT_NAME(PRIu16);
	PRINT_FORMAT_NAME(PRIx16);
	PRINT_FORMAT_NAME(PRIX16);
	PRINT_FORMAT_NAME(PRId32);
	PRINT_FORMAT_NAME(PRIi32);
	PRINT_FORMAT_NAME(PRIo32);
	PRINT_FORMAT_NAME(PRIu32);
	PRINT_FORMAT_NAME(PRIx32);
	PRINT_FORMAT_NAME(PRIX32);
	PRINT_FORMAT_NAME(PRId64);
	PRINT_FORMAT_NAME(PRIi64);
	PRINT_FORMAT_NAME(PRIo64);
	PRINT_FORMAT_NAME(PRIu64);
	PRINT_FORMAT_NAME(PRIx64);
	PRINT_FORMAT_NAME(PRIX64);
	PRINT_FORMAT_NAME(PRIdPTR);
	PRINT_FORMAT_NAME(PRIiPTR);
	PRINT_FORMAT_NAME(PRIoPTR);
	PRINT_FORMAT_NAME(PRIuPTR);
	PRINT_FORMAT_NAME(PRIxPTR);
	PRINT_FORMAT_NAME(PRIXPTR);
	PRINT_FORMAT_NAME(PRIdMAX);

	/* print base types len */
	PRINT_TYPE_LEN(bool);
	PRINT_TYPE_LEN(char);
	PRINT_TYPE_LEN(unsigned char);
	PRINT_TYPE_LEN(short);
	PRINT_TYPE_LEN(unsigned short);
	PRINT_TYPE_LEN(int);
	PRINT_TYPE_LEN(unsigned int);
	PRINT_TYPE_LEN(long);
	PRINT_TYPE_LEN(unsigned long);
	PRINT_TYPE_LEN(long int);
	PRINT_TYPE_LEN(unsigned long int);
	PRINT_TYPE_LEN(long long);
	PRINT_TYPE_LEN(unsigned long long);
	PRINT_TYPE_LEN(void *);

	/* print other types len */
	PRINT_TYPE_LEN(int8_t);
	PRINT_TYPE_LEN(uint8_t);
	PRINT_TYPE_LEN(int16_t);
	PRINT_TYPE_LEN(uint16_t);
	PRINT_TYPE_LEN(int32_t);
	PRINT_TYPE_LEN(uint32_t);
	PRINT_TYPE_LEN(int64_t);
	PRINT_TYPE_LEN(uint64_t);
	PRINT_TYPE_LEN(size_t);
	PRINT_TYPE_LEN(ssize_t);
	PRINT_TYPE_LEN(BaseType_t);
	PRINT_TYPE_LEN(intptr_t);
	PRINT_TYPE_LEN(uintptr_t);

#if 0
	BaseType_t x = 2;
	//printf("x: %"PRIdPTR"\n", (intptr_t)x);  // error in m33
	printf("x: %ld\n", (long)x);
#endif

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(sunxi_format_print, format, print format);

