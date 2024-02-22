#include <tinatest.h>
#include "tt-wifitest.h"

//wifi connect test
//cmd:tt wcwifi ssid password

int tt_wifi_conn_test(int argc, char **argv)
{
	int ret;
	wifi_sta_cn_para_t cn_para;
	cn_para.ssid = "AWTest-2.4G";
	cn_para.password = "1qaz@WSX";
	cn_para.sec = WIFI_SEC_WPA2_PSK;

	wifimanager_init();
	wifi_on(WIFI_STATION);
	printf("\n========TINATEST FOR WIFI CONNECT========\n");

	ret = wifi_sta_connect(&cn_para);
	if(!ret)
		printf("\n=======TINATEST FOR WIFI CONNECT OK=======\n");
	else
		printf("\n=======TINATEST FOR WIFI CONNECT FAIL=======\n");

	return ret;
}

testcase_init(tt_wifi_conn_test, wifi_conn, wifi connect for tinatest);
