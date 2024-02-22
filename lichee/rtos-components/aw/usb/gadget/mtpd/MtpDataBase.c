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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* #include <aw_list.h> */
#include <sys/dirent.h>

#include "MtpDataBase.h"
#include "MtpProperty.h"
#include "mtp.h"
#include "MtpServer.h"

static uint16_t DeviceProperties[] = {
	MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER,
	MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME,
	MTP_DEVICE_PROPERTY_IMAGE_SIZE,
	MTP_DEVICE_PROPERTY_BATTERY_LEVEL,
};

static MtpDevicePropertyList mDeviceProperties = {
	.array = DeviceProperties,
	.size = sizeof(DeviceProperties)/sizeof(uint16_t),
};

static MtpObjectFormatList mCaptureFormats = {
	.array = NULL,
	.size = 0,
};
#if 1
static uint16_t PlaybackFormats[] = {
	MTP_FORMAT_UNDEFINED,
	MTP_FORMAT_ASSOCIATION,
	MTP_FORMAT_TEXT,
	MTP_FORMAT_HTML,
	MTP_FORMAT_WAV,
	MTP_FORMAT_MP3,
	MTP_FORMAT_MPEG,
	MTP_FORMAT_EXIF_JPEG,
	MTP_FORMAT_TIFF_EP,
	MTP_FORMAT_BMP,
	MTP_FORMAT_GIF,
	MTP_FORMAT_JFIF,
	MTP_FORMAT_PNG,
	MTP_FORMAT_TIFF,
	MTP_FORMAT_WMA,
	MTP_FORMAT_OGG,
	MTP_FORMAT_AAC,
	MTP_FORMAT_MP4_CONTAINER,
	MTP_FORMAT_MP2,
	MTP_FORMAT_3GP_CONTAINER,
	MTP_FORMAT_ABSTRACT_AV_PLAYLIST,
	MTP_FORMAT_WPL_PLAYLIST,
	MTP_FORMAT_M3U_PLAYLIST,
	MTP_FORMAT_PLS_PLAYLIST,
	MTP_FORMAT_XML_DOCUMENT,
	MTP_FORMAT_FLAC,
};

static MtpObjectFormatList mPlaybackFormats = {
	.array = PlaybackFormats,
	.size = ARRAY_SIZE(PlaybackFormats),
};
#else
static uint16_t PlaybackFormats[] = {
	MTP_FORMAT_ASSOCIATION,
	MTP_FORMAT_TEXT,
};
static MtpObjectFormatList mPlaybackFormats = {
	.array = PlaybackFormats,
	.size = ARRAY_SIZE(PlaybackFormats),
};
#endif

typedef struct {
	MtpObjectProperty   property;
	int type;
} PropertyTableEntry;

static const PropertyTableEntry kObjectPropertyTable[] = {
	{   MTP_PROPERTY_STORAGE_ID,        MTP_TYPE_UINT32     },
	{   MTP_PROPERTY_OBJECT_FORMAT,     MTP_TYPE_UINT16     },
	{   MTP_PROPERTY_PROTECTION_STATUS, MTP_TYPE_UINT16     },
	{   MTP_PROPERTY_OBJECT_SIZE,       MTP_TYPE_UINT64     },
	{   MTP_PROPERTY_OBJECT_FILE_NAME,  MTP_TYPE_STR        },
	{   MTP_PROPERTY_DATE_MODIFIED,     MTP_TYPE_STR        },
	{   MTP_PROPERTY_PARENT_OBJECT,     MTP_TYPE_UINT32     },
	{   MTP_PROPERTY_PERSISTENT_UID,    MTP_TYPE_UINT128    },
	{   MTP_PROPERTY_NAME,              MTP_TYPE_STR        },
	{   MTP_PROPERTY_DISPLAY_NAME,      MTP_TYPE_STR        },
	{   MTP_PROPERTY_DATE_ADDED,        MTP_TYPE_STR        },
	{   MTP_PROPERTY_ARTIST,            MTP_TYPE_STR        },
	{   MTP_PROPERTY_ALBUM_NAME,        MTP_TYPE_STR        },
	{   MTP_PROPERTY_ALBUM_ARTIST,      MTP_TYPE_STR        },
	{   MTP_PROPERTY_TRACK,             MTP_TYPE_UINT16     },
	{   MTP_PROPERTY_ORIGINAL_RELEASE_DATE, MTP_TYPE_STR    },
	{   MTP_PROPERTY_GENRE,             MTP_TYPE_STR        },
	{   MTP_PROPERTY_COMPOSER,          MTP_TYPE_STR        },
	{   MTP_PROPERTY_DURATION,          MTP_TYPE_UINT32     },
	{   MTP_PROPERTY_DESCRIPTION,       MTP_TYPE_STR        },
	{   MTP_PROPERTY_AUDIO_WAVE_CODEC,  MTP_TYPE_UINT32     },
	{   MTP_PROPERTY_BITRATE_TYPE,      MTP_TYPE_UINT16     },
	{   MTP_PROPERTY_AUDIO_BITRATE,     MTP_TYPE_UINT32     },
	{   MTP_PROPERTY_NUMBER_OF_CHANNELS,MTP_TYPE_UINT16     },
	{   MTP_PROPERTY_SAMPLE_RATE,       MTP_TYPE_UINT32     },
};

