/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define DEBUG
#include "File.h"
#include "DiskCommon.h"
#include "MtpCommon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>

static void getFileInfo(struct File *file)
{
	struct stat sb;

	if (stat(file->path, &sb) == -1) {
		ELOG("get file %s failed\n", file->path);
		return;
	}
#if 0
#ifdef DEBUG
	switch (sb.st_mode & S_IFMT) {
		case S_IFBLK:  printf("block device\n");            break;
		case S_IFCHR:  printf("character device\n");        break;
		case S_IFDIR:  printf("directory\n");               break;
		case S_IFIFO:  printf("FIFO/pipe\n");               break;
		case S_IFLNK:  printf("symlink\n");                 break;
		case S_IFREG:  printf("regular file\n");            break;
		case S_IFSOCK: printf("socket\n");                  break;
		default:       printf("unknown?\n");                break;
	}
	printf("I-node number:            %ld\n", (long) sb.st_ino);
	printf("Mode:                     %lo (octal)\n",
		(unsigned long) sb.st_mode);
	printf("Link count:               %ld\n", (long) sb.st_nlink);
	printf("Ownership:                UID=%ld   GID=%ld\n",
		(long) sb.st_uid, (long) sb.st_gid);
	printf("Preferred I/O block size: %ld bytes\n",
		(long) sb.st_blksize);
	printf("File size:                %lld bytes\n",
		(long long) sb.st_size);
	printf("Blocks allocated:         %lld\n",
		(long long) sb.st_blocks);
	printf("Last status change:       %s", ctime(&sb.st_ctime));
	printf("Last file access:         %s", ctime(&sb.st_atime));
	printf("Last file modification:   %s", ctime(&sb.st_mtime));
#endif
#endif
	file->fileType = sb.st_mode & S_IFMT;
	file->fileSize = sb.st_size;
	file->fileAccessTime = sb.st_atime;
	file->fileModifyTime = sb.st_mtime;

	DLOG(" <%s> size: %"PRIu64"M %"PRIu64"K %"PRIu64"bytes", file->fileName,
		(file->fileSize>>20),
		(file->fileSize>>10) & (1<<10 - 1),
		file->fileSize & (1<<10 - 1));
	DLOG(" <%s> accecc time: %s", file->fileName, ctime(&file->fileAccessTime));
	DLOG(" <%s> modify time: %s", file->fileName, ctime(&file->fileModifyTime));
	return ;
}

void updateFileInfo(struct File *file)
{
	getFileInfo(file);
}

void deleteFileInfo(struct File *file)
{
	list_del(&file->list);
}

extern char *getFileNameFromPath(const char *path, char *name, size_t *size);

static bool renameTo(const char *newPath, struct File *file)
{
	int ret;
	char buf[NAME_LENGTH_MAX];
	size_t len = sizeof(buf);

	if (access(newPath, F_OK) == 0)
		return false;
	ret = rename(file->path, newPath);
	if (ret != 0) {
		printf("rename oldpath:%s to newpath:%s failed, errno:%d",
				file->path, newPath, errno);
		return false;
	}
	if (file->path)
		free_wrapper(file->path);
	file->path = strdup_wrapper(newPath);
	if (file->fileName)
		free_wrapper(file->fileName);
	if (getFileNameFromPath(newPath, buf, &len) != NULL)
		file->fileName = strdup_wrapper(buf);
	getFileInfo(file);

	return true;
}

struct File *FileInit(const char *path, struct Dir *parentDir)
{
	struct File *file = NULL;
	size_t len;
	char buf[NAME_LENGTH_MAX];

	if (!path)
		return NULL;

	DLOG(" path:%s", path);
	file = (struct File *)calloc_wrapper(1, sizeof(struct File));

	file->path = strdup_wrapper(path);

	len = sizeof(buf);
	if (getFileNameFromPath(path, buf, &len) != NULL) {
		file->fileName = strdup_wrapper(buf);
		DLOG("filename: %p, %s", file->fileName, file->fileName);
	}

	getFileInfo(file);

	INIT_LIST_HEAD(&file->list);

	file->parentDir = parentDir;

	file->renameTo = renameTo;

	DLOG("create File:%p", file);
	return file;
}

void FileRelease(struct File *file)
{
	DLOG("file: %p", file);
	if (!file)
		return ;
	DLOG("path:%p, %s", file->path, file->path);
	if (file->path) {
		free_wrapper(file->path);
		file->path = NULL;
	}
	DLOG("name:%p, %s", file->fileName, file->fileName);
	if (file->fileName) {
		free_wrapper(file->fileName);
		file->fileName = NULL;
	}
	free_wrapper(file);

	return ;
}
