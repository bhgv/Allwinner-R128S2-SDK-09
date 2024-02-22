/*
 * ===========================================================================================
 *
 *       Filename:  sunxi_hal_spi.h
 *
 *    Description:  SPI HAL definition.
 *
 *        Version:  Melis3.0
 *         Create:  2019-11-18 11:11:56
 *       Revision:  none
 *       Compiler:  GCC:version 9.2.1
 *
 *         Author:  bantao@allwinnertech.com
 *   Organization:  SWC-BPD
 *  Last Modified:  2019-12-03 16:02:11
 *
 * ===========================================================================================
 */

#ifndef SUNXI_HAL_SPI_H
#define SUNXI_HAL_SPI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "sunxi_hal_common.h"
#include <hal_dma.h>
#include <hal_sem.h>
#include <hal_mutex.h>
#include <hal_clk.h>
#include <spi/common_spi.h>
#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#include <pm_wakelock.h>
#endif

/*****************************************************************************
 * spi controller
 *****************************************************************************/
#define	SPI_CPHA		BIT(0)	/* clock phase */
#define	SPI_CPOL		BIT(1)	/* clock polarity */

#define	SPI_MODE_0		(0|0)
#define	SPI_MODE_1		(0|SPI_CPHA)
#define	SPI_MODE_2		(SPI_CPOL|0)
#define	SPI_MODE_3		(SPI_CPOL|SPI_CPHA)

#define	SPI_CS_HIGH		BIT(2)	/* chipselect active high? */
#define	SPI_LSB_FIRST	BIT(3)	/* per-word bits-on-wire */
#define	SPI_3WIRE		BIT(4)	/* SI/SO signals shared */
#define	SPI_LOOP		BIT(5)	/* loopback mode */
#define	SPI_NO_CS		BIT(6)	/* 1 dev/bus, no chipselect */
#define	SPI_READY		BIT(7)	/* slave pulls low to pause */
#define	SPI_TX_DUAL		BIT(8)	/* transmit with 2 wires */
#define	SPI_TX_QUAD		BIT(9)	/* transmit with 4 wires */
#define	SPI_RX_DUAL		BIT(10)	/* receive with 2 wires */
#define	SPI_RX_QUAD		BIT(11)	/* receive with 4 wires */

typedef enum spi_mode_type
{
	SINGLE_HALF_DUPLEX_RX,		/* single mode, half duplex read */
	SINGLE_HALF_DUPLEX_TX,		/* single mode, half duplex write */
	SINGLE_FULL_DUPLEX_RX_TX,	/* single mode, full duplex read and write */
	DUAL_HALF_DUPLEX_RX,		/* dual mode, half duplex read */
	DUAL_HALF_DUPLEX_TX,		/* dual mode, half duplex write */
	QUAD_HALF_DUPLEX_RX,		/* quad mode, half duplex read */
	QUAD_HALF_DUPLEX_TX,		/* quad mode, half duplex write */
	FULL_DUPLEX_TX_RX,			/* full duplex read and write */
	MODE_TYPE_NULL,
} spi_mode_type_t;

typedef enum
{
	HAL_SPI_BUS_MASTER = 0,
	HAL_SPI_BUS_SLAVE = 1,
	HAL_SPI_BUS_BIT = 2,
} hal_spi_master_bus_mode_t;

typedef enum
{
	HAL_SPI_CS_AUTO = 0,
	HAL_SPI_CS_SOFT = 1,
} hal_spi_master_cs_mode_t;

typedef enum
{
	SUNXI_SPI_SAMP_MODE_OLD = 0,
	SUNXI_SPI_SAMP_MODE_NEW = 1,
} hal_spi_master_bus_sample_mode_t;

typedef enum
{
	SUNXI_SPI_SAMP_DELAY_CYCLE_0_0 = 0,
	SUNXI_SPI_SAMP_DELAY_CYCLE_0_5 = 1,
	SUNXI_SPI_SAMP_DELAY_CYCLE_1_0 = 2,
	SUNXI_SPI_SAMP_DELAY_CYCLE_1_5 = 3,
	SUNXI_SPI_SAMP_DELAY_CYCLE_2_0 = 4,
	SUNXI_SPI_SAMP_DELAY_CYCLE_2_5 = 5,
	SUNXI_SPI_SAMP_DELAY_CYCLE_3_0 = 6,
} hal_spi_master_spi_sample_mode_t;

/** @brief SPI master running status */
typedef enum
{
	HAL_SPI_MASTER_IDLE = 0,	/* SPI master is idle */
	HAL_SPI_MASTER_USING = 1,	/* SPI master is busy */
} hal_spi_master_running_status_t;