static const PropertyTableEntry   kDevicePropertyTable[] = {
	{ MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER, MTP_TYPE_STR },
	{ MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME,    MTP_TYPE_STR },
	{ MTP_DEVICE_PROPERTY_IMAGE_SIZE,              MTP_TYPE_STR },
	{ MTP_DEVICE_PROPERTY_BATTERY_LEVEL,           MTP_TYPE_UINT8 },
};

static int getDeviceProperty(int property, long *outIntValue, char *outStringValue)
{
	switch (property) {
		case MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER:
		case MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME:
			/* TODO */
			strcpy(outStringValue, "");
			break;
		case MTP_DEVICE_PROPERTY_IMAGE_SIZE:
			break;
		default:
			break;
	}
	return 0;
}

static bool getObjectPropertyInfo(MtpObjectProperty property, int *type)
{
	int i, count;
	const PropertyTableEntry *entry;

	count = sizeof(kObjectPropertyTable) / sizeof(kObjectPropertyTable[0]);
	entry = kObjectPropertyTable;
	for (i = 0; i < count; i++, entry++) {
		if (entry->property == property) {
			*type = entry->type;
			return true;
		}
	}
	return false;
}

static bool getDevicePropertyInfo(MtpDeviceProperty property, int *type)
{
	int count, i;
	const PropertyTableEntry *entry;

	count = sizeof(kDevicePropertyTable) / sizeof(kDevicePropertyTable[0]);
	entry = kDevicePropertyTable;
	for (i = 0; i < count; i++, entry++) {
		if (entry->property == property) {
			*type = entry->type;
			return true;
		}
	}
	return false;
}


static struct MtpProperty *getObjectPropertyDesc(MtpDeviceProperty property, MtpObjectFormat format)
{
	struct MtpProperty * result = NULL;

	switch (property) {
		case MTP_PROPERTY_OBJECT_FORMAT:
			result = MtpPropertyInit(property, MTP_TYPE_UINT16, false, format);
			break;
		case MTP_PROPERTY_OBJECT_SIZE:
			result = MtpPropertyInit(property, MTP_TYPE_UINT64, false, 0);
			break;
		case MTP_PROPERTY_PERSISTENT_UID:
			result = MtpPropertyInit(property, MTP_TYPE_UINT64, false, 0);
			break;
		case MTP_PROPERTY_OBJECT_FILE_NAME:
			result = MtpPropertyInit(property, MTP_TYPE_STR, true, 0);
			break;
		case MTP_PROPERTY_PROTECTION_STATUS:
			result = MtpPropertyInit(property, MTP_TYPE_UINT16, true, 0);
			break;
		case MTP_PROPERTY_STORAGE_ID:
		case MTP_PROPERTY_PARENT_OBJECT:
			result = MtpPropertyInit(property, MTP_TYPE_UINT32, true, 0);
			break;
		case MTP_PROPERTY_NAME:
		case MTP_PROPERTY_DISPLAY_NAME:
			result = MtpPropertyInit(property, MTP_TYPE_STR, true, 0);
			break;
		case MTP_PROPERTY_DATE_MODIFIED:
		case MTP_PROPERTY_DATE_ADDED:
		case MTP_PROPERTY_ORIGINAL_RELEASE_DATE:
			result = MtpPropertyInit(property, MTP_TYPE_STR, true, 0);
			//snprintf(mProperty->mDefaultValue.str, 64, "%s", );
			break;
		case MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME:
			result = MtpPropertyInit(property, MTP_TYPE_STR, true, 0);
			break;
		default:
			mtp_debug("TODO unknown property!\n");
			return NULL;
	}
	return result;
}

static struct MtpProperty *getDevicePropertyDesc(MtpDeviceProperty property)
{
	bool writable = false;
	struct MtpProperty * result = NULL;

	switch (property) {
		case MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER:
		case MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME:
			writable = true;
		case MTP_DEVICE_PROPERTY_IMAGE_SIZE: {
			result = MtpPropertyInit(property, MTP_TYPE_STR, writable, 0);
			break;
		case MTP_DEVICE_PROPERTY_BATTERY_LEVEL:
			result = MtpPropertyInit(property, MTP_TYPE_UINT8, false, 0);
			/* mBatteryScale */
			result->mMinimumValue.u.u8 = 0;
			result->mMaximumValue.u.u8 = 100;
			result->mStepSize.u.u8 = 1;
			/* mCurrentValue */
			result->mCurrentValue.u.u8 = 50;
			break;
		}
	}
	return result;
}
static MtpResponseCode getDevicePropertyValue(MtpDeviceProperty property, struct MtpPacket *mData)
{
	if (property == MTP_DEVICE_PROPERTY_BATTERY_LEVEL) {
		/* Battery Level */
		mData->putData8(50, mData);
		return MTP_RESPONSE_OK;
	} else {
		int type;

		if (!getDevicePropertyInfo(property, &type))
			return MTP_RESPONSE_DEVICE_PROP_NOT_SUPPORTED;

		//getDeviceProperty(property);
		/* TODO */
		mData->putString("", mData);
	}
	return MTP_RESPONSE_OK;
}

