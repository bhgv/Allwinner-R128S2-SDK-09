/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdio.h>

#include "bt_lib.h"

#include "bt_vendor.h"

#define H4_TYPE_CMD        0x01
#define H4_TYPE_ACL        0x02
#define H4_TYPE_SCO        0x03
#define H4_TYPE_EVT        0x04

static const bt_lib_interface_t *bt_lib_if = NULL;
static int bt_vs_id;

int vs_hci_write(uint8_t type, uint8_t *data, uint16_t len)
{
	unsigned char status = 0;

	if ((bt_lib_if != NULL) && !bt_lib_if->hci_ctrl_ops->status()) {
		printf("vs write err type %d!\n", type);
		return -1;
	}

	if (type == H4_TYPE_CMD) {
		status = bt_lib_if->hci_ops->write(bt_vs_id, type, data, len);
		if (status != 0) {
			printf("vs write err %d %d!\n", type, status);
			return status;
		}
	} else {
		printf("vs write err type %d!\n", type);
		return -1;
	}

	return status;
}

static int vs_data_ind(const uint8_t *data, uint16_t len)
{
	uint8_t hci_type;
	const uint8_t *stream = data;

	STREAM_TO_U8(hci_type, stream);

	if (hci_type != H4_TYPE_EVT) {
		printf("hci type errr: %d\n", hci_type);
		return 1;
	}

	return vs_recv(stream, len - 1);
}

static bt_hc_callbacks_t vs_hc_callbacks = {
	.data_ind = vs_data_ind,
};

int vs_hci_open(void)
{
	bt_lib_if = bt_lib_get_interface();
	if (bt_lib_if && bt_lib_if->hci_ops && bt_lib_if->hci_ops->open) {
		bt_vs_id = bt_lib_if->hci_ops->open(BT_FEATURES_VS, &vs_hc_callbacks);
		if (bt_vs_id >= 0)
			return 0;
	}
	return -1;
}

int vs_hci_close(void)
{
	if (bt_lib_if && bt_lib_if->hci_ops && bt_lib_if->hci_ops->close) {
		bt_lib_if->hci_ops->close(bt_vs_id);
		bt_vs_id = -1;
		bt_lib_if = NULL;
	}
	return 0;
}

static const struct vs_driver drv = {
	.name = "",
	.open = vs_hci_open,
	.close = vs_hci_close,
	.send = vs_hci_write,
};

const struct vs_driver * bt_ctrl_get_vs_interface(void)
{
	return &drv;
}

