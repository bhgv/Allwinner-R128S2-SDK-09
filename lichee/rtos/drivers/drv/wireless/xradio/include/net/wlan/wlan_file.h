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
#ifndef _XR_WLAN_FILE_H_
#define _XR_WLAN_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_ARCH_SUN20IW2P1

typedef enum wlan_file_type {
	FILE_WLAN_BL = 0,
	FILE_WLAN_FW,
	FILE_WLAN_ETF,
} wlan_file_type;

int wlan_set_file_path(wlan_file_type type, char *path, uint32_t len);
int wlan_get_file_path(wlan_file_type type, char *buf, uint32_t len);

#ifdef CONFIG_ARCH_ARM_ARMV8M
char *wlan_get_path(wlan_file_type type);
#define WLAN_BOOTLOADER  wlan_get_path(FILE_WLAN_BL)
#define WLAN_FIRMWARE    wlan_get_path(FILE_WLAN_FW)
#define WLAN_ETF_FW      wlan_get_path(FILE_WLAN_ETF)
#endif

#endif /* CONFIG_ARCH_SUN20IW2P1 */

#ifdef __cplusplus
}
#endif
#endif /* _XR_WLAN_FILE_H_ */