static MtpObjectHandle gHandle = 1;

static void MtpDataBaseBindSubDirObjectInfo(struct MtpStorage *mStorage, struct Dir *parentDir, struct list_head *objectList)
{
	struct list_head *head = &mStorage->mDisk->dirList;
	struct MtpObjectInfo *newObject = NULL;
	struct list_head *pos = NULL, *tmp = NULL;

	list_for_each_safe(pos, tmp, head) {
		struct Dir *dir = NULL;
		dir = list_entry(pos, struct Dir, list);
		if (dir->parentDir != parentDir)
			continue;
		newObject = MtpObjectInfoInitWithDir(gHandle++, dir);
		newObject->fillObject(mStorage, newObject);
		newObject->mParent = ((struct MtpObjectInfo *)dir->parentDir->object)->mHandle;
		list_add(&newObject->mList, objectList);
		MtpDataBaseBindSubDirObjectInfo(mStorage, dir, objectList);
	}
}

static void MtpDataBaseBindObjectInfo(struct MtpStorage *mStorage, struct list_head *objectList)
{
	struct list_head *pos = NULL;
	struct list_head *head = &mStorage->mDisk->dirList;
	struct list_head *headFile = &mStorage->mDisk->fileList;
	struct Dir *dirRoot = mStorage->mDisk->dDirRoot;
	struct MtpObjectInfo *newObject = NULL;
	int i;

	mtp_debug("");
	newObject = MtpObjectInfoInitWithDir(MTP_PARENT_ROOT, dirRoot);
	newObject->fillObject(mStorage, newObject);
	newObject->mParent = 0;
	list_add(&newObject->mList, objectList);
#if 0
	list_for_each(pos, head) {
		struct Dir *dir = NULL;

		dir = list_entry(pos, struct Dir, list);
		mtp_debug("dirName: %s", dir->dirName);
	}
#endif
	MtpDataBaseBindSubDirObjectInfo(mStorage, dirRoot, objectList);
	list_for_each(pos, headFile) {
		struct File *file = NULL;
		struct MtpObjectInfo *newObject = NULL;
		file = list_entry(pos, struct File, list);
		mtp_debug("fileName: %s", file->fileName);
		//sleep(1);
		newObject = MtpObjectInfoInitWithFile(gHandle++, file);
		newObject->fillObject(mStorage, newObject);
		newObject->mParent = ((struct MtpObjectInfo *)file->parentDir->object)->mHandle;
		list_add(&newObject->mList, objectList);
	}

	return;
}

void MtpDataBaseAddStorage(struct MtpStorage *mStorage, struct MtpDataBase *mDataBase)
{
	struct list_head *objectLists;

	VectorAdd(mStorage, &mDataBase->mStorageList);

	objectLists = calloc_wrapper(1, sizeof(struct list_head));
	/* rtos: differ from linux */
	memset(objectLists, 0, sizeof(struct list_head));
	VectorAdd(objectLists, &mDataBase->mRootObjectInfoList);
	INIT_LIST_HEAD(objectLists);

	MtpDataBaseBindObjectInfo(mStorage, objectLists);

	mtp_debug("add new Storage into MtpDataBase MtpObject");
}

void MtpDataBaseDelStorage(struct MtpStorage *mStorage, struct MtpDataBase *mDataBase)
{
	VectorRemove_by_object(mStorage, &mDataBase->mStorageList);
}

static struct list_head *getRootObjectInfo(MtpStorageID id, struct MtpDataBase *mDataBase)
{
	int i;

	for (i = 0; i < VectorSize(&mDataBase->mStorageList); i++) {
		struct MtpStorage *mStorage = VectorObject(i, &mDataBase->mStorageList);
		if (mStorage->mStorageID == id) {
			mtp_debug("found storage ID:%u", id);
			return VectorObject(i, &mDataBase->mRootObjectInfoList);
		}
	}
	return NULL;
}

uint32_t *MtpDataBaseGetObjectHandlesList(MtpStorageID id,
					MtpObjectFormat format,
					MtpObjectHandle parent,
					size_t *arrayNum,
					struct MtpDataBase *mDataBase)
{
	struct list_head *objectLists = getRootObjectInfo(id, mDataBase);

	return getMtpObjectHandlesList(parent, arrayNum, objectLists);
}

static struct MtpObjectInfo *getObjectInfo(MtpObjectHandle handle, struct MtpDataBase *mDataBase)
{
	struct list_head *objectLists = NULL;
	struct MtpObjectInfo *info = NULL;
	int i, num = VectorSize(&mDataBase->mRootObjectInfoList);

	mtp_debug("search handle, num :%d", num);
	for (i = 0; i < num; i++) {
		objectLists = VectorObject(i, &mDataBase->mRootObjectInfoList);
		info = getMtpObjectByHandle(handle, objectLists);
		if (info != NULL) {
			return info;
		}

	}
	return NULL;
}

MtpResponseCode MtpDataBaseGetObjectInfo(MtpObjectHandle handle, struct MtpObjectInfo *object, struct MtpDataBase *mDataBase)
{
	struct MtpObjectInfo *info = NULL;

	info = getObjectInfo(handle, mDataBase);
	if (!info) {
		printf("getMtpObjectByHandle failed, handle:%u\n", handle);
		return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
	}
	*object = *info;
	return MTP_RESPONSE_OK;
}

