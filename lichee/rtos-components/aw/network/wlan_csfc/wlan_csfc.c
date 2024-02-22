/*#####################################################################
# File Describe:wlan_csfc.c
# Author: flyranchaoflyranchao
# Created Time:flyranchao@allwinnertech.com
# Created Time:2022年10月24日 星期一 11时13分47秒
#====================================================================*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <console.h>

#include "sysinfo/sysinfo.h"
#include <sunxi_hal_trng.h>
#include <sunxi_hal_efuse.h>

#include <wlan_csfc.h>
#include <wifimg.h>
#include <freertos_common.h>

#ifdef CONFIG_DRIVERS_XRADIO
#include "wlan.h"
#include "net_init.h"
#include "ethernetif.h"
#include "tcpip_adapter.h"
#include "errno.h"
#endif


#define OEM1_WLAN_MAC_FLAG_START 1408
#define OEM1_WLAN_MAC_FLAG_NUM 1
#define OEM1_WLAN_MAC_START 1409
#define OEM1_WLAN_MAC_NUM 48

uint8_t mac_addr[6];
char mac_addr_char[24] = {0};
extern struct netif *aw_netif[IF_MAX];

#ifdef CONFIG_COMPONENTS_WLAN_MAC_DEFAULT
static void get_mac_by_default()
{
	mac_addr[0] = 0xCC;
	mac_addr[1] = 0xBB;
	mac_addr[2] = 0xAA;
	mac_addr[3] = 0x65;
	mac_addr[4] = 0x43;
	mac_addr[5] = 0x21;
}
#endif

#ifdef CONFIG_COMPONENTS_WLAN_MAC_RANDOM
static void get_mac_by_random(uint8_t sysinfo_mac_addr[6])
{
    uint32_t rand[4];
	int i;
	HAL_TRNG_Extract(0, rand);
    for (i = 0; i < 6; ++i) {
        sysinfo_mac_addr[i] = *(uint8_t *)rand + i;
    }
    sysinfo_mac_addr[0] &= 0xFC;
}
#endif

#ifdef CONFIG_COMPONENTS_WLAN_MAC_PARAM
static void get_mac_by_param(uint8_t sysinfo_mac_addr[6])
{
	printf("===%s: get mac addr by parameter.\n", __func__);
}
#endif
#ifdef CONFIG_COMPONENTS_WLAN_MAC_FILE
static void get_mac_by_file()
{
	int i;
	int fd;
	char *pch;
	char *read_buf;
	if(!access("/data/xr_wifi.conf", R_OK))
	{
		printf("===%s: /data/xr_wifi.conf file is existed, get mac_addr.\n", __func__);
		fd = open("/data/xr_wifi.conf", O_RDWR);
		read_buf = (char *)malloc(18);
		memset(read_buf, 0, 18);
		read(fd, read_buf, 18);
		printf("===%s: read file mac_addr:%s ======\n", __func__, read_buf);
		pch = strtok(read_buf, ":");
		for(i = 0;(pch != NULL) && (i < 6); i++){
			mac_addr[i] = char2uint8(pch);
			pch = strtok(NULL, ":");
		}
		close(fd);
	}else{
		printf("===%s: /data/xr_wifi.conf is not existed!\n", __func__);
	}
}
#endif

#ifdef CONFIG_COMPONENTS_WLAN_MAC_EFUSE
static void set_mac_to_efuse(int argc, char *argv[])
{
	int ret = 0;
	int i;
	int oem1_wlan_mac_flag_value[] = {0x1};
	char *pch;
	uint8_t oem1_wlan_mac_flag[1] = {0};
	uint8_t temp_addr[6] = {0};
	//uint8_t efuse_mac[6] = {0x10, 0x32, 0x54, 0xba, 0xdc, 0xfe};
	uint8_t efuse_mac[6] = {0};
	if (argv[1] != NULL){
		printf("===%s: argv[1]: %s.\n", __func__, argv[1]);
		pch = strtok(argv[1], ":");
		for(i = 0;(pch != NULL) && (i < 6); i++){
			efuse_mac[i] = char2uint8(pch);
			pch= strtok(NULL, ":");
		}
	}else{
		printf("===%s: useg: set_wifimac_efuse 10:32:54:ba:dc:fe\n", __func__);
		return;
	}
	/* write efuse oem1_wlan_mac_flag.*/
	hal_efuse_write_ext(OEM1_WLAN_MAC_FLAG_START, OEM1_WLAN_MAC_FLAG_NUM, oem1_wlan_mac_flag_value);
	/* read efuse oem1_wlan_mac_flag.*/
	ret = hal_efuse_read_ext(OEM1_WLAN_MAC_FLAG_START, OEM1_WLAN_MAC_FLAG_NUM, oem1_wlan_mac_flag);
	if (ret) {
		printf("===%s: read efuse oem1_wlan_mac_flag error.\n", __func__);
	}
	//printf("===%s: read efuse bit: %d-%d, oem1_wlan_mac_flag: %d\n", __func__, start_bit, start_bit + bit_num, *oem1_wlan_mac_flag);
	/* only oem1_wlan_mac_flag = 1, can write efuse oem1_wlan_mac.*/
	if(*oem1_wlan_mac_flag){
		/* write efuse oem1_wlan_mac*/
		ret = hal_efuse_write_ext(OEM1_WLAN_MAC_START, OEM1_WLAN_MAC_NUM, efuse_mac);
		/* read efuse oem1_wlan_mac.*/
		ret = hal_efuse_read_ext(OEM1_WLAN_MAC_START, OEM1_WLAN_MAC_NUM, temp_addr);
		if (ret) {
			printf("===%s: read efuse oem1_wlan_mac error.\n", __func__);
		}
		uint8tochar(mac_addr_char, temp_addr);
		printf("===%s: set mac_addr: %s sucess\n", __func__, mac_addr_char);
	}else
		printf("===%s: write efuse failed, please set oem1_wlan_mac_flag = 1.\n", __func__);

}
FINSH_FUNCTION_EXPORT_CMD(set_mac_to_efuse, set_wifimac_efuse, set wlan_mac to efuse test);

