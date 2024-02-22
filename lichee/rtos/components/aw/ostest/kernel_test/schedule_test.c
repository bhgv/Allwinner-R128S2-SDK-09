#include <stdint.h>
#include <stdio.h>

#include <hal_time.h>
#include <hal_thread.h>
#include <hal_interrupt.h>
#include <hal_cmd.h>

#define TEST_PRINT(fmt,...) printf("[%s] "fmt, thread_name, ##__VA_ARGS__)

#define SCHEDULE_TEST_THREAD_NUM 2
#define SCHEDULE_TEST_THREAD_SLEEP_TIME 100
#define SCHEDULE_TEST_DIS_INT_TIME 3000

typedef struct
{
	int sleep_time;
	int thread_exec_cnt;
} test_thread_info_t;

static test_thread_info_t s_test_thread_info[SCHEDULE_TEST_THREAD_NUM];

void schedule_test_thread_func(void *param)
{
	test_thread_info_t *thread_info = (test_thread_info_t *)param;
	const char *thread_name = pcTaskGetName(NULL);

	TEST_PRINT("thread '%s' runing, sleep_time: %d, exec_cnt: %d\n",
				thread_name, thread_info->sleep_time, thread_info->thread_exec_cnt);

	while (1)
	{
		thread_info->thread_exec_cnt++;
		hal_msleep(thread_info->sleep_time);
	}
	hal_thread_stop(NULL);
}

#define SCHEDULE_TEST_THREAD_PRIORITY (configMAX_PRIORITIES / 2)
static void *s_thread_handle[SCHEDULE_TEST_THREAD_NUM];

static int init_schedule_test_thread(int thread_num)
{
	int i, j;
	char name_buf[configMAX_TASK_NAME_LEN];
	for (i = 0; i < thread_num; i++)
	{
		snprintf(name_buf, configMAX_TASK_NAME_LEN, "schedule_test%d", i);
		s_test_thread_info[i].sleep_time = (i + 1) * SCHEDULE_TEST_THREAD_SLEEP_TIME;
		s_thread_handle[i] = hal_thread_create(schedule_test_thread_func, &s_test_thread_info[i], name_buf, 1024, SCHEDULE_TEST_THREAD_PRIORITY);
		if (!s_thread_handle[i])
			goto exit_fallback;

		hal_thread_start(s_thread_handle[i]);
	}

	return 0;

exit_fallback:
	for (j = 0; j < i; j++)
	{
		hal_thread_stop(s_thread_handle[j]);
	}
	return -(i + 1);
}

static void deinit_schedule_test_thread(int thread_num)
{
	int i;
	for (i = 0; i < thread_num; i++)
	{
		hal_thread_stop(s_thread_handle[i]);
		s_test_thread_info[i].thread_exec_cnt = 0;
	}
}

static void read_thread_exec_cnt(int thread_num, int *exec_cnt_array)
{
	int i;
	for (i = 0; i < thread_num; i++)
	{
		exec_cnt_array[i] = s_test_thread_info[i].thread_exec_cnt;
	}
}

static void dump_thread_exec_cnt(int thread_num, int *exec_cnt_array)
{
	int i;
	const char *thread_name = pcTaskGetName(NULL);

	for (i = 0; i < thread_num; i++)
	{
		TEST_PRINT("thread%d exec cnt: %d\n", i, exec_cnt_array[i]);
	}
}

#define SCHEDULE_TEST_CASE_NAME "can not schedule when global interrupt is disabled"
int cmd_schedule_test(int argc, const char **argv)
{
	int ret = 0, i, is_test_pass = 1;
	unsigned long flag;

	int thread_exec_cnt_before[SCHEDULE_TEST_THREAD_NUM];
	int thread_exec_cnt_after[SCHEDULE_TEST_THREAD_NUM];

	int thread_num = SCHEDULE_TEST_THREAD_NUM;
	const char *thread_name = pcTaskGetName(NULL);
	int delay_time = SCHEDULE_TEST_DIS_INT_TIME;

	TEST_PRINT("schedule test case: '%s'\n", SCHEDULE_TEST_CASE_NAME);

	ret = init_schedule_test_thread(thread_num);
	if (ret)
	{
		TEST_PRINT("init_schedule_test_thread failed ,ret: %d\n", ret);
		return -1;
	}

	//let the schedule test threads to run at least twice
	hal_msleep(SCHEDULE_TEST_THREAD_SLEEP_TIME * 2);

	TEST_PRINT("global interrupt disabled: %ld\n", hal_interrupt_is_disable());
	TEST_PRINT("disable global interrupt begin\n");
	flag = hal_interrupt_disable_irqsave();
	TEST_PRINT("disable global interrupt end\n");
	TEST_PRINT("global interrupt disabled: %ld\n", hal_interrupt_is_disable());

	read_thread_exec_cnt(thread_num, thread_exec_cnt_before);
	dump_thread_exec_cnt(thread_num, thread_exec_cnt_before);

	TEST_PRINT("busy wait %dms begin\n", delay_time);

	hal_msleep(delay_time);

	TEST_PRINT("busy wait %dms end\n", delay_time);

	read_thread_exec_cnt(thread_num, thread_exec_cnt_after);
	dump_thread_exec_cnt(thread_num, thread_exec_cnt_after);

	TEST_PRINT("global interrupt disabled: %ld\n", hal_interrupt_is_disable());

	TEST_PRINT("enable global interrupt begin\n");
	hal_interrupt_enable_irqrestore(flag);
	TEST_PRINT("enable global interrupt end\n");

	TEST_PRINT("global interrupt disabled: %ld\n", hal_interrupt_is_disable());

	for (i = 0; i < thread_num; i++)
	{
		if (thread_exec_cnt_before[i] != thread_exec_cnt_after[i])
		{
			is_test_pass = 0;
			TEST_PRINT("schedule test case: '%s' failed!\n", SCHEDULE_TEST_CASE_NAME);
			TEST_PRINT("the execute count of schedule test thread%d is not same, before: %d, after: %d\n",
						i, thread_exec_cnt_before[i], thread_exec_cnt_after[i]);
			break;
		}
	}

	hal_msleep(SCHEDULE_TEST_THREAD_SLEEP_TIME * 4);
	read_thread_exec_cnt(thread_num, thread_exec_cnt_after);
	dump_thread_exec_cnt(thread_num, thread_exec_cnt_after);

	deinit_schedule_test_thread(thread_num);
	if (is_test_pass)
		TEST_PRINT("schedule test case: '%s' success\n", SCHEDULE_TEST_CASE_NAME);
	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_schedule_test, schedule_test, schedule test);