MtpResponseCode MtpDataBaseGetObjectFilePath(MtpObjectHandle handle,
						char *pathBuf, size_t pathBufLen,
						uint64_t *fileLenth,
						MtpObjectFormat *format,
						struct MtpDataBase *mDataBase)
{
	struct MtpObjectInfo *info;

	info = getObjectInfo(handle, mDataBase);
	if (!info)
		return MTP_RESPONSE_INVALID_OBJECT_HANDLE;

	*format = info->mFormat;
	if (info->mFormat == MTP_FORMAT_ASSOCIATION) {
		struct Dir *dir = info->object.dir;
		if (strlen(dir->path) + 1 > pathBufLen)
			return MTP_RESPONSE_GENERAL_ERROR;
		mtp_debug("dir path:%s", dir->path);
		strcpy(pathBuf, dir->path);
		*fileLenth = dir->dirSize;
	} else {
		struct File *file = info->object.file;
		if (strlen(file->path) + 1 > pathBufLen)
			return MTP_RESPONSE_GENERAL_ERROR;
		mtp_debug("file path:%s", file->path);
		strcpy(pathBuf, file->path);
		*fileLenth = file->fileSize;
	}

	return MTP_RESPONSE_OK;
}

static int MtpDataBaseDelete(MtpObjectHandle handle, struct list_head *objectLists)
{
	struct MtpObjectInfo *info = NULL;

	info = getMtpObjectByHandle(handle, objectLists);
	if (info != NULL) {
		mtp_debug("delete %s", info->mName);
		if (info->mFormat == MTP_FORMAT_ASSOCIATION) {
			uint32_t *array = NULL;
			size_t arrayNum, i;
			array = getMtpObjectHandlesList(handle, &arrayNum, objectLists);
			if (!array)
				return -1;
			for (i = 0; i < arrayNum; i++) {
				mtp_debug("recursion delete handle:%u", handle);
				if (MtpDataBaseDelete(array[i], objectLists) != 0) {
					free_wrapper(array);
					return -1;
				}
			}
			free_wrapper(array);
			mtp_debug("delete dir & object [%s]", info->object.dir->dirName);
			deleteDirInfo(info->object.dir);
			mtp_debug("");
			DirRelease(info->object.dir);
			info->object.dir = NULL;
			mtp_debug("");
		} else {
			mtp_debug("delete file & object [%s]", info->object.file->fileName);
			deleteFileInfo(info->object.file);
			mtp_debug("");
			FileRelease(info->object.file);
			info->object.file = NULL;
			mtp_debug("");
		}
		mtp_debug("");
		deleteObjectInfo(info);
		mtp_debug("");
		MtpObjectInfoRelease(info);
		mtp_debug("");
		return 0;
	}
	return -1;
}

MtpResponseCode MtpDataBaseDeleteFile(MtpObjectHandle handle, struct MtpDataBase *mDataBase)
{
	struct list_head *objectLists = NULL;
	int i, num = VectorSize(&mDataBase->mRootObjectInfoList);

	for (i = 0; i < num; i++) {
		objectLists = VectorObject(i, &mDataBase->mRootObjectInfoList);
		if (!MtpDataBaseDelete(handle, objectLists))
			return MTP_RESPONSE_OK;
	}

	return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
}

void MtpDataBaseEndSendObject(const char *path, MtpObjectHandle handle,
			MtpObjectFormat format, bool succeeded,
			struct MtpDataBase *mDataBase)
{
	struct MtpObjectInfo *info;
	info = getObjectInfo(handle, mDataBase);
	if (!info) {
		mtp_err("get info failed\n");
		return ;
	}
	if (succeeded) {
		if (info->mFormat == MTP_FORMAT_ASSOCIATION)
			updateDirInfo(info->object.dir);
		else
			updateFileInfo(info->object.file);
		updateObjectInfo(info);
	} else {
		/* TODO */
		mtp_debug("!!!!!!! fixme");
		if (info->mFormat == MTP_FORMAT_ASSOCIATION) {
			deleteDirInfo(info->object.dir);
			DirRelease(info->object.dir);
			info->object.dir = NULL;
		} else {
			deleteFileInfo(info->object.file);
			FileRelease(info->object.file);
			info->object.file = NULL;
		}
		deleteObjectInfo(info);
		MtpObjectInfoRelease(info);
		deletePath(path);
	}
}

static struct MtpStorage *getStorage(MtpStorageID id, struct MtpDataBase *mDataBase)
{
	int count = VectorSize(&mDataBase->mStorageList);
	int i;

	mtp_debug("count = %d id = %u", count, id);
	for (i = 0; i < count; i++) {
		struct MtpStorage *mStorage = VectorObject(i, &mDataBase->mStorageList);
		if (mStorage->mStorageID == id) {
			mtp_debug("found Storage. path:%s", MtpStorageGetPath(mStorage));
			return mStorage;
		}
	}
	return NULL;
}

static struct list_head *getObjectList(MtpStorageID id, struct MtpDataBase *mDataBase)
{
	int count = VectorSize(&mDataBase->mStorageList);
	int i;

