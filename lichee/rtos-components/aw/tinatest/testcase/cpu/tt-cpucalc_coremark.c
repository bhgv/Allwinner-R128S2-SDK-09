#include <stdint.h>
#include <stdio.h>
#include <tinatest.h>

extern int coremark_main(void);

int tt_cpu_calc_coremark(int argc, char **argv)
{
	int ret = 0;
	int times = strtol(CONFIG_LOOP_TIMES, NULL, 0);

	printf("======TEST FOR CPU CALC COREMARK======\n");

	while(times) {
		times --;
		ret = coremark_main();
		if(ret == 0) {
			printf("======LOOP TESTED OK TIMES:%d======\n", strtol(CONFIG_LOOP_TIMES, NULL, 0) - times);
			printf("======LOOP LEFT TIMES:%d======\n", times);
		} else {
			printf("======TEST FOR CPU CALC COREMARK FAIL!======\n");
			return -1;
		}
	}

	printf("======TEST FOR CPU CALC COREMARK OK!======\n");

	return ret;
}
testcase_init(tt_cpu_calc_coremark, cpucoremark, cpucalccoremarktest for tinatest);
