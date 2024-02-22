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
#include "MtpObjectInfo.h"
#include "MtpCommon.h"
#include "mtp.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <aw_list.h>

struct MtpObjectInfo *getMtpObjectByNameWithParent(const char *name, MtpObjectHandle parent, struct list_head *objectLists)
{
	struct MtpObjectInfo *object = NULL;
	struct list_head *pos = NULL;
	struct list_head *head = objectLists;

	list_for_each(pos, head) {
		object = list_entry(pos, struct MtpObjectInfo, mList);
		mtp_debug("object parent:%u", object->mParent);
		if (object->mParent == parent) {
			if (!strcmp(name, object->mName)) {
				mtp_debug("found object(%s) with parent:%u", object->mName, parent);
				return object;
			}
		}
	}

	return NULL;
}

uint32_t *getMtpObjectHandlesList(MtpObjectHandle parent, size_t *arrayNum, struct list_head *objectLists)
{
	struct MtpObjectInfo *object = NULL;
	struct list_head *pos = NULL;
	struct list_head *head = objectLists;
	uint32_t *array = NULL;
	size_t num = 0;
#define OBJECHANDLESTLIST_ARRAY_NUM 10
	size_t defaultNum = OBJECHANDLESTLIST_ARRAY_NUM;

	array = (uint32_t *)calloc_wrapper(1, defaultNum * sizeof(uint32_t));
	if (!array) {
		mtp_err("malloc failed\n");
		return NULL;
	}
#if 1
	list_for_each(pos, head) {
		object = list_entry(pos, struct MtpObjectInfo, mList);
		mtp_debug("object: name(%s) handle(%u) parent(%u)", object->mName, object->mHandle, object->mParent);
	}
#endif
	list_for_each(pos, head) {
		object = list_entry(pos, struct MtpObjectInfo, mList);
		mtp_debug("object parent:%u", object->mParent);
		if (object->mParent == parent) {
			num++;
			if (num > defaultNum) {
				uint32_t *array_tmp = array;
				defaultNum += OBJECHANDLESTLIST_ARRAY_NUM;
				array = realloc_wrapper(array, defaultNum*sizeof(uint32_t));
				if (!array) {
					free_wrapper(array_tmp);
					return NULL;
				}
			}
			mtp_debug("add handle:%u", object->mHandle);
			array[num-1] = object->mHandle;
		}
	}
	mtp_debug("found (parent:%u) ObjectHandlesList num:%u", parent, num);
	*arrayNum = num;

	return array;
}

struct MtpObjectInfo *getMtpObjectByPath(const char *path, struct list_head *objectLists)
{
	struct MtpObjectInfo *object;
	struct list_head *pos = NULL;
	struct list_head *head = objectLists;

	list_for_each(pos, head) {
		object = list_entry(pos, struct MtpObjectInfo, mList);
		mtp_debug("search path, object name:%s", object->mName);
		if (object->mFormat == MTP_FORMAT_ASSOCIATION) {
			struct Dir *dir = object->object.dir;
			if (!strcmp(path, dir->path)) {
				mtp_debug("found path:%s", dir->path);
				object->print(object);
				return object;
			}
		} else {
			struct File *file = object->object.file;
			if (!strcmp(path, file->path)) {
				mtp_debug("found path:%s", file->path);
				object->print(object);
				return object;
			}
		}
	}

	return NULL;
}

struct MtpObjectInfo *getMtpObjectByHandle(MtpObjectHandle handle, struct list_head *objectLists)
{
	struct MtpObjectInfo *object;
	struct list_head *pos = NULL;
	struct list_head *head = objectLists;

	list_for_each(pos, head) {
		object = list_entry(pos, struct MtpObjectInfo, mList);
		mtp_debug("search handle, object handle:%u", object->mHandle);
		if (object->mHandle == handle) {
			mtp_debug("found handle:%u", handle);
			object->print(object);
			return object;
		}
	}

	return NULL;
}

void MtpObjectSetParent(MtpObjectHandle parent, struct MtpObjectInfo *object)
{
	object->mParent = parent;
}