	mtp_debug("count = %d id = %u", count, id);
	for (i = 0; i < count; i++) {
		struct MtpStorage *mStorage = VectorObject(i, &mDataBase->mStorageList);
		if (mStorage->mStorageID == id) {
			mtp_debug("found Storage. path:%s", MtpStorageGetPath(mStorage));
			return VectorObject(i, &mDataBase->mRootObjectInfoList);
		}
	}
	return NULL;
}

MtpObjectHandle MtpDataBaseBeginSendObject(const char *path, MtpObjectFormat format,
					MtpObjectHandle parent,
					MtpStorageID storageID,
					uint64_t size,
					time_t modified,
					struct MtpDataBase *mDataBase)
{
	struct MtpObjectInfo *info, *objInfo;
	struct MtpStorage *storage;
	struct list_head *objectList;

	mtp_debug("");
	storage = getStorage(storageID, mDataBase);
	if (!storage)
		return kInvalidObjectHandle;
	mtp_debug("");
	info = getObjectInfo(parent, mDataBase);
	if (!info)
		return kInvalidObjectHandle;
	mtp_debug("fount parent ObjectInfo:%s", info->mName);
	objectList = getObjectList(storageID, mDataBase);
	if (!objectList)
		return kInvalidObjectHandle;

	objInfo = getMtpObjectByPath(path, objectList);
	if (objInfo != NULL) {
		mtp_debug("%s objinfo exist", path);
		return kInvalidObjectHandle;
	}

	mtp_debug("");
	if (format == MTP_FORMAT_ASSOCIATION) {
		struct Dir *dir, *parentDir;

		mtp_debug("Dir & Object Init");
		parentDir = info->object.dir;
		dir = DirInit(path, parentDir);
		storage->mDisk->addSubDir(dir, storage->mDisk);

		info = MtpObjectInfoInitWithDir(gHandle++, dir);
		info->fillObject(storage, info);
		info->mParent = parent;
		list_add(&info->mList, objectList);
	} else {
		struct File *file;
		struct Dir *parentDir;

		mtp_debug("File & Object Init");
		parentDir = info->object.dir;
		file = FileInit(path, parentDir);
		storage->mDisk->addSubFile(file, storage->mDisk);
		info = MtpObjectInfoInitWithFile(gHandle++, file);

		info->fillObject(storage, info);
		info->mParent = parent;
		list_add(&info->mList, objectList);
	}

	return info->mHandle;
}

static void getNewFilePath(char *path, size_t pathLen, const char *name)
{
	char *ptr = NULL;
	size_t len = strlen(path);

	if (path[len - 1] == '/')
		path[len - 1] = 0;
	ptr = strrchr(path, '/');
	if (ptr != NULL)
		strcpy(++ptr, name);
	mtp_debug("new path: %s", path);
}

static int renameFile(MtpObjectHandle handle,
			const char *stringValue,
			struct MtpDataBase *mDataBase)
{
	struct MtpObjectInfo *info;

	info = getObjectInfo(handle, mDataBase);

	if (info->mFormat == MTP_FORMAT_ASSOCIATION) {
		struct Dir *dir = info->object.dir;
		char newPath[PATH_MAX];

		strcpy(newPath, dir->path);
		getNewFilePath(newPath, sizeof(newPath), stringValue);
		if (!dir->renameTo(newPath, dir))
			return MTP_RESPONSE_GENERAL_ERROR;

	} else {
		struct File *file = info->object.file;
		char newPath[PATH_MAX];

		strcpy(newPath, file->path);
		getNewFilePath(newPath, sizeof(newPath), stringValue);
		if (!file->renameTo(newPath, file))
			return MTP_RESPONSE_GENERAL_ERROR;

	}
	updateObjectInfo(info);

	return MTP_RESPONSE_OK;
}

static MtpResponseCode setObjectProperty(MtpObjectHandle handle,
					MtpObjectProperty property,
					const char *stringValue,
					struct MtpDataBase *mDataBase)
{
	int result = MTP_RESPONSE_OK;
	switch(property) {
		case MTP_PROPERTY_OBJECT_FILE_NAME:
			result = renameFile(handle, stringValue, mDataBase);
			break;
		case MTP_PROPERTY_NAME:
			/* TODO? */
			mtp_debug("--%s--", stringValue);
			result = MTP_RESPONSE_OK;
			break;
		default:
			mtp_debug("unknow property: 0x%x", property);
			return MTP_RESPONSE_INVALID_OBJECT_PROP_FORMAT;
	}
	return result;
}

static MtpResponseCode getObjectProperty(MtpObjectHandle handle,
					MtpObjectProperty property,
					void *data,
					struct MtpDataBase *mDataBase)
{
	int result = MTP_RESPONSE_OK;
	struct MtpObjectInfo *info;

	info = getObjectInfo(handle, mDataBase);
	if (!info)
		return MTP_RESPONSE_GENERAL_ERROR;

