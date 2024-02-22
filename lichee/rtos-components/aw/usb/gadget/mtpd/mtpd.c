/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
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
#include <string.h>
#include <unistd.h>
#include <console.h>
#include <aw_common.h>
#include <hal_sem.h>
#include <sunxi_hal_usb.h>

#include "MtpServer.h"
#include "MtpDataBase.h"
#include "Disk.h"
#include "mtp.h"

#define KB(size)    (size * 1024)
#define MB(size)    (KB(size) * 1024)

#define AID_MEDIA_RW    1023

int g_mtpd_debug_mask = 0;

extern int mtp_exit;
extern int usb_gadget_mtp_init();
extern int usb_gadget_mtp_deinit();
extern int poll_usb_state(unsigned int *state);

enum {
	USB_STATE_UNKNOWN = 0,
	USB_STATE_CONNECTED = 1,
	USB_STATE_DISCONNECTED,
};


hal_thread_t mtp_thread;

typedef struct {
	struct MtpStorage *storage;
	uint32_t id;
	char path[256];
	char description[32];
	uint64_t reserve_space;
	uint64_t max_file_size;
} mtp_storage_t;

static mtp_storage_t gStorageArray[] = {
	{NULL, 65537, "/data", "RTOS MTP", 0, 0},
	// {NULL, 65537, "/tmp", "RTOS MTP", 0, 0},
};

typedef struct {
	struct MtpServer *server;
	mtp_storage_t *storage_array;
	uint32_t storage_array_size;
	int controlFd;
} mtp_handle_t;

static mtp_handle_t gMtpHandle = {
	.server = NULL,
	.storage_array = gStorageArray,
	.storage_array_size = sizeof(gStorageArray) / sizeof(mtp_storage_t),
	.controlFd = -1,
};

static int mtp_storage_add(mtp_storage_t *storage)
{
	storage->storage =
		MtpStorageInit(storage->id, storage->path,
				storage->description,
				storage->reserve_space,
				storage->max_file_size);
	if (!storage->storage)
		return -1;
	/* Add storage into MtpServer */
	MtpServerAddStorage(storage->storage, gMtpHandle.server);
	/* Add storage into MtpDatabase */
	MtpDataBaseAddStorage(storage->storage, gMtpHandle.server->mDataBase);

	return 0;
}

static void mtp_storage_del(mtp_storage_t *storage)
{
#if 0
	MtpDataBaseDelStorage(storage->storage, gMtpHandle.server->mDataBase);
	MtpServerDelStorage(storage->storage, gMtpHandle.server);
#endif
	MtpStorageRelease(storage->storage);
}

static int mtp_server_init()
{
	int i = 0, ret = 0;

	mtp_info("MtpServer init!\n");

	gMtpHandle.server = MtpServerInit(AID_MEDIA_RW, 0664, 0775, gMtpHandle.controlFd);
	if (!gMtpHandle.server) {
		mtp_err("MtpServer init failed\n");
		return -1;
	}

	mtp_info("MtpStroage init!\n");
	for (i = 0; i < gMtpHandle.storage_array_size; i++) {
		mtp_storage_t *storage = &gMtpHandle.storage_array[i];
		ret = mtp_storage_add(storage);
		if (ret < 0)
			mtp_err("mtp add storage failed\n");
	}

	mtp_info("mtp server init ok, run server\n");
	gMtpHandle.server->run(gMtpHandle.server);
}

static void mtp_server_exit()
{
	int i = 0;

	if (!gMtpHandle.server)
		return;

	printf("MtpServer exit!\n");

	MtpServerRelease(gMtpHandle.server);
	gMtpHandle.server = NULL;

	mtp_info("mtp server exit, del storage\n");
	for (i = 0; i < gMtpHandle.storage_array_size; i++) {
		mtp_storage_t *storage = &gMtpHandle.storage_array[i];
		if (storage->storage != NULL) {
			mtp_storage_del(storage);
			storage->storage = NULL;
		}
	}
}

static void MtpServerControlThread(void *arg)
{
	int ret = 0;
	unsigned int state = USB_STATE_UNKNOWN, old_state = USB_STATE_UNKNOWN;

	while (1) {
		poll_usb_state(&state);
		mtp_info("usb state change: %d -> %d\n", state, old_state);
		if (state != old_state) {
			switch (state) {
			case USB_STATE_UNKNOWN:
				break;
			case USB_STATE_CONNECTED:
				mtp_server_exit();
				mtp_server_init();
				break;
			case USB_STATE_DISCONNECTED:
				mtp_server_exit();
				break;
			}
		}
		old_state = state;
	}
}

static void mtpd_version(void)
{
	printf("Mtp Daemon version:%s, compiled on: %s %s\n", MTPD_VERSION, __DATE__, __TIME__);
}

int mtpd_main(void)
{
	int ret = 0;

	mtp_exit = 0;

	mtp_thread = hal_thread_create(MtpServerControlThread, NULL,
			"mtp_server_thread", 2048, HAL_THREAD_PRIORITY_APP);
	if (!mtp_thread) {
		mtp_err("mtpd create thread failed\n");
		return -1;
	}

	mtpd_version();
	ret = usb_gadget_mtp_init();
	if (ret < 0) {
		mtp_err("mtpd init failed\n");
		return -1;
	}

	ret = usb_gadget_function_enable("mtp");
	if (ret < 0) {
		 mtp_err("mtpd enable failed\n");
		 return -1;
	}


	printf("mtpd service init successful\n");

	return 0;
}

int mtpd_exit(void)
{
	hal_sem_t mtp_sem = NULL;

	mtp_sem = hal_sem_create(0);

	while (mtp_exit == 0)
		hal_sem_timedwait(mtp_sem, MS_TO_OSTICK(100));

	usb_gadget_function_disable("mtp");
	usb_gadget_mtp_deinit();
	hal_sem_delete(mtp_sem);

	return 0;
}

static void usage(void)
{
	printf("Usgae: mtpd [option]\n");
	printf("-v,          mtpd version\n");
	printf("-d,          mtpd debug option\n");
	printf("-h,          mtpd help\n");
	printf("\n");
}

int cmd_mtpd(int argc, char *argv[])
{
	int ret = 0, c;

	optind = 0;
	while ((c = getopt(argc, argv, "vd:h")) != -1) {
		switch (c) {
		case 'v':
			mtpd_version();
			return 0;
		case 'd':
			g_mtpd_debug_mask = atoi(optarg);
			return 0;
		case 'h':
		default:
			usage();
			return 0;
		}
	}

	ret = mtpd_main();

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_mtpd, mtpd, mtpd service);
