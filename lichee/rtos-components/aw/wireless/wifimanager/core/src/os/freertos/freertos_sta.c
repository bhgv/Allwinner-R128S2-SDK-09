#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <wpa_ctrl.h>
#include <freertos_sta.h>
#include <udhcpc.h>
#include <utils.h>
#include <wifi_log.h>
#include <unistd.h>
#include <wmg_sta.h>
#include <freertos/event.h>
#include <freertos/udhcpc.h>
#include <freertos/scan.h>
#include <freertos_common.h>
#include "wlan.h"
#include "net_ctrl.h"
#include "net_init.h"
#include "sysinfo.h"
#include "cmd_util.h"
#include "wlan_ext_req.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"

#define SLEEP_TIMES 15
#define CONNECT_TIMEOUT 200
#define SCAN_TIMEOUT 100
#define SCAN_NUM_PROBES 2
#define SCAN_PROBE_DELAY 100
#define SCAN_MIN_DWELL 100
#define SCAN_MAX_DWELL 125

static wmg_sta_inf_object_t sta_inf_object;
static int call_disconnect = 0; /* "net sta disconnect" must mate "net sta conect". */
static int autoconn_disabled = 0; /* wlan_sta_set_autoconn(0) must mate wlan_sta_set_autoconn(1). */
static int scan_state = 0;
static int mac_state = 0;
static int connect_status = 0;
static const char * const net_ctrl_msg_str[] = {
	"wifimg connected",
	"wifimg disconnected",
	"wifimg scan success",
	"wifimg scan failed",
	"wifimg 4way handshake failed",
	"wifimg ssid not found",
	"wifimg auth timeout",
	"wifimg disassoc",
	"wifimg associate timeout",
	"wifimg connect failed",
	"wifimg connect loss",
	"wifimg associate failed",
	"wifimg SAE auth-commit failed",
	"wifimg SAE auth-confirm failed",
	"wifimg dev hang",
	"wifimg ap-sta connected",
	"wifimg ap-sta disconnected",
	"network dhcp start",
	"network dhcp timeout",
	"network up",
	"network down",
#if (!defined(CONFIG_LWIP_V1) && LWIP_IPV6)
	"network IPv6 state",
#endif
};

static void sta_event_notify_to_sta_dev(wifi_sta_event_t event)
{
	if (sta_inf_object.sta_event_cb) {
		sta_inf_object.sta_event_cb(event);
	}
}