static void print(struct MtpObjectInfo *info)
{
	mtp_debug("");
	mtp_debug("MtpObject Info 0x%x: %s", info->mHandle, info->mName);
	mtp_debug("StorageID: 0x%x mFormat: 0x%x mProtectionStatus: 0x%0x",
		info->mStorageID, info->mFormat, info->mProtectionStatus);
	mtp_debug("mCompressedSize: %llu mThumbFormat: 0x%x mThumbCompressedSize: %u",
		info->mCompressedSize, info->mThumbFormat, info->mThumbCompressedSize);
	mtp_debug("mThumbPixWidth: %u mThumbPixHeight: %u",
		info->mThumbPixWidth, info->mThumbPixHeight);
	mtp_debug("mImagePixWidth: %u mImagePixHeight: %u",
		info->mImagePixWidth, info->mImagePixHeight);
	mtp_debug("mParent: 0x%x mAsssociationType: 0x%x mAssociationDesc 0x%x",
		info->mParent, info->mAssociationType, info->mAssociationDesc);
	mtp_debug("mSequenceNumber %u mDateCreated %ld mDateModified: %ld mKeywords: %s",
		info->mSequenceNumber, info->mDateCreated, info->mDateModified, info->mKeywords);
}

static void fillObject(struct MtpStorage *mStorage, struct MtpObjectInfo *info)
{
	info->mStorageID = mStorage->mStorageID;
}

void updateObjectInfo(struct MtpObjectInfo *info)
{
	if (info->mName)
		free_wrapper(info->mName);
	if (info->mFormat == MTP_FORMAT_ASSOCIATION) {
		struct Dir *dir = info->object.dir;
		info->mName = strdup_wrapper((const char *)dir->dirName);
		info->mDateCreated = dir->dirAccessTime;
		info->mDateModified = dir->dirModifyTime;
	} else {
		struct File *file = info->object.file;
		info->mName = strdup_wrapper((const char *)file->fileName);
		info->mDateCreated = file->fileAccessTime;
		info->mDateModified = file->fileModifyTime;
		info->mCompressedSize = file->fileSize;
	}
}

void deleteObjectInfo(struct MtpObjectInfo *info)
{
	list_del(&info->mList);
}

struct MtpObjectInfo *MtpObjectInfoInitWithDir(MtpObjectHandle handle, struct Dir *dir)
{
	struct MtpObjectInfo *mObjectInfo = NULL;

	mObjectInfo = (struct MtpObjectInfo *)calloc_wrapper(1, sizeof(struct MtpObjectInfo));

	mObjectInfo->mHandle = handle;
	mObjectInfo->mFormat = MTP_FORMAT_ASSOCIATION;

	INIT_LIST_HEAD(&mObjectInfo->mList);

	dir->object = mObjectInfo;

	mObjectInfo->mName = strdup_wrapper((const char *)dir->dirName);
	mObjectInfo->mDateCreated = dir->dirAccessTime;
	mObjectInfo->mDateModified = dir->dirModifyTime;
	mObjectInfo->mCompressedSize = dir->dirSize;

	mObjectInfo->print = print;
	mObjectInfo->fillObject = fillObject;

	mObjectInfo->object.dir = dir;

	mtp_debug("init !!!object:%p, name:%s!!!", mObjectInfo, mObjectInfo->mName);
	return mObjectInfo;
}

struct MtpObjectInfo *MtpObjectInfoInitWithFile(MtpObjectHandle handle, struct File *file)
{
	struct MtpObjectInfo *mObjectInfo = NULL;

	mObjectInfo = (struct MtpObjectInfo *)calloc_wrapper(1, sizeof(struct MtpObjectInfo));

	mObjectInfo->mHandle = handle;
	/* TODO according file->fileType */
	mObjectInfo->mFormat = MTP_FORMAT_TEXT;
	INIT_LIST_HEAD(&mObjectInfo->mList);

	file->object = mObjectInfo;

	mObjectInfo->mName = strdup_wrapper((const char *)file->fileName);
	mObjectInfo->mDateCreated = file->fileAccessTime;
	mObjectInfo->mDateModified = file->fileModifyTime;
	mObjectInfo->mCompressedSize = file->fileSize;

	mObjectInfo->print = print;
	mObjectInfo->fillObject = fillObject;

	mObjectInfo->object.file = file;
	mtp_debug("init !!!object:%p!!!", mObjectInfo);
	return mObjectInfo;
}

void MtpObjectInfoRelease(struct MtpObjectInfo *mObjectInfo)
{
	mtp_debug("exit !!!object:%p!!!", mObjectInfo);
	if (!mObjectInfo)
		return;
	if (mObjectInfo->mName)
		free_wrapper(mObjectInfo->mName);
	free_wrapper(mObjectInfo);
}

void MtpObjectInfoListRelease(struct list_head *head)
{

	while(!list_empty(head)) {
		struct MtpObjectInfo *object = NULL;
		object = list_first_entry(head, struct MtpObjectInfo, mList);
		mtp_debug("object name:%s, list:%p, object:%p", object->mName, &object->mList, object);
		list_del(&object->mList);
		MtpObjectInfoRelease(object);
	}
	free_wrapper(head);
}
