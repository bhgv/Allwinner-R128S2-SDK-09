#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <tinatest.h>
#include <aw-alsa-lib/pcm.h>

#include <AudioSystem.h>

#include "wav_parser.h"

#define CAPTURE_TIME_SECS (3)
#define CAP_LOOP_COUNTS		CONFIG_CAPTURE_LOOP_COUNTS
#define CAP_LOOP_SAVE_PATH	CONFIG_CAPTURE_LOOP_SAVE_PATH
#define CAP_LOOP_SAMPLE_RATE	16000
#define CAP_LOOP_SAMPLE_BIT		16
#define CAP_LOOP_SAMPLE_FORAMT	SND_PCM_FORMAT_S16_LE
#define CAP_LOOP_CHANNEL		2

static int audio_cap_loop(int argc, char **argv)
{
	int ret;
	int loop_count = 10;
	char string[128];
	tAudioRecord *ar;
	tAudioTrack *at;
	char *ar_name = "default";
	char *at_name = "default";
	int total, read_size, size, read = 0;
	int frame_bytes = CAP_LOOP_SAMPLE_BIT / 8  * CAP_LOOP_CHANNEL;
	int frames_bytes_loop = frame_bytes * CAP_LOOP_SAMPLE_RATE / 100; /* 10ms */
	void *buf = NULL;
	wav_header_t header;
	struct stat statbuf;
	int fd = 0;
	int cap_then_play = 0;
	char save_path[64] = {0};

	loop_count = strtoul(CAP_LOOP_COUNTS, NULL, 0);
	strncpy(save_path, CAP_LOOP_SAVE_PATH, sizeof(save_path));
	printf("test loop_count:%d, save_path %s\n", loop_count, save_path);

	if (loop_count < 0) {
		printf("loop_count error :%d\n", loop_count);
		return -1;
	}

	snprintf(string, sizeof(string),
		"capture stream loop test start(count %d save_path %s).\n",
		loop_count, save_path);
	ttips(string);

	if (strnlen(save_path, sizeof(save_path))) {
		if (!stat(save_path, &statbuf)) {
			if (S_ISREG(statbuf.st_mode))
				remove(save_path);
		}
		fd = open(save_path, O_RDWR | O_CREAT, 0644);
		if (fd < 0) {
			printf("create wav file failed\n");
			goto err;
		}
		create_wav(&header, CAP_LOOP_SAMPLE_FORAMT, CAP_LOOP_SAMPLE_RATE, CAP_LOOP_CHANNEL);
		write(fd, &header, sizeof(header));
	} else {
		cap_then_play =1;
	}

	total = CAPTURE_TIME_SECS * CAP_LOOP_SAMPLE_RATE * frame_bytes;
	buf = malloc(total);
	if (!buf) {
		printf("no memory\n");
		goto err;
	}

	ar = AudioRecordCreate(ar_name);
	if (!ar) {
		printf("ar create failed\n");
		goto err;
	}

	AudioRecordSetup(ar, CAP_LOOP_SAMPLE_RATE, CAP_LOOP_CHANNEL, CAP_LOOP_SAMPLE_BIT);

	while (loop_count--) {
		AudioRecordStart(ar);

		total =CAPTURE_TIME_SECS * CAP_LOOP_SAMPLE_RATE * frame_bytes;
		read = 0;

		while (total > 0) {
			if (total > frames_bytes_loop)
				size = frames_bytes_loop;
			else
				size = total;
			read_size = AudioRecordRead(ar, buf + read, size);
			if (read_size != frames_bytes_loop) {
				printf("read_size(%d) != frames_bytes_loop(%d)\n", read_size, frames_bytes_loop);
				break;
			}
			total -= read_size;
			read += read_size;
			/*printf("[%s] line:%d residue:%d read=%u\n", __func__, __LINE__, total, read);*/
		}
		if (read_size < 0)
			break;

		AudioRecordStop(ar);
		usleep(500*1000);
		printf("remain count %d\n", loop_count);
	}

	if (fd > 0) {
		printf("please wait...writing data(%d bytes) into %s\n", read, save_path);
		ret = write(fd, buf, read);
		if (ret != read) {
			printf("write audiobuf to wav file failed, return %d\n", ret);
			goto err;
		}
		printf("write finish...\n");
	} else if (cap_then_play) {

		printf("It will play recording...\n");
		at = AudioTrackCreate(at_name);
		if (!at) {
			printf("at create failed\n");
			goto err;
		}
		AudioTrackSetup(at, CAP_LOOP_SAMPLE_RATE, CAP_LOOP_CHANNEL, CAP_LOOP_SAMPLE_BIT);

		AudioTrackWrite(at, buf, read);

		AudioTrackStop(at);

		AudioTrackDestroy(at);
	}

	if (ar) {
		AudioRecordDestroy(ar);
		ar = NULL;
	}

	if (fd > 0 && read > 0) {
		resize_wav(&header, read);
		lseek(fd, 0, SEEK_SET);
		write(fd, &header, sizeof(header));
	}

	if (fd > 0)
		close(fd);

	if (buf) {
		free(buf);
		buf = NULL;
	}

	ret = ttrue("Finish capture loop. Can you hear the sound?\n");
	if (ret < 0) {
		printf("enter no\n");
		return -1;
	}
	return 0;

err:
	if (ar) {
		AudioRecordDestroy(ar);
		ar = NULL;
	}

	if (fd > 0)
		close(fd);

	if (buf) {
		free(buf);
		buf = NULL;
	}
	return ret;
}
testcase_init(audio_cap_loop, captureloop, mic capture loop for tinatest);