typedef enum
{
	HAL_SPI_MASTER_ERROR = -6,			/* SPI master function error occurred */
	HAL_SPI_MASTER_ERROR_NOMEM = -5,	/* SPI master request mem failed */
	HAL_SPI_MASTER_ERROR_TIMEOUT = -4,	/* SPI master xfer timeout */
	HAL_SPI_MASTER_ERROR_BUSY = -3,		/* SPI master is busy */
	HAL_SPI_MASTER_ERROR_PORT = -2,		/* SPI master invalid port */
	HAL_SPI_MASTER_INVALID_PARAMETER = -1,	/* SPI master invalid input parameter */
	HAL_SPI_MASTER_OK = 0,				/* SPI master operation completed successfully */
} hal_spi_master_status_t;

typedef struct spi_dma
{
	struct dma_slave_config config;
	struct sunxi_dma_chan *chan;
} spi_dma_t;

typedef struct
{
	hal_spi_master_bus_mode_t bus_mode;
	hal_spi_master_cs_mode_t cs_mode;
	hal_spi_master_bus_sample_mode_t bus_sample_mode;
	hal_spi_master_spi_sample_mode_t spi_sample_mode;
	uint32_t spi_sample_delay;
	uint8_t chipselect; /* SPI slave device selection */
	uint32_t clock_frequency; /* SPI master clock frequency setting */
	uint32_t mode;
	bool sip;
	bool flash;
} hal_spi_master_config_t;

typedef struct
{
	const uint8_t *tx_buf;	/* Data buffer to send */
	uint32_t tx_len;		/* The total number of bytes to send */
	uint32_t tx_single_len;	/* The number of bytes to send in single mode */
	uint8_t *rx_buf;		/* Received data buffer, */
	uint32_t rx_len;		/* The valid number of bytes received */
	uint8_t tx_nbits : 3;	/* Data buffer to send in nbits mode */
	uint8_t rx_nbits : 3;	/* Data buffer to received in nbits mode */
	uint8_t dummy_byte;		/* Flash send dummy byte, default 0*/
#define	SPI_NBITS_SINGLE	0x01 /* 1bit transfer */
#define	SPI_NBITS_DUAL		0x02 /* 2bits transfer */
#define	SPI_NBITS_QUAD		0x04 /* 4bits transfer */
	uint8_t bits_per_word;	/* transfer bit_per_word */
} hal_spi_master_transfer_t;

struct hal_spi_master {
	int port;
	hal_spi_master_config_t cfg;
};

typedef struct sunxi_spi
{
	unsigned long base;
#ifdef CONFIG_ARCH_SUN55IW3
	unsigned long dma_sel_base;
#endif
	hal_clk_t pclk; /* PLL clock */
	hal_clk_t bus_clk; /* BUS clock */
	hal_clk_t mclk; /* spi module clock */
	struct reset_control *reset;
	int bus_num;
	uint8_t rx_triglevel;
	uint8_t tx_triglevel;
	uint16_t irqnum;
	bool use_dma;

	spi_dma_t dma_rx;
	spi_dma_t dma_tx;
	char *align_dma_buf;
#define ALIGN_DMA_BUF_SIZE (4096 + 64)

	bool used;
	spi_mode_type_t mode_type;
	hal_sem_t done;
	hal_mutex_t mutex;
	bool slave_aborted;
	int8_t result : 2;
#define SPI_XFER_READY 0
#define SPI_XFER_OK 1
#define SPI_XFER_FAILED -1

	struct sunxi_spi_params_t *para;
	hal_spi_master_config_t config;
	hal_spi_master_transfer_t *transfer;

#ifdef CONFIG_COMPONENTS_PM
	struct pm_device pm;
	struct wakelock wl;
#endif
} sunxi_spi_t;

hal_spi_master_status_t hal_spi_slave_abort(int port);
hal_spi_master_status_t hal_spi_init(int port, hal_spi_master_config_t *cfg);
hal_spi_master_status_t hal_spi_deinit(int port);
hal_spi_master_status_t hal_spi_write(int port, const void *buf, uint32_t size);
hal_spi_master_status_t hal_spi_read(int port, void *buf, uint32_t size);
hal_spi_master_status_t hal_spi_xfer(int port, hal_spi_master_transfer_t *t, int num);
#ifdef CONFIG_DRIVERS_SPI_PANIC_TRANSFER
hal_spi_master_status_t hal_spi_panic_xfer(int port, hal_spi_master_transfer_t *t, int num);
#endif

#endif /* SUNXI_HAL_SPI_H */