	switch(property) {
	case MTP_PROPERTY_OBJECT_FILE_NAME:
		strcpy(data, info->mName);
		mtp_debug("mName:%s", info->mName);
		break;
	case MTP_PROPERTY_NAME:
		/* TODO? */
		strcpy(data, "test");
		mtp_debug("--%s--", data);
		result = MTP_RESPONSE_OK;
		break;
	case MTP_PROPERTY_OBJECT_FORMAT:
		*(uint16_t *)data = info->mFormat;
		break;
	default:
		mtp_debug("unknow property: 0x%x", property);
		return MTP_RESPONSE_INVALID_OBJECT_PROP_FORMAT;
	}
	return result;
}

MtpObjectHandle MtpDataBaseGetObjectPropertyValue(MtpObjectHandle handle,
						MtpObjectProperty property,
						struct MtpPacket *mData,
						struct MtpDataBase *mDataBase)
{
	int type;
	MtpObjectHandle result = MTP_RESPONSE_INVALID_OBJECT_PROP_FORMAT;

	if (!getObjectPropertyInfo(property, &type))
		return MTP_RESPONSE_OBJECT_PROP_NOT_SUPPORTED;

	switch (type) {
	case MTP_TYPE_STR:
		{
		struct MtpStringBuffer buffer;

		result = getObjectProperty(handle, property, buffer.mBuffer, mDataBase);
		mData->putString(buffer.mBuffer, mData);
		}
		break;
	case MTP_TYPE_UINT16:
		{
			uint16_t value;
			result = getObjectProperty(handle, property, &value, mDataBase);
			mtp_debug("value:0x%x", value);
			mData->putData16(value, mData);
		}
		break;
	default:
		mtp_debug("!!!!! FIXME handle other type value 0x%x", type);
		break;
	}

	return result;
}

MtpObjectHandle MtpDataBaseSetObjectPropertyValue(MtpObjectHandle handle,
						MtpObjectProperty property,
						struct MtpPacket *mData,
						struct MtpDataBase *mDataBase)
{
	int type;
	MtpObjectHandle result = MTP_RESPONSE_INVALID_OBJECT_PROP_FORMAT;

	if (!getObjectPropertyInfo(property, &type))
		return MTP_RESPONSE_OBJECT_PROP_NOT_SUPPORTED;
	if (type == MTP_TYPE_STR) {
		struct MtpStringBuffer buffer;
		if (!mData->getString(&buffer, mData))
			goto fail;
		result = setObjectProperty(handle, property, buffer.mBuffer, mDataBase);
	} else {
		mtp_debug("!!!!! FIXME handle other type value");
	}
fail:
	return result;
}

struct MtpDataBase *MtpDataBaseInit()
{
	struct MtpDataBase *mDataBase;

	mDataBase = (struct MtpDataBase *)calloc_wrapper(1, sizeof(struct MtpDataBase));

	mDataBase->mDeviceProperties = &mDeviceProperties;
	mDataBase->mCaptureFormats = &mCaptureFormats;
	mDataBase->mPlaybackFormats = &mPlaybackFormats;

	mDataBase->getDevicePropertyDesc = getDevicePropertyDesc;
	mDataBase->getObjectPropertyDesc = getObjectPropertyDesc;
	mDataBase->getDevicePropertyValue = getDevicePropertyValue;

	return mDataBase;
}

void MtpDataBaseRelease(struct MtpDataBase *mDataBase)
{
	if (!mDataBase)
		return ;
	//VectorDestroyWithObject(mDataBase->mStorageList, struct MtpStorage, MtpStorageRelease);
	VectorDestroy(&mDataBase->mStorageList);
	//VectorDestroy(&mDataBase->mStorageList);
	VectorDestroyWithObject(mDataBase->mRootObjectInfoList, struct list_head, MtpObjectInfoListRelease);
	//VectorDestroy(&mDataBase->mRootObjectInfoList);
	free_wrapper(mDataBase);
}

static void deleteRecursive(const char *path)
{
	char pathBuf[PATH_MAX];
	size_t pathLength = strlen(path);
	char *fileSpot;
	int pathRemaining;
	DIR *dir;
	struct dirent *entry;

	mtp_debug("delete path:%s", path);
	if (pathLength >= sizeof(pathBuf) - 1) {
		printf("path too long: %s\n", path);
		exit(-1);
	}
	strcpy(pathBuf, path);
	if (pathBuf[pathLength - 1] != '/')
		pathBuf[pathLength++] = '/';

	fileSpot = pathBuf + pathLength;
	pathRemaining = sizeof(pathBuf) - pathLength - 1;
	dir = opendir(path);
	if (!dir) {
		printf("opendir %s failed: %s\n", path, strerror(errno));
		return;
	}
	while ((entry = readdir(dir))) {
		const char *name = entry->d_name;
		int nameLength;

		if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
			continue;
		mtp_debug("name: %s", name);
		nameLength = strlen(name);
		if (nameLength > pathRemaining) {
			printf("path too long: %s name:%s\n", path, name);
			exit(-1);
		}
		strcpy(fileSpot, name);
		if (entry->d_type == DT_DIR) {
			deleteRecursive(pathBuf);
			rmdir(pathBuf);
		} else {
			unlink(pathBuf);
		}
	}
	closedir(dir);
	return ;
}

void deletePath(const char *path)
{
	struct stat sb;
	if (stat(path, &sb) == 0) {
		if (S_ISDIR(sb.st_mode)) {
			deleteRecursive(path);
			rmdir(path);
		} else {
			unlink(path);
		}
	} else {
		mtp_debug("deletePath stat failed: %s: %s", path, strerror(errno));
	}
}

