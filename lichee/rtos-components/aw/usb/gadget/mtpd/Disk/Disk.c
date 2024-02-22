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
#include "Disk.h"
#include "DiskCommon.h"
#include "MtpCommon.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <stdlib.h>
#include <dirent.h>

static void addSubDir(struct Dir *dir, struct Disk *disk)
{
	list_add(&dir->list, &disk->dirList);
	if (dir->parentDir)
		dir->parentDir->subDirCount++;
}

static void addSubFile(struct File *file, struct Disk *disk)
{
	list_add(&file->list, &disk->fileList);
	if (file->parentDir)
		file->parentDir->subFileCount++;
}

static int filter(const struct dirent *dir_ent)
{
	if (strlen(dir_ent->d_name) == 1 && '.' == *dir_ent->d_name)
		return 0;
	if (strlen(dir_ent->d_name) == 2 && !strcmp(dir_ent->d_name, ".."))
		return 0;
	return 1;
}

static int getDirTreeInfo(struct Dir *dir, struct Disk *disk)
{
	struct dirent **namelist = NULL;
	int num, i;
	char pathBuf[PATH_MAX];

	mtp_debug("get dir: %s info\n", dir->path);
	num = scandir_wrapper(dir->path, &namelist, filter, NULL);
	DLOG("num = %d", num);
	for (i = 0; i < num; i++) {
		mtp_debug("dir[%d] name:%s type: 0x%x\n", i, namelist[i]->d_name, namelist[i]->d_type);
	}
	for (i = 0; i < num; i++) {
		if (strlen(namelist[i]->d_name) +
			strlen(dir->path) + 1 > PATH_MAX)	{
			mtp_err("path too long : %s %s\n", namelist[i]->d_name, dir->path);
			exit(-1);
		}
		mtp_debug("dir[%d] name:%s type: 0x%x\n", i, namelist[i]->d_name, namelist[i]->d_type);
		if (namelist[i]->d_type & DT_DIR) {
			struct Dir *tmpDir = NULL;

			snprintf(pathBuf, sizeof(pathBuf), "%s/%s", dir->path, namelist[i]->d_name);
			DLOG("-------------------------------------------------------");
			tmpDir = DirInit(pathBuf, dir);
			addSubDir(tmpDir, disk);
			getDirTreeInfo(tmpDir, disk);
		} else if (namelist[i]->d_type & DT_REG) {
			struct File *tmpFile = NULL;

			snprintf(pathBuf, sizeof(pathBuf), "%s/%s", dir->path, namelist[i]->d_name);
			tmpFile = FileInit(pathBuf, dir);
			addSubFile(tmpFile, disk);
		} else {
			mtp_err("unknow dir type:0x%x [%s]\n", namelist[i]->d_type, namelist[i]->d_name);
			exit(-1);
		}
		free_wrapper(namelist[i]);
		namelist[i] = NULL;
	}
	if (namelist != NULL)
		free_wrapper(namelist);

	DLOG("subDirCount of <%s>: %u", dir->dirName, dir->subDirCount);
	DLOG("subFileCount of <%s>: %u", dir->dirName, dir->subFileCount);

	return 0;
}

/*
static bool isMountPoint(const char *path, char *dMountDir, size_t len)
{
	char *filename = "/proc/mounts";
	FILE *mntfile;
	struct mntent *mntent;

	mntfile = setmntent(filename, "r");
	if (!mntfile) {
		printf("Failed to read %s.\n", filename);
		return false;
	}
	while (mntent = getmntent(mntfile)) {
		if (!strcmp(path, mntent->mnt_dir) ||
			!strcmp(path, mntent->mnt_fsname)) {
			if (strlen(mntent->mnt_dir) >= len) {
				printf("MountDir too long:%zu, len:%zu\n", strlen(mntent->mnt_dir), len);
				goto err;
			}
			strcpy(dMountDir, mntent->mnt_dir);
			endmntent(mntfile);
			return true;
		}
	}
err:
	endmntent(mntfile);
	return false;
}
*/
extern const char *getDiskType(DiskType type);

