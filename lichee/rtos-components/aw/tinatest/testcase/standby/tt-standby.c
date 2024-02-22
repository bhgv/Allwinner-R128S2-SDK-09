#include <stdint.h>
#include <stdio.h>
#include <tinatest.h>

#include <hal_time.h>
#include <pm_base.h>
#include <pm_testlevel.h>
#include <pm_state.h>
#include <pm_rpcfunc.h>

#define TEST_TIMES CONFIG_STANDBY_LOOP_TIMES
#define INIT_IRQ    (-32)

extern int pm_trigger_suspend(suspend_mode_t mode);
extern int pm_set_wakeirq(const int irq);
extern BaseType_t FreeRTOS_CLIProcessCommand( const char * const pcCommandInput, char * pcWriteBuffer, size_t xWriteBufferLen);
static int test_cnt = 0;

int test_irq = INIT_IRQ;
static volatile int standby_finish_flags = 0;

static int pm_test_tools_notify_cb(suspend_mode_t mode, pm_event_t event, void *arg)
{
	switch (event) {
	case PM_EVENT_PERPARED:
		break;
	case PM_EVENT_FINISHED:
		test_cnt ++;
		break;
	case PM_EVENT_SYS_FINISHED:
		printf("========PM_EVENT_SYS_FINISHED\n");
		standby_finish_flags = 1;
		break;
	default:
		break;
	}

	return 0;
}


static pm_notify_t pm_test_tools = {
	 .name = "pm_test_tools",
	 .pm_notify_cb = pm_test_tools_notify_cb,
	.arg = NULL,
};

int tt_standby(int argc, char **argv)
{
	static unsigned int time_to_suspend_ms = 5000;
	static unsigned int time_to_wakeup_ms = 5000;
	int times = strtol(TEST_TIMES, NULL, 0);
	test_cnt = strtol(TEST_TIMES, NULL, 0) - times;


	char *outputbuffer;
	int ret = 0, ret_id = 0;
	//int irq;

	printf("=============TEST FOR STANDBY===========\n");
	printf("standby stress\ntimes: %d\ntime_to_suspend_ms: %d\ntime_to_wakeup_ms: %d\n",
			times, time_to_suspend_ms, time_to_wakeup_ms);

	FreeRTOS_CLIProcessCommand("rpccli arm time_to_wakeup_ms 5000", outputbuffer, 2048);

	ret_id = pm_notify_register(&pm_test_tools);
	if (ret_id == -1) {
		printf("Failed to execute command\n");
	}

	while(times) {
		times --;
		while(pm_state_get() != PM_STATUS_RUNNING);
		printf("===== standby_stress times: %d run atfter %dms... =====\n", 
				strtol(CONFIG_STANDBY_LOOP_TIMES, NULL, 0) - times, time_to_suspend_ms);
		hal_msleep(time_to_suspend_ms);

		ret = pm_trigger_suspend(PM_MODE_STANDBY);
		if (ret) {
			printf("===== %s: trigger suspend failed, return %d =====\n", __func__, ret);
		} else {
			if (pm_state_get() == PM_STATUS_RUNNING) {
				/* if status hasn't change, wait this round complete, or may this round completed already. */
				while(pm_state_get() != PM_STATUS_RUNNING);
			}
		}
		while(!standby_finish_flags);

		while(pm_state_get() != PM_STATUS_RUNNING);
		//pm_hal_get_wakeup_irq(&irq);
		//printf("===== pm_wakeup_irq: %d =====\n", irq);

		printf("======LOOP TESTED OK TIMES:%d======\n", test_cnt);
		printf("======LOOP LEFT TIMES:%d======\n", times);
		standby_finish_flags = 0;
	}

	ret = pm_notify_unregister(ret_id);
	if (ret < 0) {
		printf("Failed to unregister\n");
	}

	printf("============TEST FOR STANDBY OK!============\n");

	return ret;
}
testcase_init(tt_standby, standby, standby for tinatest);
