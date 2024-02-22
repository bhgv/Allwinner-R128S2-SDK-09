#include <tinatest.h>
#include "tt-wifitest.h"

//wifi off test
//cmd:tt woffwifi

int tt_wifi_off_test(int argc, char **argv)
{
	int ret = 0;

	printf("\n========TINATEST FOR WIFI OFF========\n");
	wifi_off();
	printf("\n========TINATEST FOR WIFI OFF OK========\n");

	return ret;
}

testcase_init(tt_wifi_off_test, wifi_off, wifi off for tinatest);