static void get_mac_by_efuse()
{
	int ret = 0;
	uint8_t oem1_wlan_mac_flag[1] = {0};

	/* read efuse oem1_wlan_mac_flag.*/
	ret = hal_efuse_read_ext(OEM1_WLAN_MAC_FLAG_START, OEM1_WLAN_MAC_FLAG_NUM, oem1_wlan_mac_flag);
	if (ret) {
		printf("===%s: read efuse oem1_wlan_mac_flag error.\n", __func__);
	}
	//printf("===%s: read efuse bit: %d-%d, oem1_wlan_mac_flag: %d\n", __func__, start_bit, start_bit + bit_num, *oem1_wlan_mac_flag);

	/* only oem1_wlan_mac_flag = 1, can read efuse oem1_wlan_mac.*/
	if(*oem1_wlan_mac_flag){
		ret = hal_efuse_read_ext(OEM1_WLAN_MAC_START, OEM1_WLAN_MAC_NUM, mac_addr);
		if(ret){
			printf("===%s: read efuse oem1_wlan_mac error.\n", __func__);
		}else{
			uint8tochar(mac_addr_char, mac_addr);
			printf("===%s: read efuse oem1_wlan_mac success: %s\n", __func__, mac_addr_char);
		}
	}else
		printf("===%s: read efuse oem1_wlan_mac failed, please set oem1_wlan_mac_flag = 1.\n", __func__);
}
FINSH_FUNCTION_EXPORT_CMD(get_mac_by_efuse, get_wifimac_efuse, get wlan_mac to efuse test);
#endif

#ifdef CONFIG_COMPONENTS_WLAN_MAC_CHIPID
static void get_mac_by_chipid(uint8_t sysinfo_mac_addr[6])
{
	int i;
	uint8_t chipid[16];

	hal_efuse_read_ext(0, 128, chipid);
	for (i = 0; i < 2; ++i) {
		sysinfo_mac_addr[i] = chipid[i] ^ chipid[i + 6] ^ chipid[i + 12];
	}
	for (i = 2; i < 6; ++i) {
		sysinfo_mac_addr[i] = chipid[i] ^ chipid[i + 6] ^ chipid[i + 10];
	}
	sysinfo_mac_addr[0] &= 0xFC;
}
#endif

/*****************************************
 * get mac_addr
 *****************************************
 * 1.use default data: fe:dc:ba:65:43:21
 * 2.use random data.
 * 3.use parameter data.
 * 4.use file data.
 * 5.use efuse data.
 * 6.use chipid data.
 ****************************************/
