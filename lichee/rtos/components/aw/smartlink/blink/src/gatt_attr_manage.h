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

#ifndef _GATT_ATTR_MANAGE_H_
#define _GATT_ATTR_MANAGE_H_

#include "ble/bluetooth/gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	GATT_ATTR_SERVICE,
	GATT_ATTR_CHARACTERISTIC,
	GATT_ATTR_CCC,
} gatt_attr_type;

typedef struct {
	gatt_attr_type type;
	uint8_t properties;
	struct bt_gatt_attr attr;
} bt_gatt_attr_t;

typedef struct gatt_attr_base {
	int (*add)(struct gatt_attr_base *base, bt_gatt_attr_t *param);
	struct bt_gatt_attr* (*get)(struct gatt_attr_base *base, uint32_t attr_index);
	void (*destroy)(struct gatt_attr_base *base);
} gatt_attr_base;

gatt_attr_base *gatt_attr_create(uint32_t attr_num);

#ifdef __cplusplus
}
#endif

#endif /* _GATT_ATTR_MANAGE_H_ */

