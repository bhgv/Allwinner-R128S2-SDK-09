#include <stdio.h>
#include <stdint.h>
#include <tinatest.h>
#include <pthread.h>
#include <unistd.h>

extern int cmd_fork(int argc, char ** argv);
extern int cmd_as_test(int argc, char ** argv);

static void do_playback(void)
{
	int ret;
	char *playback_argv[] = {
		"fork", "as_test",
		"-s", "2",
		"-d", "3",
	};
	int playback_argc = sizeof(playback_argv) / sizeof(char *);

	cmd_fork(playback_argc, playback_argv);
}

static void do_capture(void)
{
	int ret;
	char *capture_argv[] = {
		"as_test",
		"-t", /* capture then play */
		"-s", "1",
		"-d", "4",
	};
	int capture_argc = sizeof(capture_argv) / sizeof(char *);

	cmd_as_test(capture_argc, capture_argv);
}

static int tt_playback_and_capture(int argc, char *argv[])
{
	int ret;

	ttips("Starting playing with speaker\n");
	do_playback();
	usleep(500*1000);
	ttips("Starting recording\n");
	do_capture();
	ret = ttrue("Finish playing. Can you hear the sound from speaker?\n");
	if (ret < 0) {
		printf("enter no\n");
		return -1;
	}

	return 0;
}
testcase_init(tt_playback_and_capture, audiopc, playback and then caputre);