static void getDiskInfo(struct Disk *disk)
{
	struct statfs stat;

	DLOG("<%s> statfs:", disk->dMountDir);
	memset(&stat, 0, sizeof(struct statfs));
	statfs(disk->dMountDir, &stat);


#if 1
	DLOG("f_type:0x%04X, len:%u", stat.f_type, sizeof(stat.f_type));
        DLOG("f_bsize:%u, len:%u", stat.f_bsize, sizeof(stat.f_bsize));
        DLOG("f_blocks:%lu, len:%u", stat.f_blocks, sizeof(stat.f_blocks));
        DLOG("f_bfree:%lu, len:%u", stat.f_bfree, sizeof(stat.f_bfree));
        DLOG("f_bavail:%lu, len:%u", stat.f_bavail, sizeof(stat.f_bavail));
        DLOG("f_files:%lu, len:%u", stat.f_files, sizeof(stat.f_files));
        DLOG("f_ffree:%lu, len:%u", stat.f_ffree, sizeof(stat.f_ffree));
        //DLOG("f_fsid:%lu, len:%u", stat.f_fsid, sizeof(stat.f_fsid));
        /* DLOG("f_namelen:%u, len:%u", stat.f_namelen, sizeof(stat.f_namelen)); */
	/* DLOG("f_frsize:%u, len:%u", stat.f_frsize, sizeof(stat.f_frsize)); */
#endif

	DLOG("");
	disk->dFsType = stat.f_type;
	DLOG(" Filesystem Type: %s", getDiskType(disk->dFsType));
	disk->dMaxCap = (DiskMaxCapacity)((DiskMaxCapacity)stat.f_bsize * (DiskMaxCapacity)stat.f_blocks);
	disk->dFreeSpace = (DiskFreeSpace)((DiskFreeSpace)stat.f_bsize * (DiskFreeSpace)stat.f_bavail);
	DLOG("MaxCapacity:    %"PRIu64"M. %"PRIu64"KB. %"PRIu64"bytes", disk->dMaxCap>>20, disk->dMaxCap>>10, disk->dMaxCap);
	DLOG("FreeSpace: %"PRIu64"M. %"PRIu64"KB. %"PRIu64"bytes", disk->dFreeSpace>>20, disk->dFreeSpace>>10, disk->dFreeSpace);

}

	#if 0

		   struct mntent {
		       char *mnt_fsname;   /* name of mounted filesystem */
		       char *mnt_dir;      /* filesystem path prefix */
		       char *mnt_type;     /* mount type (see mntent.h) */
		       char *mnt_opts;     /* mount options (see mntent.h) */
		       int   mnt_freq;     /* dump frequency in days */
		       int   mnt_passno;   /* pass number on parallel fsck */
		   };

		struct stat {
		       __SWORD_TYPE f_type;    /* type of filesystem (see below) */
		       __SWORD_TYPE f_bsize;   /* optimal transfer block size */
		       fsblkcnt_t   f_blocks;  /* total data blocks in filesystem */
		       fsblkcnt_t   f_bfree;   /* free blocks in fs */
		       fsblkcnt_t   f_bavail;  /* free blocks available to
						  unprivileged user */
		       fsfilcnt_t   f_files;   /* total file nodes in filesystem */
		       fsfilcnt_t   f_ffree;   /* free file nodes in fs */
		       fsid_t       f_fsid;    /* filesystem id */
		       __SWORD_TYPE f_namelen; /* maximum length of filenames */
		       __SWORD_TYPE f_frsize;  /* fragment size (since Linux 2.6) */
		       __SWORD_TYPE f_spare[5];
		   };

	#endif

struct Disk *DiskInit(const char *path)
{
	struct Disk *disk;
	/* struct mntent mntent; */
	/* char dMountDir[NAME_LENGTH_MAX]; */
	size_t len;

	/*
	memset(dMountDir, 0, sizeof(dMountDir));
	if (!isMountPoint(path, dMountDir, sizeof(dMountDir))) {
		printf("[%s] mount info Not Found!\n", path);
		strcpy(dMountDir, path);
	}
	*/

	disk = (struct Disk *)calloc_wrapper(1, sizeof(struct Disk));

	if (sizeof(const char) * strlen(path) > sizeof(char) * 128)
		len = sizeof(const char) * (strlen(path) + 1);
	else
		len = sizeof(char) * 128;

	disk->dMountDir = calloc_wrapper(1, len);
	disk->dMountDir = strdup_wrapper(path);

	INIT_LIST_HEAD(&disk->dirList);
	INIT_LIST_HEAD(&disk->fileList);

	disk->addSubDir = addSubDir;
	disk->addSubFile = addSubFile;

	getDiskInfo(disk);
	disk->dDirRoot = DirInit(disk->dMountDir, NULL);
	DLOG("======================================================");
	getDirTreeInfo(disk->dDirRoot, disk);
	DLOG("======================================================");

	return disk;
}

void  DiskRelease(struct Disk *disk)
{
	struct list_head *pos = NULL;
	struct list_head *head = &disk->dirList;
	struct list_head *headFile = &disk->fileList;

	if (!disk)
		return ;

	if (disk->dMountDir) {
		free_wrapper(disk->dMountDir);
		disk->dMountDir = NULL;
	}

	if (disk->dDirRoot)
		DirRelease(disk->dDirRoot);

	DLOG("head:%p", head);
#if 1
	list_for_each(pos, head) {
		struct Dir *dir = NULL;

		dir = list_entry(pos, struct Dir, list);
		//deleteDirInfo(dir);
		DLOG("dirName: %s", dir->dirName);
	}
#endif
	while(!list_empty(head)) {
		struct Dir *dir = NULL;
		dir = list_first_entry(head, struct Dir, list);
		DLOG("dir name:%s, list:%p, dir:%p", dir->dirName, &dir->list, dir);
		list_del(&dir->list);
		DirRelease(dir);
	}
	while(!list_empty(headFile)) {
		struct File *file = NULL;
		file = list_first_entry(headFile, struct File, list);
		DLOG("file name:%s, list:%p", file->fileName, &file->list);
		list_del(&file->list);
		FileRelease(file);
	}

	free_wrapper(disk);
}
