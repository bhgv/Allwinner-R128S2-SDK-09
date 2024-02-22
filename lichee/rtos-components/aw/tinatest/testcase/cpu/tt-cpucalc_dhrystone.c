#include <stdint.h>
#include <stdio.h>
#include <tinatest.h>

extern void dhrystonre_main(int Number_Of_Runs);

int tt_cpu_calc_dhrystonre(int argc, char **argv)
{
	int number = 1000000;
	int times = strtol(CONFIG_LOOP_TIMES, NULL, 0);

	printf("======TEST FOR CPU CALC DHRYSTONRE=======\n");

	while(times) {
		times --;
		dhrystonre_main(number);
		printf("======LOOP TESTED OK TIMES:%d=======\n", strtol(CONFIG_LOOP_TIMES, NULL, 0) - times);
		printf("======LOOP LEFT TIMES:%d=======\n", times);
	}

	printf("======TEST FOR CPU CALC DHRYSTONRE OK!=======\n");

	return 0;
}
testcase_init(tt_cpu_calc_dhrystonre, cpucalcdhrystonre, cpucalcdhrystonretest for tinatest);