static void freertos_sta_net_msg_process(uint32_t event, uint32_t data, void *arg)
{
	uint16_t type = EVENT_SUBTYPE(event);
	WMG_DEBUG("recv msg <%s>\n", net_ctrl_msg_str[type]);
	WMG_DEBUG("wifi state: %d, connect_status: %d\n", mac_state, connect_status);

	WMG_DEBUG("event : %d \n", type);
	wlan_ext_stats_code_get_t param;
	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
		sta_event_notify_to_sta_dev(WIFI_CONNECTED);
		mac_state = 1;
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		sta_event_notify_to_sta_dev(WIFI_DISCONNECTED);
		/* flush scan buf. */
		wlan_sta_bss_flush(0);
		connect_status = 0;
		mac_state = 0;
		break;
	case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
		WMG_DEBUG("%s() scan_state:%d, we will set it to 1!\n", __func__, scan_state);
		sta_event_notify_to_sta_dev(WIFI_SCAN_RESULTS);
		scan_state = 1;
		break;
	case NET_CTRL_MSG_WLAN_SCAN_FAILED:
		sta_event_notify_to_sta_dev(WIFI_SCAN_FAILED);
		break;
	case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
		sta_event_notify_to_sta_dev(WIFI_4WAY_HANDSHAKE_FAILED);
		mac_state = 0;
		break;
	case NET_CTRL_MSG_WLAN_SSID_NOT_FOUND:
		sta_event_notify_to_sta_dev(WIFI_NETWORK_NOT_FOUND);
		mac_state = 0;
		break;
	case NET_CTRL_MSG_WLAN_AUTH_TIMEOUT:
		sta_event_notify_to_sta_dev(WIFI_AUTH_TIMEOUT);
		mac_state = 0;
		break;
	case NET_CTRL_MSG_WLAN_ASSOC_TIMEOUT:
		sta_event_notify_to_sta_dev(WIFI_AUTH_FAILED);
		mac_state = 0;
		break;
	case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
		sta_event_notify_to_sta_dev(WIFI_CONNECT_FAILED);
		mac_state = 0;
		break;
	case NET_CTRL_MSG_WLAN_DEV_HANG:
		break;
	case NET_CTRL_MSG_WLAN_CONNECTION_LOSS:
		break;
	case NET_CTRL_MSG_WLAN_ASSOC_FAILED:
		sta_event_notify_to_sta_dev(WIFI_ASSOC_FAILED);
		mac_state = 0;
		break;
	case NET_CTRL_MSG_WLAN_SAE_COMMIT_FAILED:
		break;
	case NET_CTRL_MSG_WLAN_SAE_CONFIRM_FAILED:
		break;
	case NET_CTRL_MSG_WLAN_AP_STA_DISCONNECTED:
		break;
	case NET_CTRL_MSG_WLAN_DISASSOC:
		sta_event_notify_to_sta_dev(WIFI_DISASSOC);
		mac_state = 0;
		break;
	case NET_CTRL_MSG_WLAN_AP_STA_CONNECTED:
		break;
	case NET_CTRL_MSG_NETWORK_DHCP_START:
		sta_event_notify_to_sta_dev(WIFI_DHCP_START);
		break;
	case NET_CTRL_MSG_NETWORK_DHCP_TIMEOUT:
		sta_event_notify_to_sta_dev(WIFI_DHCP_TIMEOUT);
		connect_status = 0;
		break;
	case NET_CTRL_MSG_NETWORK_UP:
		sta_event_notify_to_sta_dev(WIFI_DHCP_SUCCESS);
		connect_status = 1;
		if (scan_state) {
			scan_state = 0;
		}
		break;
	case NET_CTRL_MSG_NETWORK_DOWN:
		connect_status = 0;
		break;
	default:
		sta_event_notify_to_sta_dev(WIFI_UNKNOWN);
		WMG_WARNG("unknown msg (%u, %u)\n", type, data);
		break;
	}
	WMG_DEBUG("deal msg <%s>\n", net_ctrl_msg_str[type]);
	WMG_DEBUG("wifi state: %d, connect_status: %d\n", mac_state, connect_status);
}

