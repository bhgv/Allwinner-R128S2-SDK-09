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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/time.h>

#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_mem.h>
#include <hal_timer.h>
#include <sunxi_hal_spi.h>

#define KB (1024)
#define MB (1024*KB)
#define US (1)
#define MS (1000*US)
#define S  (1000*MS)

static void pabort(const char *s)
{
	if (errno != 0)
		perror(s);
	else
		hal_log_err("%s\n", s);

	abort();
}

static int port = 1;
static uint32_t mode;
static uint8_t bits = 8;
static uint32_t speed = 5000000;
static int verbose;
static int transfer_size;
static int iterations;

static uint8_t default_tx[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x95,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xF0, 0x0D,
};

static uint8_t default_rx[sizeof(default_tx)];
static char *input_tx;

static void hex_dump(const void *src, size_t length, size_t line_size,
			 char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;

	printf("%s | ", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" |");
			while (line < address) {
				c = *line++;
				printf("%c", (c < 32 || c > 126) ? '.' : c);
			}
			printf("|\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

/*
 *  Unescape - process hexadecimal escape character
 *      converts shell input "\x23" -> 0x23
 */
static int unescape(char *_dst, char *_src, size_t len)
{
	int ret = 0;
	int match;
	char *src = _src;
	char *dst = _dst;
	unsigned int ch;

	while (*src) {
		if (*src == '\\' && *(src+1) == 'x') {
			match = sscanf(src + 2, "%2x", &ch);
			if (!match)
				pabort("malformed input string");

			src += 4;
			*dst++ = (unsigned char)ch;
		} else {
			*dst++ = *src++;
		}
		ret++;
	}
	return ret;
}

static unsigned long transfer(int port, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	hal_spi_master_status_t ret = 0;
	unsigned long usec = 0;
	struct timeval start, end;
	hal_spi_master_transfer_t tr = {
		.tx_buf = (uint8_t *)tx,
		.tx_len = len,
		.rx_buf = (uint8_t *)rx,
		.rx_len = len,
		.tx_single_len = len,
		.dummy_byte = 0,
		.bits_per_word = bits,
	};

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	else if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
		else if (mode & SPI_3WIRE)
			tr.rx_buf = 0;
	}

	gettimeofday(&start, NULL);
	ret = hal_spi_xfer(port, &tr, 1);
	gettimeofday(&end, NULL);
	if (ret < 0)
		pabort("can't send spi message");

	if (verbose)
	{
		hex_dump(tx, len, 32, "TX");
		hex_dump(rx, len, 32, "RX");
	}

	if (memcmp(tx, rx, len))
		hal_log_info("rx/tx buffer is not same, data error!!!\n");

	usec = (1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec);
	return usec;
}

static void print_usage(const char *prog)
{
	hal_log_info("Usage: %s [-DsblHOLC3vpNR24SI]\n", prog);
	puts("  -D --device   device port to use (default 1)\n"
	     "  -s --speed    max speed (Hz)\n"
	     "  -b --bpw      bits per word\n"
	     "  -l --loop     loopback\n"
	     "  -H --cpha     clock phase\n"
	     "  -O --cpol     clock polarity\n"
	     "  -L --lsb      least significant bit first\n"
	     "  -C --cs-high  chip select active high\n"
	     "  -3 --3wire    SI/SO signals shared\n"
	     "  -v --verbose  Verbose (show tx buffer)\n"
	     "  -p            Send data (e.g. \"1234\\xde\\xad\")\n"
	     "  -N --no-cs    no chip select\n"
	     "  -R --ready    slave pulls low to pause\n"
	     "  -2 --dual     dual transfer\n"
	     "  -4 --quad     quad transfer\n"
	     "  -S --size     transfer size\n"
	     "  -I --iter     iterations\n");
}

static int parse_opts(int argc, char *argv[])
{
	int ret = 0;

	while (1) {
		static const struct option lopts[] = {
			{ "device",  1, 0, 'D' },
			{ "speed",   1, 0, 's' },
			{ "bpw",     1, 0, 'b' },
			{ "loop",    0, 0, 'l' },
			{ "cpha",    0, 0, 'H' },
			{ "cpol",    0, 0, 'O' },
			{ "lsb",     0, 0, 'L' },
			{ "cs-high", 0, 0, 'C' },
			{ "3wire",   0, 0, '3' },
			{ "no-cs",   0, 0, 'N' },
			{ "ready",   0, 0, 'R' },
			{ "dual",    0, 0, '2' },
			{ "verbose", 0, 0, 'v' },
			{ "quad",    0, 0, '4' },
			{ "size",    1, 0, 'S' },
			{ "iter",    1, 0, 'I' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "D:s:b:lHOLC3NR24p:vS:I:",
				lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			port = atoi(optarg);
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'l':
			mode |= SPI_LOOP;
			break;
		case 'H':
			mode |= SPI_CPHA;
			break;
		case 'O':
			mode |= SPI_CPOL;
			break;
		case 'L':
			mode |= SPI_LSB_FIRST;
			break;
		case 'C':
			mode |= SPI_CS_HIGH;
			break;
		case '3':
			mode |= SPI_3WIRE;
			break;
		case 'N':
			mode |= SPI_NO_CS;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'R':
			mode |= SPI_READY;
			break;
		case 'p':
			input_tx = optarg;
			break;
		case '2':
			mode |= SPI_TX_DUAL;
			break;
		case '4':
			mode |= SPI_TX_QUAD;
			break;
		case 'S':
			transfer_size = atoi(optarg);
			break;
		case 'I':
			iterations = atoi(optarg);
			break;
		default:
			print_usage(argv[0]);
			ret = -1;
		}
	}
	if (mode & SPI_LOOP) {
		if (mode & SPI_TX_DUAL)
			mode |= SPI_RX_DUAL;
		if (mode & SPI_TX_QUAD)
			mode |= SPI_RX_QUAD;
	}

	return ret;
}

static void transfer_escaped_string(int port, char *str)
{
	size_t size = strlen(str);
	uint8_t *tx;
	uint8_t *rx;

	tx = hal_malloc(size);
	if (!tx)
		pabort("can't allocate tx buffer");

	rx = hal_malloc(size);
	if (!rx)
		pabort("can't allocate rx buffer");

	size = unescape((char *)tx, str, size);
	transfer(port, tx, rx, size);
	hal_free(rx);
	hal_free(tx);
}

static void show_transfer_info(unsigned long size, unsigned long time)
{
	double rate;

	printf("total size   : ");
	if (size >= MB) {
		printf("%.2lf MB", (double)size/(double)MB);
	} else if (size >= KB) {
		printf("%.2lf KB", (double)size/(double)KB);
	} else {
		printf("%lu B", size);
	}
	printf("\n");

	printf("total time   : ");
	if (time >= S) {
		printf("%.2lf s", (double)time/(double)S);
	} else if (time >= MS) {
		printf("%.2lf ms", (double)time/(double)MS);
	} else {
		printf("%.2lf us", (double)time/(double)US);
	}
	printf("\n");

	rate = ((double)size / (double)MB) / ((double)time / (double)S);
	printf("averange rate: %.2lf MB/s\n", rate);
}

static unsigned long transfer_buf(int port, int len)
{
	uint8_t *tx;
	uint8_t *rx;
	int i;
	unsigned long usec = 0;

	tx = hal_malloc(len);
	if (!tx)
		pabort("can't allocate tx buffer");

	srand(time(NULL));
	for (i = 0; i < len; i++)
		tx[i] = random();

	rx = hal_malloc(len);
	if (!rx)
		pabort("can't allocate rx buffer");

	usec = transfer(port, tx, rx, len);

	if (mode & SPI_LOOP) {
		if (memcmp(tx, rx, len)) {
			fprintf(stderr, "transfer error !\n");
			hex_dump(tx, len, 32, "TX");
			hex_dump(rx, len, 32, "RX");
			exit(1);
		}
	}

	hal_free(rx);
	hal_free(tx);

	return usec;
}

static int cmd_spidev_test(int argc, char **argv)
{
	hal_spi_master_config_t cfg = { 0 };

	port = 1;
	mode = 0;
	bits = 8;
	speed = 5000000;
	verbose = 0;
	transfer_size = 0;
	iterations = 0;
	input_tx = NULL;
	memset(default_rx, 0, sizeof(default_rx));

	if (parse_opts(argc, argv) < 0) {
		return 0;
	}

	if (mode & SPI_3WIRE)
		cfg.bus_mode = HAL_SPI_BUS_BIT;
	else
		cfg.bus_mode = HAL_SPI_BUS_MASTER;
	cfg.cs_mode = HAL_SPI_CS_AUTO;
	cfg.clock_frequency = speed;
	cfg.chipselect = 0;
	cfg.mode = mode;
	cfg.sip = 0;
	cfg.flash = 0;
	hal_spi_init(port, &cfg);

	hal_log_info("spi mode: 0x%x\n", mode);
	hal_log_info("bits per word: %u\n", bits);
	hal_log_info("max speed: %u Hz (%u kHz)\n", speed, speed/1000);

	if (input_tx)
		transfer_escaped_string(port, input_tx);
	else if (transfer_size) {
		unsigned long total_size = transfer_size * iterations;
		unsigned long total_usec = 0;
		int i;

		for (i = 0; i < iterations; i++)
			total_usec += transfer_buf(port, transfer_size);

		show_transfer_info(total_size, total_usec);
		printf("averange time: %.2lf us\n", (double)total_usec/(double)(iterations));
	} else
		transfer(port, default_tx, default_rx, sizeof(default_tx));

	hal_spi_deinit(port);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_spidev_test, hal_spidev_test, spidev hal APIs tests)
