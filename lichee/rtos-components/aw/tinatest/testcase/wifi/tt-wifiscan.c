#include <tinatest.h>
#include "tt-wifitest.h"
#include <sys/times.h>

//wifi scan test
//cmd:tt wscanwifi

#define BAND_NOME     0
#define BAND_2_4G     1
#define BAND_5G       2
#define RES_LEN 50

static void uint8tochar(char *mac_addr_char, uint8_t *mac_addr_uint8)
{
	sprintf(mac_addr_char,"%02x:%02x:%02x:%02x:%02x:%02x",
						(unsigned char)mac_addr_uint8[0],
						(unsigned char)mac_addr_uint8[1],
						(unsigned char)mac_addr_uint8[2],
						(unsigned char)mac_addr_uint8[3],
						(unsigned char)mac_addr_uint8[4],
						(unsigned char)mac_addr_uint8[5]);
	mac_addr_char[17] = '\0';
}

static char * key_mgmt_to_char(char *key_mgmt_buf, wifi_sec key_mgmt)
{
	if(key_mgmt & WIFI_SEC_WEP) {
		strcat(key_mgmt_buf, "[WEP]");
	}
	if(key_mgmt & WIFI_SEC_WPA_PSK) {
		strcat(key_mgmt_buf, "[WPA]");
	}
	if(key_mgmt & WIFI_SEC_WPA2_PSK) {
		strcat(key_mgmt_buf, "[WPA2]");
	}
	if(key_mgmt & WIFI_SEC_WPA3_PSK) {
		strcat(key_mgmt_buf, "[WPA3]");
	}
	if(key_mgmt == WIFI_SEC_NONE) {
		strcat(key_mgmt_buf, "[NONE]");
	}
	return key_mgmt_buf;
}

static uint32_t freq_to_channel(uint32_t freq)
{
	int band;
	uint32_t channel = 0;
	if((freq >= 5180) && (freq <= 5825)){
		band = BAND_5G;
	} else if((freq >= 2407) && (freq <= 2484)){
		band = BAND_2_4G;
	} else {
		band = BAND_NOME;
	}
	switch (band) {
	case BAND_2_4G:
		if(freq == 2484) {
			channel = 14;
		} else if(freq == 2407) {
			channel = 0;
		} else if((freq <= 2472) && (freq > 2407)){
			if(((freq - 2407) % 5) == 0) {
				channel = ((freq - 2407) / 5);
			} else {
				channel = 1000;
			}
		} else {
			channel = 1000;
		}
		break;
	case BAND_5G:
		if(((freq - 5180) % 5) == 0) {
			channel = ((freq - 5180) / 5) + 36;
		} else {
			channel = 1000;
		}
		break;
	case BAND_NOME:
	default:
		channel = 1000;
		break;
	}
}

int tt_wifi_scan_test(int argc, char **argv)
{
    wmg_status_t ret = WMG_STATUS_FAIL;
    char scan_res_char[20] = {0};
    int get_scan_results_num = 0;
    char mac_addr_char[18] = {0};
    int scan_max_num = RES_LEN;
    wifi_scan_result_t *scan_res = NULL;
    int i, bss_num = 0;

    scan_res = (wifi_scan_result_t *)malloc(sizeof(wifi_scan_result_t) * scan_max_num);
    if(scan_res == NULL) {
        printf("malloc scan_res fail\n");
        return WMG_STATUS_FAIL;
    }

	wifimanager_init();
	wifi_on(WIFI_STATION);
    ret = wifi_get_scan_results(scan_res, &bss_num, scan_max_num);
    if (ret == WMG_STATUS_SUCCESS) {
        get_scan_results_num = (scan_max_num > bss_num ? bss_num : scan_max_num);
        for (i = 0; i < get_scan_results_num; i++) {
            memset(scan_res_char, 0, 20);
            memset(mac_addr_char, 0, 18);
            uint8tochar(mac_addr_char, scan_res[i].bssid);
            printf("[%02d]: bssid=%s  channel=%-3d  freq=%d  rssi=%d  sec=%-12s  ssid=%s\n",
                    i, mac_addr_char, freq_to_channel(scan_res[i].freq),scan_res[i].freq,
                    scan_res[i].rssi, key_mgmt_to_char(scan_res_char, scan_res[i].key_mgmt), scan_res[i].ssid);
        }
        printf("==Wi-Fi scan successful, total %d ap(scan_max_num: %d)==\n", bss_num, scan_max_num);
        ret = WMG_STATUS_SUCCESS;
    } else {
        printf("==Wi-Fi scan failed==\n");
        ret = WMG_STATUS_FAIL;
    }
    if(scan_res != NULL) {
        free(scan_res);
        scan_res = NULL;
    }

	printf("\n========TINATEST FOR WIFI SCAN========\n");
	if(ret > 0)
		printf("\n=======TINATEST FOR WIFI SCAN OK=======\n");
	else
		printf("\n=======TINATEST FOR WIFI SCAN FAIL=======\n");

    return (ret <= 0 ? ret : 0);
}
testcase_init(tt_wifi_scan_test, wifi_scan, wifi scan for tinatest);
