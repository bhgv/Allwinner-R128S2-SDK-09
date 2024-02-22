#include <tinatest.h>
#include "tt-wifitest.h"

//wifi rmove test
//cmd:tt wrmwifi

int tt_wifi_disconn_test(int argc, char **argv)
{
	int ret;

	printf("\n========TINATEST FOR WIFI DISCONNECT========\n");
	wifimanager_init();
	wifi_on(WIFI_STATION);
	ret = wifi_sta_disconnect();
	if(!ret)
		printf("\n=======TINATEST FOR WIFI DISCONNECT OK=======\n");
	else
		printf("\n=======TINATEST FOR WIFI DISCONNECT FAIL=======\n");

	return ret;
}

testcase_init(tt_wifi_disconn_test, wifi_disconn, wifi disconnect for tinatest);
