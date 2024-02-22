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
#include "Dir.h"
#include "File.h"
#include "DiskCommon.h"
#include "MtpCommon.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

static void getFileInfo(struct Dir *dir)
{
	struct stat sb;
	int fd = 0;

	if (stat(dir->path, &sb) == -1) {
		ELOG("get dir %s failed\n", dir->path);
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
	dir->dirSize = sb.st_size;
	dir->dirAccessTime = sb.st_atime;
	dir->dirModifyTime = sb.st_mtime;

	DLOG(" <%s> size: %"PRIu64"M %"PRIu64"K %"PRIu64"bytes", dir->dirName, dir->dirSize>>20, dir->dirSize>>10, dir->dirSize);
	DLOG(" <%s> accecc time: %s", dir->dirName, ctime(&dir->dirAccessTime));
	DLOG(" <%s> modify time: %s", dir->dirName, ctime(&dir->dirModifyTime));
	return ;
}

char *getFileNameFromPath(const char *path, char *name, size_t *size)
{
	char *ptr = NULL;
	ptr = strrchr(path, '/');
	if (ptr != NULL) {
		size_t len = strlen(++ptr);
		if (len < 1 || len > *size)
			goto err;
		*size = len + 1;
		strcpy(name, ptr);
		return name;
	}
err:
	printf("path unknown: %s\n", path);
	return NULL;
}

void updateDirInfo(struct Dir *dir)
{
	getFileInfo(dir);
}

void deleteDirInfo(struct Dir *dir)
{
	list_del(&dir->list);
}

static bool renameTo(const char *newPath, struct Dir *dir)
{
	int ret;
	char buf[NAME_LENGTH_MAX];
	size_t len = sizeof(buf);

	if (access(newPath, F_OK) == 0)
		return false;
	ret = rename(dir->path, newPath);
	if (ret != 0) {
		printf("rename oldpath:%s to newpath:%s failed, errno:%d",
				dir->path, newPath, errno);
		return false;
	}
	if (dir->path)
		free_wrapper(dir->path);
	dir->path = strdup_wrapper(newPath);
	if (dir->dirName)
		free_wrapper(dir->dirName);
	if (getFileNameFromPath(newPath, buf, &len) != NULL)
		dir->dirName = strdup_wrapper(buf);
	getFileInfo(dir);
	return true;
}

struct Dir *DirInit(const char *path, struct Dir *parentDir)
{
	struct Dir *dir;
	size_t len;
	char buf[NAME_LENGTH_MAX];

	if (!path)
		return NULL;
	DLOG(" path:%s", path);
	dir = (struct Dir *)calloc_wrapper(1, sizeof(struct Dir));
	memset(dir, 0, sizeof(struct Dir));

	dir->path = strdup_wrapper(path);

	len = sizeof(buf);
	if (getFileNameFromPath(path, buf, &len) != NULL)
		dir->dirName = strdup_wrapper(buf);

	getFileInfo(dir);
	INIT_LIST_HEAD(&dir->list);
	dir->parentDir = parentDir;

	dir->renameTo = renameTo;

	DLOG("init !! dir:%p", dir);
	return dir;
}

void DirRelease(struct Dir *dir)
{
	DLOG("exit !! dir:%p", dir);
	if (!dir)
		return ;
	if (dir->path) {
		free_wrapper(dir->path);
		dir->path = NULL;
	}
	if (dir->dirName) {
		free_wrapper(dir->dirName);
		dir->dirName = NULL;
	}
	free_wrapper(dir);
}