/* MtpToools Command */

static inline void sendObjectAdded(MtpObjectHandle handle, struct MtpServer *mServer)
{
	mtp_debug("handle:%d", handle);
	MtpServerSendEvent(MTP_EVENT_OBJECT_ADDED, handle, mServer);
}

static inline void sendObjectRemoved(MtpObjectHandle handle, struct MtpServer *mServer)
{
	mtp_debug("handle:%d", handle);
	MtpServerSendEvent(MTP_EVENT_OBJECT_REMOVED, handle, mServer);
}

static inline void sendObjectInfoChanged(MtpObjectHandle handle, struct MtpServer *mServer)
{
	mtp_debug("handle:%d", handle);
	MtpServerSendEvent(MTP_EVENT_OBJECT_INFO_CHANGED, handle, mServer);
}

#if 0
	MTP_EVENT_STORE_ADDED
	MTP_EVENT_STORE_REMOVED
	MTP_EVENT_DEVICE_PROP_CHANGED
#endif

static int filter(const struct dirent *dir_ent)
{
	if (strlen(dir_ent->d_name) == 1 && '.' == *dir_ent->d_name)
		return 0;
	if (strlen(dir_ent->d_name) == 2 && !strcmp(dir_ent->d_name, ".."))
		return 0;
	return 1;
}

static void MtpToolsCommandUpdateDir(struct Dir *dir, struct list_head *objectList,
				struct MtpStorage *storage, struct MtpServer *mServer)
{
	struct dirent **namelist = NULL;
	int num, i, j;
	char pathBuf[PATH_MAX];
	int parent = ((struct MtpObjectInfo *)dir->object)->mHandle;
	uint32_t *objectChild = NULL;
	size_t objectChildNum = 0, actualNum = 0;


	objectChild = getMtpObjectHandlesList(parent, &objectChildNum, objectList);
	num = scandir_wrapper(dir->path, &namelist, filter, NULL);
	mtp_debug("objectChildNum = %zu, scandir num = %d", objectChildNum, num);
	for (i = 0; i < num; i++) {
		if (strlen(namelist[i]->d_name) +
			strlen(dir->path) + 1 > PATH_MAX)	{
			printf("path too long : %s %s\n", namelist[i]->d_name, dir->path);
			exit(-1);
		}
		if (namelist[i]->d_type & DT_DIR) {
			struct MtpObjectInfo *objectInfo = NULL;
			objectInfo = getMtpObjectByNameWithParent(namelist[i]->d_name, parent, objectList);
			if (objectInfo != NULL)	 {
				updateDirInfo(objectInfo->object.dir);
				updateObjectInfo(objectInfo);
				actualNum++;
				for (j = 0; j < objectChildNum; j++) {
					if (objectInfo->mHandle == objectChild[j])
						objectChild[j] = 0;
				}
				sendObjectInfoChanged(objectInfo->mHandle, mServer);
			} else {
				struct Dir *tmpDir = NULL;
				snprintf(pathBuf, sizeof(pathBuf), "%s/%s", dir->path, namelist[i]->d_name);
				mtp_debug("-------------------------------------------------------");
				tmpDir = DirInit(pathBuf, dir);
				storage->mDisk->addSubDir(tmpDir, storage->mDisk);

				objectInfo = MtpObjectInfoInitWithDir(gHandle++, tmpDir);
				objectInfo->fillObject(storage, objectInfo);
				objectInfo->mParent = parent;
				list_add(&objectInfo->mList, objectList);
				sendObjectAdded(objectInfo->mHandle, mServer);
			}
			MtpToolsCommandUpdateDir(objectInfo->object.dir, objectList, storage, mServer);
		} else if (namelist[i]->d_type & DT_REG) {
			struct MtpObjectInfo *objectInfo = NULL;
			objectInfo = getMtpObjectByNameWithParent(namelist[i]->d_name, parent, objectList);
			if (objectInfo != NULL)	 {
				updateFileInfo(objectInfo->object.file);
				updateObjectInfo(objectInfo);
				actualNum++;
				for (j = 0; j < objectChildNum; j++) {
					if (objectInfo->mHandle == objectChild[j])
						objectChild[j] = 0;
				}
				sendObjectAdded(objectInfo->mHandle, mServer);
			} else {
				struct File *tmpFile = NULL;
				snprintf(pathBuf, sizeof(pathBuf), "%s/%s", dir->path, namelist[i]->d_name);
				tmpFile = FileInit(pathBuf, dir);
				storage->mDisk->addSubFile(tmpFile, storage->mDisk);

				objectInfo = MtpObjectInfoInitWithFile(gHandle++, tmpFile);

				objectInfo->fillObject(storage, objectInfo);
				objectInfo->mParent = parent;
				list_add(&objectInfo->mList, objectList);
				sendObjectAdded(objectInfo->mHandle, mServer);
			}
		} else {
			printf("unknow dir type:0x%x [%s]\n", namelist[i]->d_type, namelist[i]->d_name);
			exit(-1);
		}
		free_wrapper(namelist[i]);
		namelist[i] = NULL;
	}
	if (namelist != NULL)
		free_wrapper(namelist);
	if (actualNum != objectChildNum) {
		int handle;
		mtp_debug("some dir or file has been delete.");
		for (j = 0; j < objectChildNum; j++) {
			if (objectChild[j] != 0) {
				mtp_debug("handle:%u should be deleted", objectChild[j]);
				MtpDataBaseDelete(objectChild[j], objectList);
				sendObjectRemoved(objectChild[j], mServer);
			}
		}
	}


