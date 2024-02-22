#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <console.h>
#include <hal_mem.h>

typedef struct {
	uint32_t action;	/* 0:add, 1:remove, 2:update */
	uint32_t type;		/* 0:file, 2:dir */
	uint32_t srcPathLen;	/* src path buffer length */
	uint32_t destPathLen;	/* dest path buffer length */
	char *path;		/* object path */
}mtp_command_t;
#define MTP_FIFO_NAME	"/tmp/.mtp_fifo"

enum {
	MTP_TOOLS_FUNCTION_ADD = 0,
	MTP_TOOLS_FUNCTION_REMOVE,
	MTP_TOOLS_FUNCTION_UPDATE,
	MTP_TOOLS_FUNCTION_CUT,
	MTP_TOOLS_FUNCTION_COPY,
	MTP_TOOLS_FUNCTION_CONNECT = 100,
};

static int mtp_tools_send_command(int fd, uint32_t action, uint32_t type, const char *spath, const char *dpath)
{
	int ret;
	mtp_command_t *command;
	size_t spathLen = 0, dpathLen = 0, pathLen = 0;
	size_t command_size = 0;

	if (spath != NULL) {
		spathLen = strlen(spath) + 1;
		dpathLen = (dpath != NULL) ? (strlen(dpath) + 1) : 0;
		pathLen = spathLen + dpathLen;
	} else if (action != MTP_TOOLS_FUNCTION_CONNECT) {
		printf("spath is NULL!\n");
		return -1;
	}

	command_size = sizeof(mtp_command_t) + pathLen;

#if 0
	printf("action:%u, type:%u\n", action, type);
	if (spathLen > 1)
		printf("spath:%s\n", spath);
	if (dpathLen > 1)
		printf("dpath:%s\n", dpath);
#endif

	command = calloc(1, command_size);
	command->action = action;
	command->type = type;
	command->srcPathLen =  spathLen;
	command->destPathLen =  dpathLen;
	if (spathLen > 1) {
		command->path = (char *)&command[1];
		strcpy(command->path, spath);
	} else if (action != MTP_TOOLS_FUNCTION_CONNECT) {
		printf("spath error: %s\n", spath);
		hal_free(command);
		return -1;
	}
	if (dpathLen > 1) {
		strcpy(command->path + spathLen, dpath);
	}

	ret = write(fd, command, command_size);
	if (ret != command_size) {
		printf("write failed, command size:%zu, but only write %d\n",
			command_size, ret);
		hal_free(command);
		return -1;
	}
	hal_free(command);
	return 0;
}


static void usage()
{
	printf("MtpTools usage:\n");
	printf("MtpTools [function] [path]\n");
	printf(" -f, --function      function opion, contains add,remove,update,cut,copy\n");
	printf(" -t, --type          object type, FILE or DIR\n");
	printf(" -s, --spath         object src path\n");
	printf(" -d, --dpath         object dest path, specify dest path of object which will be cut or copy\n");
	printf(" -h, --help          show help\n");
	printf("\n");
	printf("example:\n");
	printf("MtpTools -f add -t FILE -s /mnt/UDISK/test\n");
	printf("\n");
	exit(-1);
}

static int mtp_update(int argc, char *argv[])
{
	int fd;
	enum {
		MTP_TOOLS_TYPE_FILE = 0,
		MTP_TOOLS_TYPE_DIR,
	} type = MTP_TOOLS_TYPE_FILE;
	int func = MTP_TOOLS_FUNCTION_ADD;
	char *spath = NULL;
	char *dpath = NULL;

	while (1) {
		/*
		const struct option long_options[] = {
			{"function", required_argument, NULL, 'f'},
			{"type", required_argument, NULL, 't'},
			{"spath", required_argument, NULL, 's'},
			{"dpath", required_argument, NULL, 'd'},
			{"help", no_argument, NULL, 'h'},
		};
		int option_index = 0;
		*/
		int c = 0;

		/* c = getopt_long(argc, argv, "f:t:s:d:h", long_options, &option_index); */
		c = getopt(argc, (char *const *)argv, "f:t:s:d:h");
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage();
			break;
		case 'f':
			if (!strcmp(optarg, "add"))
				func = MTP_TOOLS_FUNCTION_ADD;
			else if (!strcmp(optarg, "remove"))
				func = MTP_TOOLS_FUNCTION_REMOVE;
			else if (!strcmp(optarg, "update"))
				func = MTP_TOOLS_FUNCTION_UPDATE;
			else if (!strcmp(optarg, "cut"))
				func = MTP_TOOLS_FUNCTION_CUT;
			else if (!strcmp(optarg, "copy"))
				func = MTP_TOOLS_FUNCTION_COPY;
			else if (!strcmp(optarg, "connect"))
				func = MTP_TOOLS_FUNCTION_CONNECT;
			else {
				printf("invalid function:[%s]\n", optarg);
				usage();
			}
			break;
		case 't':
			if (!strcmp(optarg, "FILE"))
				type = MTP_TOOLS_TYPE_FILE;
			else if (!strcmp(optarg, "DIR"))
				type = MTP_TOOLS_TYPE_DIR;
			else {
				printf("invalid type:[%s]\n", optarg);
				usage();
			}
			break;
		case 's':
			if (!optarg)
				usage();
			spath = strdup(optarg);
			break;
		case 'd':
			if (!optarg)
				usage();
			dpath = strdup(optarg);
			break;
		default:
			usage();
			break;
		}
	}

	if (optind < 2)
		usage();
	if (func != MTP_TOOLS_FUNCTION_CONNECT && !spath) {
		printf("need spath!\n");
		usage();
	}

	fd = open(MTP_FIFO_NAME, O_WRONLY | O_NONBLOCK, 0);
	if (fd < 0) {
		printf("open %s failed, %s\n", MTP_FIFO_NAME, strerror(errno));
		return -1;
	}

	mtp_tools_send_command(fd, func, type, spath, dpath);

	close(fd);
	if (spath)
		hal_free(spath);
	if (dpath)
		hal_free(dpath);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(mtp_update, mtp_update, mtp update tools)
