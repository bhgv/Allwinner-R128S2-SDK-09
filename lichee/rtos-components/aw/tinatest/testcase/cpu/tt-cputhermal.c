#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <tinatest.h>

#include "thermal_external.h"

int tt_thermal(int argc, char **argv)
{
	int times = strtol(CONFIG_THERMAL_LOOP_TIMES, NULL, 0);

	int ret = 0;
	int num;
	int *id_buf;
	int *temp_buf;

	printf("=============TEST FOR THERMAL===========\n");

	while(times) {
		times --;

		num = thermal_external_get_zone_number();

		temp_buf = malloc(num * sizeof(int));
		if (temp_buf == NULL) {
			printf("temp_buf malloc fail\n");
			return -1;
		}
		id_buf = malloc(num * sizeof(int));
		if (id_buf == NULL) {
			printf("id_buf malloc fail\n");
			free(temp_buf);
			return -1;
		}

		ret = thermal_external_get_temp(id_buf, temp_buf, num);

		if(ret) {
			printf("get temp fail!\n");
			return ret;
		}

		printf("get thermal list:\n");
		for(int i=0; i < num; i++) {
			printf("thermal zone%d: %d\n", id_buf[i], temp_buf[i]);
		}

		printf("======LOOP TESTED OK TIMES:%d======\n", strtol(CONFIG_THERMAL_LOOP_TIMES, NULL, 0) - times);
		printf("======LOOP LEFT TIMES:%d======\n", times);

		//hal_mdelay(500);
	}

	printf("============TEST FOR THERMAL OK!============\n");

	free(temp_buf);
	free(id_buf);

	return ret;
fail:
	printf("============TEST FOR THERMAL FAIL!============\n");
	return ret;
}
testcase_init(tt_thermal, thermal, thermal for tinatest);