void get_wlan_mac(int argc, char *argv[])
{
	/* wlan driver mac address init.*/
#ifdef CONFIG_DRIVERS_XRADIO
#ifdef CONFIG_COMPONENTS_WLAN_MAC_DEFAULT
	printf("===%s: get mac_addr use default value.\n", __func__);
	get_mac_by_default();
#endif

#ifdef CONFIG_COMPONENTS_WLAN_MAC_RANDOM
	printf("===%s: get mac_addr use random data.\n", __func__);
	get_mac_by_random(mac_addr);
#endif

#ifdef CONFIG_COMPONENTS_WLAN_MAC_PARAM
	printf("===%s: get mac_addr use parameter data.\n", __func__);
	get_mac_by_param(argv[1]);
#endif

#ifdef CONFIG_COMPONENTS_WLAN_MAC_FILE
	printf("===%s: get mac_addr use file data\n", __func__);
	get_mac_by_file();
#endif

#ifdef CONFIG_COMPONENTS_WLAN_MAC_EFUSE
	printf("===%s: get mac_addr use efuse data.\n", __func__);
	get_mac_by_efuse();
#endif

#ifdef CONFIG_COMPONENTS_WLAN_MAC_CHIPID
	printf("===%s: get mac_addr use chipid data.\n", __func__);
	get_mac_by_chipid(mac_addr);
#endif
}
#endif

void wlan_csfc()
{
	int fd;
	int n_read;
	char *read_buf;
	char *p = NULL;
	char *q = NULL;
	char *ssid_buf = NULL;
	char *psk_buf = NULL;
	struct stat file_stat;
	int ret;
	int loop_flag = 0;
	wlan_sta_states_t state;
	wifi_sta_cn_para_t cn_para;
	cn_para.sec = 0;
	cn_para.fast_connect = 1;

	/* wifimg init.*/
	wifimanager_init();

#ifdef CONFIG_DRIVERS_XRADIO
	/* get wlan mac.*/
	get_wlan_mac(1, NULL);

	/* set mac_addr.*/
	sysinfo_set_default_mac(mac_addr);
	//default open wlan sta mode
	ret = wifi_on(WIFI_STATION);
	if (ret)
		printf("wlan sta mode init error.\n");
	uint8tochar(mac_addr_char, mac_addr);
	printf("===%s: set mac_addr: %s sucess\n", __func__, mac_addr_char);
	printf("===%s: fast connect begin ...\n", __func__);
	//juge ap info is exist
	//exist: fast connect when system start.
	//not exist: enter peiwang mode.
	if(!access("/data/wpa_supplicant.conf", R_OK))
	{
		fd = open("/data/wpa_supplicant.conf", O_RDWR);
		ret = fstat(fd, &file_stat);
		if (ret == -1) {
			printf("===%s: get file state failed.\n", __func__);
		} else{
			printf("===%s: get file size: %d\n", __func__, file_stat.st_size);
			read_buf = (char *)malloc(file_stat.st_size + 1);
			memset(read_buf, 0, file_stat.st_size +1);
			n_read = read(fd, read_buf, file_stat.st_size);
			printf("===%s: read %d, context:%s\n", __func__, n_read, read_buf);
			p = read_buf;
			while ((q = strchr(p, ':')) != NULL){
				if (loop_flag == 0){
					*q = '\0';
					ssid_buf = p;
					cn_para.ssid = ssid_buf;
					printf("===%s: ssid_buf:[%s], len: %d\n", __func__,  ssid_buf, strlen(ssid_buf));
				}
				if (loop_flag == 1){
					*q = '\0';
					psk_buf = p;
					cn_para.password = psk_buf;
					printf("===%s: psk_buf:[%s], len: %d\n", __func__,  psk_buf, strlen(psk_buf));
				}
				p = q + 1;
				loop_flag += 1;
			}
			if (p != NULL){
				if ( strcmp(p, "WIFI_SEC_NONE") == 0)
					cn_para.sec = WIFI_SEC_NONE;
				if ( strcmp(p, "WIFI_SEC_WPA_PSK") == 0)
					cn_para.sec = WIFI_SEC_WPA_PSK;
				if ( strcmp(p, "WIFI_SEC_WPA2_PSK") == 0)
					cn_para.sec = WIFI_SEC_WPA2_PSK;
				if ( strcmp(p, "WIFI_SEC_WPA3_PSK") == 0)
					cn_para.sec = WIFI_SEC_WPA3_PSK;
				if ( strcmp(p, "WIFI_SEC_WEP") == 0)
					cn_para.sec = WIFI_SEC_WEP;
				printf("===%s: sec_buf:[%s]\n", __func__,  p);
			}

			close(fd);
		}
		if (ssid_buf != NULL)
			ret = wifi_sta_connect(&cn_para);
		else
			printf("===%s: fast connect failed, wpa_supplicant.conf is exist, but is empty.\n", __func__);

	} else
		printf("===%s: fast connect failed, wpa_supplicant.conf is not exist, please connect wifi first.\n", __func__);

#endif
}
