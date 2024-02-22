#include <tinatest.h>
#include "tt-wifitest.h"

//wifi on test
//cmd:tt wonwifi
int tt_wifi_on_test(int argc, char **argv)
{
	int ret;

	printf("\n========TINATEST FOR WIFI ON========\n");
	ret = wifi_on(WIFI_STATION);
	if(!ret)
		printf("\n=======TINATEST FOR WIFI ON OK=======\n");
	else
		printf("\n=======TINATEST FOR WIFI ON FAIL=======\n");

	return ret;
}
testcase_init(tt_wifi_on_test, wifi_on, wifi on for tinatest);
