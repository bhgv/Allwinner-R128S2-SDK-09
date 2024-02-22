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

#include "net/wlan/wlan_file.h"
#include <stdio.h>
#include <string.h>

#ifdef CONFIG_ARCH_SUN20IW2P1

#define WF_SYSLOG   printf

#define WF_LOG(flags, fmt, arg...)      \
    do {                                \
        if (flags)                      \
            WF_SYSLOG(fmt, ##arg);      \
    } while (0)

#define WF_DBG_ON        0
#define WF_ERR_ON        1
#define WF_INF_ON        1

#define WF_DBG(fmt, arg...) WF_LOG(WF_DBG_ON, "[WF-D] %s():"fmt"\n", __func__, ##arg)
#define WF_ERR(fmt, arg...) WF_LOG(WF_ERR_ON, "[WF-E] %s():"fmt"\n", __func__, ##arg)
#define WF_INF(fmt, arg...) WF_LOG(WF_INF_ON, "[WF-I] %s():"fmt"\n", __func__, ##arg)

#define PATH_MAXLEN  64

/* default use /data dir */
static char wlan_bootloader[PATH_MAXLEN + 1] = "/data/wlan_bl.bin";
static char wlan_firmware[PATH_MAXLEN + 1] = "/data/wlan_fw.bin";
static char wlan_etf_fw[PATH_MAXLEN + 1] = "/data/etf_r128.bin";

// This function will automatically fill in '\0' after 'len' Byte
int wlan_set_file_path(wlan_file_type type, char *path, uint32_t len)
{
	int ret = 0;
	if (len > PATH_MAXLEN) {
		WF_ERR("path len overflow! max:%d, real:%d", PATH_MAXLEN, len);
		return -1;
	}

	switch (type) {
	case FILE_WLAN_BL:
		strncpy(wlan_bootloader, path, len);
		wlan_bootloader[len] = '\0';
		WF_DBG("%s", wlan_bootloader);
		break;
	case FILE_WLAN_FW:
		strncpy(wlan_firmware, path, len);
		wlan_firmware[len] = '\0';
		WF_DBG("%s", wlan_firmware);
		break;
	case FILE_WLAN_ETF:
		strncpy(wlan_etf_fw, path, len);
		wlan_etf_fw[len] = '\0';
		WF_DBG("%s", wlan_etf_fw);
		break;
	default:
		WF_INF("unknown type(%d)", type);
		ret = -1;
		break;
	}

	return ret;
}

char *wlan_get_path(wlan_file_type type)
{
	char *path = NULL;
	switch (type) {
	case FILE_WLAN_BL:
		if (*wlan_bootloader != '\0') {
			WF_DBG("%s", wlan_bootloader);
			path = wlan_bootloader;
		}
		break;
	case FILE_WLAN_FW:
		if (*wlan_firmware != '\0') {
			WF_DBG("%s", wlan_firmware);
			path = wlan_firmware;
		}
		break;
	case FILE_WLAN_ETF:
		if (*wlan_etf_fw != '\0') {
			WF_DBG("%s", wlan_etf_fw);
			path = wlan_etf_fw;
		}
		break;
	default:
		WF_INF("unknown type(%d)", type);
		break;
	}
	return path;
}

int wlan_get_file_path(wlan_file_type type, char *buf, uint32_t len)
{
	char *src = wlan_get_path(type);
	uint32_t src_len = strlen(src);

	if (len <= src_len) {
		WF_ERR("buf len too small! need:%d, real:%d", src_len + 1, len);
		return -1;
	}
	strncpy(buf, src, len);
	return 0;
}

#endif /* CONFIG_ARCH_SUN20IW2P1 */