static wmg_status_t freertos_sta_mode_init(sta_event_cb_t sta_event_cb, void *para)
{
	WMG_DEBUG("sta mode init\n");
	if (wlan_get_init_status() == WLAN_STATUS_NO_INIT) {
		net_core_init();
		wlan_set_init_status(WLAN_STATUS_INITED);
	}
	if (sta_event_cb != NULL){
		sta_inf_object.sta_event_cb = sta_event_cb;
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_mode_enable()
{
	WMG_DEBUG("sta mode enable\n");
	int ret = 0;
	ret = net_switch_mode(WLAN_MODE_STA);
	if (ret)
		return WMG_STATUS_FAIL;

	observer_base *ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK, NET_CTRL_MSG_ALL, freertos_sta_net_msg_process, NULL);
	if (ob == NULL) {
		WMG_ERROR("freertos callback observer create failed\n");
		return WMG_STATUS_FAIL;
	}
	if (sys_ctrl_attach(ob) != 0) {
		WMG_ERROR("freertos callback observer attach failed\n");
		sys_callback_observer_destroy(ob);
		return WMG_STATUS_FAIL;
	} else {
		/* use sta_private_data to save observer_base point. */
		sta_inf_object.sta_private_data = ob;
	}
	/*set scan param.*/
	wlan_ext_scan_param_t param;
	param.num_probes = SCAN_NUM_PROBES;
	param.probe_delay = SCAN_PROBE_DELAY;
	param.min_dwell = SCAN_MIN_DWELL;
	param.max_dwell = SCAN_MAX_DWELL;
	wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_SCAN_PARAM, (uint32_t)(uintptr_t)(&param));

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_mode_disable()
{
	/* nothing to do in freertos */
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_mode_deinit(void *para)
{
	WMG_DEBUG("sta mode deinit\n");
	observer_base *ob = (observer_base *)sta_inf_object.sta_private_data;
	if(ob) {
		sys_ctrl_detach(ob);
		sys_callback_observer_destroy(ob);
		sta_inf_object.sta_private_data = NULL;
	}
	if(g_wlan_netif) {
		net_close(g_wlan_netif);
		g_wlan_netif = NULL;
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_disconnect()
{
	/* disconnect ap and report WIFI_DISCONNECTED to wifimg */
	WMG_DEBUG("sta disconnect\n");
	int sleep_times = SLEEP_TIMES;
	struct netif *nif = g_wlan_netif;
	wlan_sta_disconnect();
	call_disconnect = 1;
	wlan_sta_disable();
	while(mac_state) {
		if(sleep_times <= 0)
			break;
		sleep_times--;
		usleep(1000 * 1000);
	}

	return WMG_STATUS_SUCCESS;
}

int wifi_wep_connect(const char *ssid, const char *passwd)
{
    uint8_t ssid_len;
	int sleep_times = SLEEP_TIMES;
    wlan_sta_config_t config;
    WMG_DEBUG("%s,ssid %s,passwd, %s\n", __func__, ssid, passwd);

    if (ssid)
        ssid_len = strlen(ssid);
    else
        goto err;


    if (ssid_len > WLAN_SSID_MAX_LEN)
        ssid_len = WLAN_SSID_MAX_LEN;

    memset(&config, 0, sizeof(config));

    /* ssid */
    config.field = WLAN_STA_FIELD_SSID;
    memcpy(config.u.ssid.ssid, ssid, ssid_len);
    config.u.ssid.ssid_len = ssid_len;
    if (wlan_sta_set_config(&config) != 0)
        goto err;

    /* WEP key0 */
    config.field = WLAN_STA_FIELD_WEP_KEY0;
    strlcpy((char *)config.u.wep_key, passwd, sizeof(config.u.wep_key));
    if (wlan_sta_set_config(&config) != 0)
        return -1;

    /* WEP key index */
    config.field = WLAN_STA_FIELD_WEP_KEY_INDEX;
    config.u.wep_tx_keyidx = 0;
    if (wlan_sta_set_config(&config) != 0)
        goto err;

    /* auth_alg: OPEN */
    config.field = WLAN_STA_FIELD_AUTH_ALG;
    config.u.auth_alg = WPA_AUTH_ALG_OPEN | WPA_AUTH_ALG_SHARED;
    if (wlan_sta_set_config(&config) != 0)
        goto err;

    /* key_mgmt: NONE */
    config.field = WLAN_STA_FIELD_KEY_MGMT;
    config.u.key_mgmt = WPA_KEY_MGMT_NONE;
    if (wlan_sta_set_config(&config) != 0)
        goto err;

    if (wlan_sta_enable()!= 0)
        goto err;

	while(sleep_times >= 6) {
		usleep(2000 * 1000);
		WMG_DEBUG("try wep connect\n");
		if (connect_status == 0) {
			wlan_sta_disable();
			WMG_DEBUG("%s, WPA_AUTH_ALG_SHARED\n", __func__);
			config.field = WLAN_STA_FIELD_AUTH_ALG;
			config.u.auth_alg = WPA_AUTH_ALG_SHARED;
			if (wlan_sta_set_config(&config) != 0)
				goto err;
			break;
		}
		if (connect_status == 1) {
			WMG_DEBUG("%s, WPA_AUTH_ALG_OPEN\n", __func__);
			break;
		}
		sleep_times--;
    }

    return 0;

err:
    WMG_ERROR("connect ap failed\n");
    return -1;
}

static wmg_status_t freertos_sta_connect(wifi_sta_cn_para_t *cn_para)
{
	WMG_DEBUG("sta connect\n");
	if(cn_para->ssid == NULL) {
		WMG_ERROR("freertos os unsupport bssid connect\n");
		sta_event_notify_to_sta_dev(WIFI_DISCONNECTED);
		return WMG_STATUS_UNSUPPORTED;
	}
	int sleep_times = CONNECT_TIMEOUT;
#ifdef CONFIG_ARCH_SUN20IW2P1
	int ret;
	char *sec_buf;
	WMG_DEBUG("wifi state: %d\n", mac_state);
	if (mac_state != 0){
		wlan_sta_disable();
	}

	/* judge wifi encryption method */
	WMG_DEBUG("fast_connect :%d, ssid: %s, psk: %s sec: %d\n", cn_para->fast_connect, cn_para->ssid, cn_para->password, cn_para->sec);
	switch (cn_para->sec) {
		case WIFI_SEC_NONE:
			ret = wlan_sta_set((uint8_t *)cn_para->ssid, strlen(cn_para->ssid),(uint8_t *)cn_para->password);
			sec_buf = "WIFI_SEC_NONE";
			break;
		case WIFI_SEC_WPA_PSK:
			ret = wlan_sta_set((uint8_t *)cn_para->ssid, strlen(cn_para->ssid),(uint8_t *)cn_para->password);
			sec_buf = "WIFI_SEC_WPA_PSK";
			break;
		case WIFI_SEC_WPA2_PSK:
			ret = wlan_sta_set((uint8_t *)cn_para->ssid, strlen(cn_para->ssid),(uint8_t *)cn_para->password);
			sec_buf = "WIFI_SEC_WPA2_PSK";
			break;
		case WIFI_SEC_WPA3_PSK:
			ret = wlan_sta_set((uint8_t *)cn_para->ssid, strlen(cn_para->ssid),(uint8_t *)cn_para->password);
			sec_buf = "WIFI_SEC_WPA3_PSK";
			break;
		case WIFI_SEC_WEP:
			wifi_wep_connect(cn_para->ssid, cn_para->password);
			sec_buf = "WIFI_SEC_WEP";
			break;

		default:
			WMG_ERROR("unknown key mgmt\n");
			return WMG_STATUS_FAIL;
	}

	wlan_sta_enable();
	if(call_disconnect || autoconn_disabled){
		wlan_sta_connect(); /* if call "waln_sta_disconnect, must call wlan_sta_connect". */
		call_disconnect = 0;
		autoconn_disabled = 0;
	}

	/* wait for wifi connected, until 50us * 200 = 10s timeout. */
	while(!connect_status) {
		if(sleep_times <= 0)
			break;
		sleep_times--;
		usleep(50 * 1000);
	}

	/* judge if fast connect mode, do not save ap. */
	if(cn_para->fast_connect){
		if(connect_status == 1){
			WMG_INFO("sta fast connect successful.\n");
			return WMG_STATUS_SUCCESS;
		}else{
			WMG_ERROR("sta fast connect failed.\n");
            /* notify connect_timeout, or will in connecting status. */
            sta_event_notify_to_sta_dev(WIFI_CONNECT_TIMEOUT);
            /* disable wlan sta, or driver will always try connect. */
            wlan_sta_disable();
            return WMG_STATUS_FAIL;
        }
	}

	/* wifi connected, save ap info. */
	if(connect_status){
		/* if file existed, we should not create new one */
		int fd;
		int n_write;
		int n_read;
		char *buf_temp = ":";
		char *read_buf;

		if(!access("/data/wpa_supplicant.conf", R_OK))
		{
			WMG_DEBUG("wpa_supplicant.conf file is existed\n");
			fd = open("/data/wpa_supplicant.conf", O_RDWR | O_TRUNC);
			n_write = 0;
			n_write += write(fd, cn_para->ssid, strlen(cn_para->ssid));
			n_write += write(fd, buf_temp, strlen(buf_temp));
			n_write += write(fd, cn_para->password, strlen(cn_para->password));
			n_write += write(fd, buf_temp, strlen(buf_temp));
			n_write += write(fd, sec_buf, strlen(sec_buf));
			if(n_write != -1)
				WMG_DEBUG("write %d byte to wpa_supplicant.conf\n", n_write);
			close(fd);
		}else{
			WMG_DEBUG("wpa_supplicant.conf file is not existed\n");
			fd = open("/data/wpa_supplicant.conf", O_RDWR | O_CREAT | O_WRONLY | O_TRUNC, 0666);
			n_write = 0;
			n_write += write(fd, cn_para->ssid, strlen(cn_para->ssid));
			n_write += write(fd, buf_temp, strlen(buf_temp));
			n_write += write(fd, cn_para->password, strlen(cn_para->password));
			n_write += write(fd, buf_temp, strlen(buf_temp));
			n_write += write(fd, sec_buf, strlen(sec_buf));
			if(n_write != -1)
				WMG_DEBUG("write %d byte to wpa_supplicant.conf\n", n_write);
			close(fd);
		}

		WMG_DEBUG("ssid: %s password: %s\n", cn_para->ssid, cn_para->password);
		fd = open("/data/wpa_supplicant.conf", O_RDWR);
		read_buf = (char *)malloc(n_write+1);
		memset(read_buf, 0, n_write + 1);
		n_read = read(fd, read_buf, n_write);
		WMG_DEBUG("read %d, context:%s\n", n_read, read_buf);
		close(fd);
		return WMG_STATUS_SUCCESS;
	}
	/* connect failed, print state then disable wifi. */
	WMG_DEBUG("wifi state: %d, connect_status: %d\n", mac_state, connect_status);
    /* notify connect_timeout, or will in connecting status. */
	sta_event_notify_to_sta_dev(WIFI_CONNECT_TIMEOUT);
	/* disable wlan sta, or driver will always try connect. */
	wlan_sta_disable();
	return WMG_STATUS_FAIL;
#endif /* CONFIG_ARCH_SUN20IW2P1 */

#ifdef CONFIG_ARCH_SUN8IW18P1
	WMG_ERROR("ssid:%s, passwd:%s\n", cn_para->ssid, cn_para->password);
	wifi_connect(cn_para->ssid, cn_para->password);
	sta_event_notify_to_sta_dev(WIFI_CONNECTED);
#endif /* CONFIG_ARCH_SUN20IW2P1 */
}

static wmg_status_t freertos_sta_auto_connect(bool *autoconnect)
{
	int sleep_times = CONNECT_TIMEOUT;
	/* set wifi auto connect disable. */
	if (!(*autoconnect)){
		WMG_DEBUG("sta set auto connect disable\n");
		wlan_sta_set_autoconnect(0);
		autoconn_disabled = 1;
		return WMG_STATUS_SUCCESS;
	}

	/* set wifi auto connect enable. */
	if(mac_state && connect_status){
		WMG_DEBUG("wifi has already connected!\n");
		return WMG_STATUS_SUCCESS;
	}
	/* check /data/wpa_supplicant.conf file is exist. */
	if(access("/data/wpa_supplicant.conf", R_OK))
	{
		WMG_DEBUG("wpa_supplicant.conf file is not existed\n");
		return WMG_STATUS_FAIL;
	}

	/* check /data/wpa_supplicant.conf file is empty. */
	FILE* file1 = fopen("/data/wpa_supplicant.conf", "r");
	int c = fgetc(file1);
	if (c == EOF) {
		WMG_DEBUG("wpa_supplicant.conf file is empty\n");
		fclose(file1);
		return WMG_STATUS_FAIL;
	}
	fclose(file1);

	/* start auto connect. */
	WMG_DEBUG("sta set auto connect enable\n");
	wlan_sta_set_autoconnect(1);
	autoconn_disabled = 0;
	wlan_sta_enable();
	wlan_sta_connect();

	/* wait for wifi dhcp successed, until 50us * 200 = 10s timeout. */
	while(!connect_status) {
		if(sleep_times <= 0)
			break;
		sleep_times--;
		usleep(50 * 1000);
	}

	if(mac_state && connect_status){
		WMG_DEBUG("wifi reconnect success!\n");
		return WMG_STATUS_SUCCESS;
	}else {
		WMG_DEBUG("wifi reconnect failed!\n");
		return WMG_STATUS_FAIL;
	}
}

static wmg_status_t freertos_sta_get_info(wifi_sta_info_t *sta_info)
{
	WMG_DEBUG("sta get info\n");
	int ret;
	struct netif *nif = g_wlan_netif;

	wlan_sta_ap_t *ap = cmd_malloc(sizeof(wlan_sta_ap_t));
	if (ap == NULL) {
		WMG_ERROR("no mem\n");
		return WMG_STATUS_FAIL;
	}
	ret = wlan_sta_ap_info(ap);
	if (ret == 0) {
		sta_info->id = -1;
		sta_info->freq = ap->freq;
		sta_info->rssi = ap->level;
		memcpy(sta_info->bssid, ap->bssid, (sizeof(uint8_t) * 6));
		memcpy(sta_info->ssid, ap->ssid.ssid, ap->ssid.ssid_len);

		if (NET_IS_IP4_VALID(nif) && netif_is_link_up(nif)) {
			sta_info->ip_addr[0] = (nif->ip_addr.addr & 0xff);
			sta_info->ip_addr[1] = ((nif->ip_addr.addr >> 8) & 0xff);
			sta_info->ip_addr[2] = ((nif->ip_addr.addr >> 16) & 0xff);
			sta_info->ip_addr[3] = ((nif->ip_addr.addr >> 24) & 0xff);

			sta_info->gw_addr[0] = (nif->gw.addr & 0xff);
			sta_info->gw_addr[1] = ((nif->gw.addr >> 8) & 0xff);
			sta_info->gw_addr[2] = ((nif->gw.addr >> 16) & 0xff);
			sta_info->gw_addr[3] = ((nif->gw.addr >> 24) & 0xff);

		}

		if(ap->wpa_flags & WPA_FLAGS_WPA) {
			sta_info->sec = WIFI_SEC_WPA_PSK;
		} else if ((ap->wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == WPA_FLAGS_RSN) {
			sta_info->sec = WIFI_SEC_WPA2_PSK;
		} else if ((ap->wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) {
			sta_info->sec = WIFI_SEC_WPA3_PSK;
		} else if (ap->wpa_flags & WPA_FLAGS_WEP) {
			sta_info->sec = WIFI_SEC_WEP;
		} else {
			sta_info->sec = WIFI_SEC_NONE;
		}
	}
	cmd_free(ap);
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_scan_networks(get_scan_results_para_t *sta_scan_results_para)
{
	WMG_DEBUG("sta scan\n");
	int ret, i;
	int sleep_times = SCAN_TIMEOUT;
	int scan_default_size = 50;

	wlan_sta_scan_results_t results;
	results.ap = cmd_malloc(scan_default_size * sizeof(wlan_sta_ap_t));
	if (results.ap == NULL) {
		WMG_ERROR("no mem\n");
		return WMG_STATUS_FAIL;
    }
	results.size = scan_default_size;

    /* scan hidden ssid. */
    if (sta_scan_results_para->ssid != NULL) {
		WMG_DEBUG("sta scan hidden ssid: %s\n", sta_scan_results_para->ssid);
        wlan_sta_config(sta_scan_results_para->ssid, strlen(sta_scan_results_para->ssid), "00000000", WLAN_STA_CONF_FLAG_WPA3);
	}

	/* set scan num max, default 50 */
	int size = 50;
	if ((sta_scan_results_para->arr_size != 0) && (sta_scan_results_para->arr_size > scan_default_size))
	{
		WMG_DEBUG("sta scan bss_num_max=%d\n", sta_scan_results_para->arr_size);
		scan_default_size = sta_scan_results_para->arr_size;
	}
	wlan_sta_bss_max_count((uint8_t)scan_default_size);

	/* judge need to scan once, or directly get last scan results. */
	WMG_INFO("sta scan_action = %d\n", sta_scan_results_para->scan_results->scan_action);
	if (sta_scan_results_para->scan_results->scan_action == 1){
		wlan_sta_scan_once();
		WMG_DEBUG("%s->%d, scan state: %d\n", __func__, __LINE__, scan_state);
		/* wait for scan success event, until 50us * 100 = 5s timeout. */
		while(!scan_state) {
			if(sleep_times <= 0)
				break;
			sleep_times--;
			usleep(50 * 1000);
		}
	}

	/* scan success, but no results, print info then return.*/
	wlan_sta_scan_result(&results);
	WMG_DEBUG("%s->%d, scan state: %d, we will clear it to 0!\n", __func__, __LINE__, scan_state);
	scan_state = 0;
	if (results.num == 0){
		WMG_DEBUG("sta scan success, but results is 0\n");
		cmd_free(results.ap);
		return WMG_STATUS_SUCCESS;
	}

	/* cp scan driver results to wifimg.*/
	*(sta_scan_results_para->bss_num) = results.num;
	for(i = 0; i < results.num; ++i) {
		WMG_DEBUG("driver scan one %s\n", results.ap[i].ssid.ssid);
		memcpy(sta_scan_results_para->scan_results[i].bssid, results.ap[i].bssid, (sizeof(uint8_t) * 6));
		memcpy(sta_scan_results_para->scan_results[i].ssid, results.ap[i].ssid.ssid, SSID_MAX_LEN);
		sta_scan_results_para->scan_results[i].ssid[SSID_MAX_LEN] = '\0';
		sta_scan_results_para->scan_results[i].freq = (uint32_t)(results.ap[i].freq);
		sta_scan_results_para->scan_results[i].rssi = results.ap[i].level;
		sta_scan_results_para->scan_results[i].key_mgmt = WIFI_SEC_NONE;
		if(results.ap[i].wpa_flags & WPA_FLAGS_WPA) {
			sta_scan_results_para->scan_results[i].key_mgmt = WIFI_SEC_WPA_PSK;
		}
		if ((results.ap[i].wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == WPA_FLAGS_RSN) {
			sta_scan_results_para->scan_results[i].key_mgmt |= WIFI_SEC_WPA2_PSK;
		}
		if ((results.ap[i].wpa_flags & (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) == (WPA_FLAGS_RSN | WPA_FLAGS_SAE)) {
			sta_scan_results_para->scan_results[i].key_mgmt |= WIFI_SEC_WPA3_PSK;
		}
		if (results.ap[i].wpa_flags & WPA_FLAGS_WEP) {
			sta_scan_results_para->scan_results[i].key_mgmt |= WIFI_SEC_WEP;
		}
	}
	cmd_free(results.ap);
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_sta_remove_network(char *ssid)
{
	WMG_DEBUG("sta remove network\n");
	// check /data/wpa_supplicant.conf file is exist.
	if(access("/data/wpa_supplicant.conf", R_OK))
	{
		WMG_DEBUG("wpa_supplicant.conf file is not existed\n");
		return WMG_STATUS_FAIL;
	}

	// check /data/wpa_supplicant.conf file is empty.
	FILE* file1 = fopen("/data/wpa_supplicant.conf", "r");
	int c = fgetc(file1);
	if (c == EOF) {
		WMG_DEBUG("wpa_supplicant.conf file is empty\n");
		fclose(file1);
		return WMG_STATUS_FAIL;
	}
	fclose(file1);

	/*ToDo: check ap is exist in wpa_supplicant.conf */
	int file2;
	if (ssid != NULL) {
		file2 = open("/data/wpa_supplicant.conf", O_RDWR | O_TRUNC);
		WMG_DEBUG("remove network (%s) in /data/wpa_supplicant.conf file\n", ssid);
	} else {
		file2 = open("/data/wpa_supplicant.conf", O_RDWR | O_TRUNC);
		WMG_DEBUG("remove all networks in /data/wpa_supplicant.conf file\n");
	}
	wlan_sta_disable();
	close(file2);
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t freertos_platform_extension(int cmd, void* cmd_para,int *erro_code)
{
	WMG_DEBUG("platform extension\n");
	switch (cmd) {
	case STA_CMD_GET_INFO:
		return freertos_sta_get_info((wifi_sta_info_t *)cmd_para);
	case STA_CMD_CONNECT:
		return freertos_sta_connect((wifi_sta_cn_para_t *)cmd_para);
	case STA_CMD_DISCONNECT:
			return freertos_sta_disconnect();
	case STA_CMD_LIST_NETWORKS:
			return WMG_STATUS_UNSUPPORTED;
	case STA_CMD_REMOVE_NETWORKS:
			return freertos_sta_remove_network((char *)cmd_para);
	case STA_CMD_SET_AUTO_RECONN:
			return freertos_sta_auto_connect((bool *)cmd_para);
	case STA_CMD_GET_SCAN_RESULTS:
			return freertos_sta_scan_networks((get_scan_results_para_t *)cmd_para);
	default:
			return WMG_FALSE;
	}
	return WMG_FALSE;
}

static wmg_sta_inf_object_t sta_inf_object = {
	.sta_init_flag = WMG_FALSE,
	.sta_auto_reconn = WMG_FALSE,
	.sta_event_cb = NULL,
	.sta_private_data = NULL,

	.sta_inf_init = freertos_sta_mode_init,
	.sta_inf_deinit = freertos_sta_mode_deinit,
	.sta_inf_enable = freertos_sta_mode_enable,
	.sta_inf_disable = freertos_sta_mode_disable,
	.sta_platform_extension = freertos_platform_extension,
};

wmg_sta_inf_object_t* sta_rtos_inf_object_register(void)
{
	return &sta_inf_object;
}