	if (objectChild != NULL)
		free_wrapper(objectChild);
	return ;
}

int MtpToolsCommandControl(mtp_tools_function_t control, const char *path, MtpObjectFormat format,
			MtpStorageID storageID, struct MtpStorage *storage,
			void *server)
{
	int ret;
	char buf[PATH_MAX];
	char *name = NULL;
	size_t len;
	char *ptr = NULL;
	struct list_head *objectList;
	struct MtpObjectInfo *info, *objectInfo;
	MtpObjectHandle parent;
	int group, permission;
	struct MtpDataBase *mDataBase;
	struct MtpServer *mServer = (struct MtpServer *)server;

	group = mServer->mFileGroup;
	permission = mServer->mFilePermission;
	mDataBase = mServer->mDataBase;

	len = strlen(path);
	memset(buf, 0, sizeof(buf));
	strcpy(buf, path);

	objectList = getObjectList(storageID, mDataBase);
	if (!objectList)
		return -1;

	/* get parent Dir path */
	if (buf[len - 1] == '/')
		buf[len - 1] = '\0';

	/* is Root Dir? */
	if (control == MTP_TOOLS_FUNCTION_UPDATE &&
		!strcmp(storage->mFilePath, buf)) {
		MtpToolsCommandUpdateDir(storage->mDisk->dDirRoot, objectList, storage, mServer);
		return 0;
	}

	ptr = strrchr(buf, '/');
	if (ptr != NULL && ptr != buf) {
		name = ptr+1;
		*ptr = '\0';
	}

	info = getMtpObjectByPath(buf, objectList);
	if (!info) {
		mtp_debug("parent object can not found");
		return -1;
	}
	parent = info->mHandle;
	mtp_debug("fount parent ObjectInfo:%s", info->mName);
	objectInfo = getMtpObjectByNameWithParent(name, parent, objectList);
	if (objectInfo != NULL) {
		if (control == MTP_TOOLS_FUNCTION_UPDATE) {
			if (objectInfo->mFormat == MTP_FORMAT_ASSOCIATION)
				MtpToolsCommandUpdateDir(objectInfo->object.dir, objectList, storage, mServer);
			else
				MtpDataBaseEndSendObject(path, objectInfo->mHandle, objectInfo->mFormat, true, mDataBase);
			sendObjectInfoChanged(objectInfo->mHandle, mServer);
		} else if (control == MTP_TOOLS_FUNCTION_REMOVE) {
			if (MtpDataBaseDeleteFile(objectInfo->mHandle, mDataBase) == MTP_RESPONSE_OK) {
				sendObjectRemoved(objectInfo->mHandle, mServer);
				deletePath(path);
			} else {
				printf("delete handle:%u failed", objectInfo->mHandle);
			}
		} else {
			printf("control=%d, but %s already exist!\n", control, path);
			return -1;
		}
	} else {
		if (control == MTP_TOOLS_FUNCTION_ADD ||
			control == MTP_TOOLS_FUNCTION_UPDATE) {
			if (format == MTP_FORMAT_ASSOCIATION) {
				struct Dir *dir, *parentDir;

				mtp_debug("Dir & Object Init");
				parentDir = info->object.dir;
				dir = DirInit(path, parentDir);
				storage->mDisk->addSubDir(dir, storage->mDisk);

				info = MtpObjectInfoInitWithDir(gHandle++, dir);
				info->fillObject(storage, info);
				info->mParent = parent;
				list_add(&info->mList, objectList);
				//TODO: rtos change file owner
				/* chown(path, getuid(), group); */
				MtpDataBaseEndSendObject(path, info->mHandle, format, true, mDataBase);
				sendObjectAdded(info->mHandle, mServer);
				if (control == MTP_TOOLS_FUNCTION_UPDATE)
					MtpToolsCommandUpdateDir(dir, objectList, storage, mServer);
			} else {
				struct File *file;
				struct Dir *parentDir;
				mode_t mask;

				if (access(path, F_OK) != 0)
					return -1;

				mtp_debug("File & Object Init");
				parentDir = info->object.dir;
				file = FileInit(path, parentDir);
				storage->mDisk->addSubFile(file, storage->mDisk);
				info = MtpObjectInfoInitWithFile(gHandle++, file);

				info->fillObject(storage, info);
				info->mParent = parent;
				list_add(&info->mList, objectList);

				//TODO: rtos change file owner
				/* chown(path, getuid(), group); */
				mask = umask(0);
				chmod(path, permission);
				umask(mask);
				MtpDataBaseEndSendObject(path, info->mHandle, format, true, mDataBase);
				sendObjectAdded(info->mHandle, mServer);
			}
		} else {
			printf("control=%d, but %s isn't exist!\n", control, path);
			return -1;
		}
	}

	return 0;
}


