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
#include "MtpServer.h"
#include "MtpDebug.h"
#include "MtpProperty.h"
#include "mtp.h"
#include "f_mtp.h"

#include <hal_thread.h>
#include <sys/ioctl.h>
#include <vfs.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>

static const MtpOperationCode kSupportedOperationCodes[] = {
	MTP_OPERATION_GET_DEVICE_INFO,
	MTP_OPERATION_OPEN_SESSION,
	MTP_OPERATION_CLOSE_SESSION,
	MTP_OPERATION_GET_STORAGE_IDS,
	MTP_OPERATION_GET_STORAGE_INFO,
	MTP_OPERATION_GET_NUM_OBJECTS,
	MTP_OPERATION_GET_OBJECT_HANDLES,
	MTP_OPERATION_GET_OBJECT_INFO,
	MTP_OPERATION_GET_OBJECT,
	MTP_OPERATION_GET_THUMB,
	MTP_OPERATION_DELETE_OBJECT,
	MTP_OPERATION_SEND_OBJECT_INFO,
	MTP_OPERATION_SEND_OBJECT,
#if 0
	MTP_OPERATION_INITIATE_CAPTURE,
	MTP_OPERATION_FORMAT_STORE,
	MTP_OPERATION_RESET_DEVICE,
	MTP_OPERATION_SELF_TEST,
	MTP_OPERATION_SET_OBJECT_PROTECTION,
	MTP_OPERATION_POWER_DOWN,
#endif
	MTP_OPERATION_GET_DEVICE_PROP_DESC,
	MTP_OPERATION_GET_DEVICE_PROP_VALUE,
	MTP_OPERATION_SET_DEVICE_PROP_VALUE,
	MTP_OPERATION_RESET_DEVICE_PROP_VALUE,
#if 0
	MTP_OPERATION_TERMINATE_OPEN_CAPTURE,
	MTP_OPERATION_MOVE_OBJECT,
	MTP_OPERATION_COPY_OBJECT,
#endif
	MTP_OPERATION_GET_PARTIAL_OBJECT,
#if 0
	MTP_OPERATION_INITIATE_OPEN_CAPTURE,
#endif
	MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED,
	MTP_OPERATION_GET_OBJECT_PROP_DESC,
	MTP_OPERATION_GET_OBJECT_PROP_VALUE,
	MTP_OPERATION_SET_OBJECT_PROP_VALUE,
	MTP_OPERATION_GET_OBJECT_PROP_LIST,
#if 0
	MTP_OPERATION_GET_INTERDEPENDENT_PROP_DESC,
	MTP_OPERATION_SEND_OBJECT_PROP_LIST,
#endif
	MTP_OPERATION_GET_OBJECT_REFERENCES,
	MTP_OPERATION_SET_OBJECT_REFERENCES,

	MTP_OPERATION_GET_PARTIAL_OBJECT_64,
	MTP_OPERATION_SEND_PARTIAL_OBJECT,
	MTP_OPERATION_TRUNCATE_OBJECT,
	MTP_OPERATION_BEGIN_EDIT_OBJECT,
	MTP_OPERATION_END_EDIT_OBJECT,
};

static const MtpEventCode kSupportedEventCodes[] = {
	MTP_EVENT_OBJECT_ADDED,
	MTP_EVENT_OBJECT_REMOVED,
	MTP_EVENT_STORE_ADDED,
	MTP_EVENT_STORE_REMOVED,
	MTP_EVENT_DEVICE_PROP_CHANGED,
};

static struct MtpStorage *getStorage(MtpStorageID id, struct MtpServer *mServer)
{
	int count = VectorSize(&mServer->mStorageList);
	int i;

	mtp_debug("count = %d id = %u", count, id);
	for (i = 0; i < count; i++) {
		struct MtpStorage *mStorage = VectorObject(i, &mServer->mStorageList);
		if (mStorage->mStorageID == id) {
			mtp_debug("found Storage. path:%s", MtpStorageGetPath(mStorage));
			return mStorage;
		}
	}
	return NULL;
}

static bool hasStorage(struct MtpServer *mServer)
{
	if (VectorSize(&mServer->mStorageList) > 0)
		return true;
	return false;
}

struct ObjectEdit {
	MtpObjectHandle mHandle;
	char *mPath;
	uint64_t mSize;
	MtpObjectFormat mFormat;
	int mFd;
};

static struct ObjectEdit *ObjectEditInit(MtpObjectHandle handle,
					const char *path,
					uint64_t size,
					MtpObjectFormat format, int fd)
{
	struct ObjectEdit *mObjectEdit;

	mObjectEdit = (struct ObjectEdit *)calloc_wrapper(1, sizeof(struct ObjectEdit));

	mObjectEdit->mHandle = handle;
	mObjectEdit->mPath = strdup_wrapper(path);
	mObjectEdit->mSize = size;
	mObjectEdit->mFormat = format;
	mObjectEdit->mFd = fd;

	return mObjectEdit;
}

static void ObjectEditRelease(struct ObjectEdit *mObjectEdit)
{
	if (!mObjectEdit)
		return ;
	if (mObjectEdit->mPath)
		free_wrapper(mObjectEdit->mPath);
	free_wrapper(mObjectEdit);
}

static struct ObjectEdit *getEditObject(MtpObjectHandle handle, Vector *list)
{
	int count = VectorSize(list);
	int i;
	for (i = 0; i < count; i++) {
		struct ObjectEdit *edit = VectorObject(i, list);
		if (edit->mHandle == handle)
			return edit;
	}
	return NULL;
}

static void addEditObject(MtpObjectHandle handle, const char *path, uint64_t size,
			MtpObjectFormat format, int fd, Vector *list)
{
	struct ObjectEdit *edit = ObjectEditInit(handle, path, size, format, fd);

	VectorAdd(edit, list);

	return ;
}

static void removeEditObject(MtpObjectHandle handle, Vector *list)
{
	int count = VectorSize(list);
	int i;

	for (i = 0; i < count; i++) {
		struct ObjectEdit *edit = VectorObject(i, list);
		if (edit->mHandle == handle) {
			ObjectEditRelease(edit);
			VectorRemove(i, list);
		}
	}
}

static void commitEdit(struct ObjectEdit *edit, struct MtpDataBase *mDataBase)
{
	MtpDataBaseEndSendObject((const char *)edit->mPath, edit->mHandle, edit->mFormat, true, mDataBase);
}

static MtpResponseCode doGetStorageIDs(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	int count, i;
	mtp_debug("");
	if(!mServer->mSessionOpen)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	count = VectorSize(&mServer->mStorageList);
	mData->putData32(count, mData);
	for (i = 0; i < count; i++) {
		struct MtpStorage *mStorage = VectorObject(i, &mServer->mStorageList);
		mData->putData32(mStorage->mStorageID, mData);
	}

	return MTP_RESPONSE_OK;
}

static MtpResponseCode doGetStorageInfo(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	MtpStorageID id;
	struct MtpStorage *mStorage = NULL;

	if(!mServer->mSessionOpen)
		return MTP_RESPONSE_SESSION_NOT_OPEN;
	if (mRequest->getParameterCount(mRequest) < 1)
		return MTP_RESPONSE_INVALID_PARAMETER;

	id = mRequest->getParameter(1, mRequest);

	mStorage = getStorage(id, mServer);
	if (!mStorage)
		return MTP_RESPONSE_INVALID_STORAGE_ID;
	/* Type */
	mData->putData16(MTP_STORAGE_FIXED_RAM, mData);
	/* FileSystem Type */
	mData->putData16(MTP_STORAGE_FILESYSTEM_HIERARCHICAL, mData);
	/* AccessCapability */
	mData->putData16(MTP_STORAGE_READ_WRITE, mData);
	/* MaxCapacity */
	mData->putData64(mStorage->mMaxCapacity, mData);
	/* FreeSpace */
	mStorage->mFreeSpace = MtpStorageGetFreeSpace(mStorage);
	mData->putData64(mStorage->mFreeSpace, mData);
	/* Free Space in Objects */
	mData->putData32(1024*1024*1024, mData);
	/* Volume Identifier */
	mData->putString(mStorage->mDescription, mData);
	mData->putData8(0, mData);

	return MTP_RESPONSE_OK;
}

static MtpResponseCode doGetObjectPropsSupported(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	MtpObjectFormat format;

	format = mRequest->getParameter(2, mRequest);

	getSupportedObjectProperties(format, mData);

	return MTP_RESPONSE_OK;
}

static MtpResponseCode doGetObjectReferences(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;

	mData->putData32(0, mData);
	return MTP_RESPONSE_OK;
}

static MtpResponseCode doGetObjectHandles(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	MtpStorageID storageID;
	MtpObjectFormat format;
	MtpObjectHandle parent;
	uint32_t *array;
	size_t arrayNum;

	if(!mServer->mSessionOpen)
		return MTP_RESPONSE_SESSION_NOT_OPEN;
	if (mRequest->getParameterCount(mRequest) < 3)
		return MTP_RESPONSE_INVALID_PARAMETER;

	storageID = mRequest->getParameter(1, mRequest);
	format = mRequest->getParameter(2, mRequest);
	parent = mRequest->getParameter(3, mRequest);

	mtp_debug("storageID:%u", storageID);
	mtp_debug("format:%u", format);
	mtp_debug("parent:%u", parent);

#if 0
	mStorage = getStorage(storageID, mServer);
	if (!mStorage)
		return MTP_RESPONSE_INVALID_STORAGE_ID;
	/* TODO */
	count = DirGetObjectCount(mStorage->mDisk->dDirRoot);
	array = (uint32_t *)malloc_wrapper(sizeof(uint32_t) * count);
	for (i = 0; i < count; i++)
		array[i] = i+1;
	mData->putAData32(array, sizeof(array)/sizeof(uint32_t), mData);
#else
	array = MtpDataBaseGetObjectHandlesList(storageID, format, parent, &arrayNum, mDataBase);
	mData->putAData32(array, arrayNum, mData);
#if 0
	mtp_debug("array num:%u", arrayNum);
	int i;
	for (i = 0; i < arrayNum; i++)
		printf("0x%x ", array[i]);
	printf("\n");
#endif
#endif

	free_wrapper(array);
	return MTP_RESPONSE_OK;
}

static MtpResponseCode doGetObjectPropList(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	struct MtpObjectInfo info;
	MtpObjectHandle handle;
	uint32_t format, property;
	int groupCode, depth, result;

	if (mRequest->getParameterCount(mRequest) < 5)
		return MTP_RESPONSE_INVALID_PARAMETER;

	handle = mRequest->getParameter(1, mRequest);
	format = mRequest->getParameter(2, mRequest);
	property = mRequest->getParameter(3, mRequest);
	groupCode = mRequest->getParameter(4, mRequest);
	depth = mRequest->getParameter(5, mRequest);

	mtp_debug("handle:%u, format:%s, property:%s, group:%d, depth:%d\n",
		handle, getFormatCodeName(format),
		getObjectPropCodeName(property), groupCode, depth);

	memset(&info, 0, sizeof(struct MtpObjectInfo));
	result = MtpDataBaseGetObjectInfo(handle, &info, mDataBase);
	if (result != MTP_RESPONSE_OK)
		return result;

	mtp_debug("");
	info.print(&info);
	getObjectPropertyList(&info, format, property, groupCode, depth, mData);
	mtp_debug("");
#if 0
	/* Object Count */
	mData->putData32(11, mData);
	/* MTP_PROPERTY_STORAGE_ID */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_STORAGE_ID, mData);
	mData->putData16(MTP_TYPE_UINT32, mData);
	mData->putData32(65537, mData);
	/* MTP_PROPERTY_OBJECT_FORMAT */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_OBJECT_FORMAT, mData);
	mData->putData16(MTP_TYPE_UINT16, mData);
	mData->putData16(MTP_FORMAT_ASSOCIATION, mData);
	/* MTP_PROPERTY_PROTECTION_STATUS */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_PROTECTION_STATUS, mData);
	mData->putData16(MTP_TYPE_UINT16, mData);
	mData->putData16(0, mData);
	/* MTP_PROPERTY_OBJECT_SIZE */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_OBJECT_SIZE, mData);
	mData->putData16(MTP_TYPE_UINT64, mData);
	mData->putData64(0, mData);
	/* MTP_PROPERTY_OBJECT_FILE_NAME */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_OBJECT_FILE_NAME, mData);
	mData->putData16(MTP_TYPE_STR, mData);
	mData->putString("Music", mData);
	/* MTP_PROPERTY_DATE_MODIFIED */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_DATE_MODIFIED, mData);
	mData->putData16(MTP_TYPE_STR, mData);
	mData->putString("20100101T080142", mData);
	/* MTP_PROPERTY_PARENT_OBJECT */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_PARENT_OBJECT, mData);
	mData->putData16(MTP_TYPE_UINT32, mData);
	mData->putData32(0, mData);
	/* MTP_PROPERTY_PERSISTENT_UID */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_PERSISTENT_UID, mData);
	mData->putData16(MTP_TYPE_UINT128, mData);
	uint128_t value = {0x01, 0x10001, 0x0, 0x0};
	mData->putData128(value, mData);
	/* MTP_PROPERTY_NAME */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_NAME, mData);
	mData->putData16(MTP_TYPE_STR, mData);
	mData->putString("Music", mData);
	/* MTP_PROPERTY_DISPLAY_NAME */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_DISPLAY_NAME, mData);
	mData->putData16(MTP_TYPE_STR, mData);
	mData->putString("", mData);
	/* MTP_PROPERTY_DATE_ADDED */
	mData->putData32(1, mData);
	mData->putData16(MTP_PROPERTY_DATE_ADDED, mData);
	mData->putData16(MTP_TYPE_STR, mData);
	mData->putString("20100101T080142", mData);
#endif

	return MTP_RESPONSE_OK;
}

static MtpResponseCode doGetObject(struct MtpServer *mServer)
{
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	MtpObjectHandle handle;
	MtpObjectFormat format;
	int result, ret;
	char pathBuf[NAME_LENGTH_MAX];
	size_t pathBufLen = sizeof(pathBuf);
	uint64_t fileLength;
	struct mtp_file_range mfr;

	if (!hasStorage(mServer))
		return MTP_RESPONSE_INVALID_OBJECT_HANDLE;

	if (mRequest->getParameterCount(mRequest) < 1)
		return MTP_RESPONSE_INVALID_PARAMETER;

	handle = mRequest->getParameter(1, mRequest);

	result = MtpDataBaseGetObjectFilePath(handle, pathBuf, pathBufLen, &fileLength, &format, mDataBase);
	if (result != MTP_RESPONSE_OK)
		return result;

	mtp_debug("doGetObject open:%s\n", pathBuf);
	mfr.fd = open(pathBuf, O_RDONLY);
	mtp_debug("doGetObject open fd:%d\n", mfr.fd);
	if (mfr.fd < 0)
		return MTP_RESPONSE_GENERAL_ERROR;
	mfr.offset = 0;
	mfr.length = fileLength;
	mfr.command = mRequest->getOperationCode(mRequest);
	mfr.transaction_id = mRequest->getTransactionID(mRequest);

	mtp_debug("fileLength=%llu", fileLength);
	/* ret = MtpDevHandleSendFile(&mfr, mServer->mDevHandle); */
	ret = ioctl(mServer->mFd, MTP_SEND_FILE_WITH_HEADER, (void *)&mfr);
	mtp_debug("MTP_SEND_FILE_WITH_HEADER returned %d", ret);
	close(mfr.fd);
	if (ret < 0) {
		if (errno == ECANCELED)
			return MTP_RESPONSE_TRANSACTION_CANCELLED;
		else
			return MTP_RESPONSE_GENERAL_ERROR;
	}

	return MTP_RESPONSE_OK;
}

static MtpResponseCode doGetObjectInfo(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	struct MtpObjectInfo info;
	char date[20];
	MtpObjectHandle handle;
	int result;

	if (mRequest->getParameterCount(mRequest) < 1)
		return MTP_RESPONSE_INVALID_PARAMETER;

	handle = mRequest->getParameter(1, mRequest);
	mtp_debug("handle:%u", handle);

	memset(&info, 0, sizeof(struct MtpObjectInfo));
	result = MtpDataBaseGetObjectInfo(handle, &info, mDataBase);
	if (result != MTP_RESPONSE_OK)
		return result;

	/* mStorageID */
	mData->putData32(info.mStorageID, mData);
	/* Format */
	mData->putData16(info.mFormat, mData);
	/* ProtectionStatus */
	mData->putData16(info.mProtectionStatus, mData);

	/* ObjectInfo */
	/* Size */
	mData->putData32(0x1000, mData);
	/* ThumbFormat */
	mData->putData16(info.mThumbFormat, mData);
	/* ThumbCompressedSize */
	mData->putData32(info.mThumbCompressedSize, mData);
	/* ThumbPixWidth */
	mData->putData32(info.mThumbPixWidth, mData);
	/* ThumbPixHeight */
	mData->putData32(info.mThumbPixHeight, mData);
	/* ImagePixWidth */
	mData->putData32(info.mImagePixWidth, mData);
	/* ImagePixHeight */
	mData->putData32(info.mImagePixHeight, mData);
	/* ImagePixDepth */
	mData->putData32(info.mImagePixDepth, mData);
	/* Parent */
	mData->putData32(info.mParent, mData);
	/* AssociationType */
	mData->putData16(info.mAssociationType, mData);
	/* AssociationDesc */
	mData->putData32(info.mAssociationDesc, mData);
	/* SequenceNumber */
	mData->putData32(info.mSequenceNumber, mData);
	/* Name */
	mData->putString(info.mName, mData);
	/* Created Date */
	formatDateTime(info.mDateCreated, date, sizeof(date));
	mData->putString(date, mData);
	/* Modify Date */
	formatDateTime(info.mDateModified, date, sizeof(date));
	mData->putString(date, mData);
	mData->putData8(0, mData);

#if 0
	/* TODO */
	/* mStorageID */
	mData->putData32(65537, mData);
	/* Format */
	mData->putData16(0x3001, mData);
	/* ProtectionStatus */
	mData->putData16(0, mData);


	/* ObjectInfo */
	/* Size */
	mData->putData32(0x1000, mData);
	/* ThumbFormat */
	mData->putData16(0, mData);
	/* ThumbCompressedSize */
	mData->putData32(0, mData);
	/* ThumbPixWidth */
	mData->putData32(0, mData);
	/* ThumbPixHeight */
	mData->putData32(0, mData);
	/* ImagePixWidth */
	mData->putData32(0, mData);
	/* ImagePixHeight */
	mData->putData32(0, mData);
	/* ImagePixDepth */
	mData->putData32(0, mData);
	/* Parent */
	mData->putData32(0, mData);
	/* AssociationType */
	mData->putData16(0, mData);
	/* AssociationDesc */
	mData->putData32(0, mData);
	/* SequenceNumber */
	mData->putData32(0, mData);
	/* Name */
	mData->putString("Test", mData);
	/* Created Date */
	mData->putString("20100101T080142", mData);
	/* Modify Date */
	mData->putString("20100101T080142", mData);
	mData->putData8(0, mData);
#endif
	return MTP_RESPONSE_OK;
}

static MtpResponseCode doSendObjectInfo(struct MtpServer *mServer)
{

	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpPacket *mResponse = mServer->mResponsePacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	char path[PATH_MAX];
	size_t pathLen = sizeof(path);
	uint64_t length, maxFileSize;
	MtpObjectHandle parent, handle = 0;
	MtpStorageID storageID;
	struct MtpStorage *mStorage;
	MtpObjectFormat format;
	uint16_t temp16;
	uint32_t temp32;
	uint16_t associationType;
	uint32_t associationDesc;
	struct MtpStringBuffer name, created, modified, keywords;
	time_t modifiedTime;

	if (mRequest->getParameterCount(mRequest) < 2)
		return MTP_RESPONSE_INVALID_PARAMETER;

	storageID = mRequest->getParameter(1, mRequest);
	mStorage = getStorage(storageID, mServer);
	if (!mStorage)
		return MTP_RESPONSE_INVALID_STORAGE_ID;
	parent = mRequest->getParameter(2, mRequest);

	memset(path, 0, sizeof(path));
	if (parent == MTP_PARENT_ROOT) {
		strcpy(path, MtpStorageGetPath(mStorage));
		//parent = 0;
	}else {
		int result;
		result = MtpDataBaseGetObjectFilePath(parent, path, pathLen, &length, &format, mDataBase);
		if (result != MTP_RESPONSE_OK)
			return result;
		if (format != MTP_FORMAT_ASSOCIATION)
			return MTP_RESPONSE_INVALID_PARENT_OBJECT;
	}

	/* Storage ID */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Object Format */
	if (!mData->getUint16(&temp16, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	format = temp16;
	/* Protectioon Status */
	if (!mData->getUint16(&temp16, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Object Compressed Size */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	mServer->mSendObjectFileSize = temp32;
	/* Thumb Format */
	if (!mData->getUint16(&temp16, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Thumb Compressed Size */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Thumb Pix Width */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Thumb Pix Height */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Image Pix Width */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Image Pix Height */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Image Bit Depth */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Parent Object */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Association Type */
	if (!mData->getUint16(&temp16, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	associationType = temp16;
	/* Association Description */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	associationDesc = temp32;
	/* Sequence Number */
	if (!mData->getUint32(&temp32, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Filename */
	if (!mData->getString(&name, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Date Created */
	if (!mData->getString(&created, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Date Modified */
	if (!mData->getString(&modified, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;
	/* Keywords */
	if (!mData->getString(&keywords, mData))
		return MTP_RESPONSE_INVALID_PARAMETER;

	if (!strlen(name.mBuffer)) {
		mtp_debug("name length is 0");
		return MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
	}

	mtp_debug("name: %s format: %04X", name.mBuffer, format);

	if (!parseDateTime(modified.mBuffer, &modifiedTime))
		modifiedTime = 0;
	if (path[strlen(path) - 1] != '/')
		path[strlen(path)] = '/';
	strncat(path, name.mBuffer, sizeof(path)-strlen(path)-1);

	if (mServer->mSendObjectFileSize > MtpStorageGetFreeSpace(mStorage))
		return MTP_RESPONSE_STORAGE_FULL;
	maxFileSize = MtpStorageGetMaxFileSize(mStorage);
	if (maxFileSize != 0) {
		if (mServer->mSendObjectFileSize > maxFileSize || mServer->mSendObjectFileSize == 0xFFFFFFFF)
			return MTP_RESPONSE_OBJECT_TOO_LARGE;
	}

	mtp_debug("path: %s parent: %u storageID: 0x%08x", path, parent, storageID);

	handle = MtpDataBaseBeginSendObject(path, format, parent,
					storageID, mServer->mSendObjectFileSize,
					modifiedTime, mDataBase);
	mtp_debug("handle: 0x%x", handle);
	if (handle == kInvalidObjectHandle)
		return MTP_RESPONSE_GENERAL_ERROR;

	if (format == MTP_FORMAT_ASSOCIATION) {
		int ret;
		mode_t mask = umask(0);
		ret = mkdir(path, mServer->mDirectoryPermission);
		umask(mask);
		if (ret && ret != -EEXIST)
			return MTP_RESPONSE_GENERAL_ERROR;
		// TODO: rtos how to change file owner
		/* chown(path, getuid(), mServer->mFileGroup); */
		MtpDataBaseEndSendObject(path, handle, MTP_FORMAT_ASSOCIATION, MTP_RESPONSE_OK, mDataBase);
	} else {
		if (mServer->mSendObjectFilePath) {
			free_wrapper(mServer->mSendObjectFilePath);
			mServer->mSendObjectFilePath = NULL;
		}
		mtp_debug("new file: %s handle: %u", path, handle);
		mServer->mSendObjectFilePath = strdup_wrapper(path);
		mServer->mSendObjectHandle = handle;
		mServer->mSendObjectFormat = format;
	}

	mResponse->setParameter(1, storageID, mResponse);
	mResponse->setParameter(2, parent, mResponse);
	mResponse->setParameter(3, handle, mResponse);

	return MTP_RESPONSE_OK;
}

static MtpResponseCode doSendObject(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	MtpObjectHandle handle;
	MtpResponseCode result = MTP_RESPONSE_OK;
	int ret, initialData, is_canceled = 0;
	struct mtp_file_range mfr;
	mode_t mask;

	TIMESTAMP_INIT();
	if (!hasStorage(mServer))
		return MTP_RESPONSE_GENERAL_ERROR;

	if (mServer->mSendObjectHandle == kInvalidObjectHandle) {
		mtp_debug("Expected SendObjectInfo before SendObject");
		result = MTP_RESPONSE_NO_VALID_OBJECT_INFO;
		goto done;
	}

	if (access(mServer->mSendObjectFilePath, F_OK) == 0) {
		mtp_debug("%s exist", mServer->mSendObjectFilePath);
		result = MTP_RESPONSE_GENERAL_ERROR;
		goto done;
	}

	/* ret = mData->read(mServer->mDevHandle, mData); */
	ret = mData->read(mServer->mFd, mData);
	if (ret < MTP_CONTAINER_HEADER_SIZE) {
		result = MTP_RESPONSE_GENERAL_ERROR;
		goto done;
	}

	initialData = ret - MTP_CONTAINER_HEADER_SIZE;

	mtp_debug("doSendObject open:%s\n", mServer->mSendObjectFilePath);
	mfr.fd = open(mServer->mSendObjectFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	mtp_debug("doSendObject open fd:%d\n", mfr.fd);
	if (mfr.fd < 0) {
		result = MTP_RESPONSE_GENERAL_ERROR;
		goto done;
	}

	//TODO:change file owner
	/*
	fchown(mfr.fd, getuid(), mServer->mFileGroup);
	mask = umask(0);
	fchmod(mfr.fd, mServer->mFilePermission);
	umask(mask);
	*/

	if (initialData > 0)
		ret = write(mfr.fd, mData->getData(mData), initialData);
	mtp_debug("initialData=%d, ret=%d, mSendObjectFileSize=%d(0x%x)\n",
		initialData, ret, mServer->mSendObjectFileSize, mServer->mSendObjectFileSize);
	if (ret < 0) {
		mtp_debug("failed to write initial data");
		result = MTP_RESPONSE_GENERAL_ERROR;
	} else {
		if (mServer->mSendObjectFileSize - initialData > 0) {
			mfr.offset = initialData;
			if (mServer->mSendObjectFileSize == 0xFFFFFFFF) {
				/* tell driver to read until it receives a short packet */
				mfr.length = 0xFFFFFFFF;
			} else {
				mfr.length = mServer->mSendObjectFileSize - initialData;
			}
			mtp_debug("receiving %s, length=%llu, offset=%lld\n", mServer->mSendObjectFilePath, mfr.length, mfr.offset);
			/* transfer the file */
			TIMESTAMP_START();
			ret = ioctl(mServer->mFd, MTP_RECEIVE_FILE, (void *)&mfr);
			/*
			ret = MtpDevHandleReceiveFile(&mfr, mfr.length == 0 &&
				initialData == MTP_BUFFER_SIZE - MTP_CONTAINER_HEADER_SIZE,
				mServer->mDevHandle);
			*/
			TIMESTAMP_END();
			if (ret < 0 && errno == ECANCELED) {
				is_canceled = 1;
			}
			mtp_debug("MTP_RECEIVE_FILE returned %d, errno=%d\n", ret, errno);
		}
	}
	/*
	 * it can't take too much time(less than 5s),
	 * or will make some error(host send GetDeviceStatus)
	 */
	fsync(mfr.fd);
	close(mfr.fd);

	if (ret < 0) {
		unlink(mServer->mSendObjectFilePath);
		if (is_canceled != 0)
			result = MTP_RESPONSE_TRANSACTION_CANCELLED;
		else
			result = MTP_RESPONSE_GENERAL_ERROR;
	}
done:
	mData->reset(mData);
	mtp_debug("result=0x%x", result);
	MtpDataBaseEndSendObject(mServer->mSendObjectFilePath, mServer->mSendObjectHandle,
				mServer->mSendObjectFormat, result == MTP_RESPONSE_OK, mDataBase);
	mServer->mSendObjectHandle = kInvalidObjectHandle;
	mServer->mSendObjectFormat = 0;

	return result;
}

static MtpResponseCode doBeginEditObject(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	MtpObjectHandle handle;
	int result, fd;
	char pathBuf[NAME_LENGTH_MAX];
	size_t pathBufLen = sizeof(pathBuf);
	uint64_t fileLength;
	MtpObjectFormat format;

	if (mRequest->getParameterCount(mRequest) < 1)
		return MTP_RESPONSE_INVALID_PARAMETER;

	handle = mRequest->getParameter(1, mRequest);
	if (getEditObject(handle, &mServer->mObjectEditList)) {
		printf("object already open for edit in doBeginEditObject\n");
		return MTP_RESPONSE_GENERAL_ERROR;
	}

	result = MtpDataBaseGetObjectFilePath(handle, pathBuf, pathBufLen, &fileLength, &format, mDataBase);
	if (result != MTP_RESPONSE_OK)
		return result;
	mtp_debug("doBeginEditObject open:%s\n", pathBuf);
	fd = open((const char *)pathBuf, O_RDWR | O_EXCL);
	mtp_debug("doBeginEditObject open fd:%d\n", fd);
	if (fd < 0) {
		printf("open failed for %s indoBeginEditObject. errno=%d\n",
				pathBuf, errno);
		return MTP_RESPONSE_GENERAL_ERROR;
	}

	addEditObject(handle, (const char *)pathBuf, fileLength, format, fd, &mServer->mObjectEditList);

	return MTP_RESPONSE_OK;
}

static MtpResponseCode doTruncateObject(struct MtpServer *mServer)
{
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct ObjectEdit *edit;
	MtpObjectHandle handle;
	uint64_t offset, offset2;

	if (mRequest->getParameterCount(mRequest) < 3)
		return MTP_RESPONSE_INVALID_PARAMETER;

	handle = mRequest->getParameter(1, mRequest);
	edit = getEditObject(handle, &mServer->mObjectEditList);
	if (!edit) {
		printf("object not open for edit in doTruncateObject\n");
		return MTP_RESPONSE_GENERAL_ERROR;
	}

	offset = mRequest->getParameter(2, mRequest);
	offset2 = mRequest->getParameter(3, mRequest);
	offset |= (offset2 << 32);

	if (!edit->mPath) {
		printf("object get no path\n");
		return MTP_RESPONSE_GENERAL_ERROR;
	}

	if (truncate(edit->mPath, offset) != 0) {
		return MTP_RESPONSE_GENERAL_ERROR;
	} else {
		edit->mSize = offset;
		return MTP_RESPONSE_OK;
	}
}

static MtpResponseCode doSendPartialObject(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpPacket *mResponse = mServer->mResponsePacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	struct ObjectEdit *edit;
	MtpObjectHandle handle;
	uint64_t offset, offset2, end;
	uint32_t length;
	int ret, initialData;

	if (!hasStorage(mServer))
		return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
	if (mRequest->getParameterCount(mRequest) < 4)
		return MTP_RESPONSE_INVALID_PARAMETER;

	handle = mRequest->getParameter(1, mRequest);
	offset = mRequest->getParameter(2, mRequest);
	offset2 = mRequest->getParameter(3, mRequest);
	offset |= (offset2 << 32);
	length = mRequest->getParameter(4, mRequest);

	edit = getEditObject(handle, &mServer->mObjectEditList);
	if (!edit) {
		printf("object not open for edit in doSendPartialObject\n");
		return MTP_RESPONSE_GENERAL_ERROR;
	}

	if (offset > edit->mSize) {
		printf("writing past end of object, offset: %llu edit->mSize: %llu\n", offset, edit->mSize);
		return MTP_RESPONSE_GENERAL_ERROR;
	}
	mtp_debug("receiving partial %s offset:%llu length:%u", edit->mPath, offset, length);

	/* ret = mData->read(mServer->mDevHandle, mData); */
	ret = mData->read(mServer->mFd, mData);
	if (ret < MTP_CONTAINER_HEADER_SIZE)
		return MTP_RESPONSE_GENERAL_ERROR;
	initialData = ret - MTP_CONTAINER_HEADER_SIZE;
	if (initialData > 0) {
		/* ret = pwrite(edit->mFd, mData->getData(mData), initialData, offset); */
		lseek(edit->mFd, offset, SEEK_SET);
		ret = write(edit->mFd, mData->getData(mData), initialData);
		offset += initialData;
		length -= initialData;
	}

	if (ret < 0) {
		printf("failed to write initial data\n");
	} else {
		if (length > 0) {
			struct mtp_file_range mfr;
			mfr.fd = edit->mFd;
			mfr.offset = offset;
			mfr.length = length;

			/*
			ret = MtpDevHandleReceiveFile(&mfr, mfr.length == 0 &&
				initialData == MTP_BUFFER_SIZE - MTP_CONTAINER_HEADER_SIZE,
				mServer->mDevHandle);
			*/
			ret = ioctl(mServer->mFd, MTP_RECEIVE_FILE, (void *)&mfr);
			mtp_debug("MTP_RECEIVE_FILE returned %d", ret);
			fsync(mfr.fd);
		}
	}
	if (ret < 0) {
		mResponse->setParameter(1, 0, mResponse);
		if (errno == ECANCELED)
			return MTP_RESPONSE_TRANSACTION_CANCELLED;
		else
			return MTP_RESPONSE_GENERAL_ERROR;
	}
	mData->reset(mData);
	mResponse->setParameter(1, length, mResponse);
	end = offset + length;
	if (end > edit->mSize)
		edit->mSize = end;

	return MTP_RESPONSE_OK;
}


static MtpResponseCode doDeleteObject(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	struct ObjectEdit *edit;
	MtpObjectHandle handle;
	MtpObjectFormat format;
	char pathBuf[NAME_LENGTH_MAX];
	size_t pathBufLen = sizeof(pathBuf);
	uint64_t fileLength;
	int result;

	if (!hasStorage(mServer))
		return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
	if (mRequest->getParameterCount(mRequest) < 1)
		return MTP_RESPONSE_INVALID_PARAMETER;

	handle = mRequest->getParameter(1, mRequest);

	result = MtpDataBaseGetObjectFilePath(handle, pathBuf, pathBufLen, &fileLength, &format, mDataBase);
	if (result == MTP_RESPONSE_OK) {
		result = MtpDataBaseDeleteFile(handle, mDataBase);
		if (result == MTP_RESPONSE_OK)
			deletePath(pathBuf);
	}

	return result;
}

static MtpResponseCode doEndEditObject(struct MtpServer *mServer)
{
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	struct ObjectEdit *edit;
	MtpObjectHandle handle;

	if (mRequest->getParameterCount(mRequest) < 1)
		return MTP_RESPONSE_INVALID_PARAMETER;
	handle = mRequest->getParameter(1, mRequest);
	edit = getEditObject(handle, &mServer->mObjectEditList);
	if (!edit) {
		printf("object not open for edit in doEndEditObject\n");
		return MTP_RESPONSE_GENERAL_ERROR;
	}

	commitEdit(edit, mDataBase);
	removeEditObject(handle, &mServer->mObjectEditList);
	return MTP_RESPONSE_OK;
}

static MtpResponseCode doGetPartialObject(MtpOperationCode operation, struct MtpServer *mServer)
{
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	uint64_t offset;
	uint32_t length;
	MtpObjectHandle handle;
	int result, ret;
	char pathBuf[NAME_LENGTH_MAX];
	size_t pathBufLen = sizeof(pathBuf);
	uint64_t fileLength;
	struct mtp_file_range mfr;
	MtpObjectFormat format;


	if (!hasStorage(mServer))
		return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
	handle = mRequest->getParameter(1, mRequest);
	offset = mRequest->getParameter(2, mRequest);
	if (operation == MTP_OPERATION_GET_PARTIAL_OBJECT_64) {
		uint64_t offset2;
		if (mRequest->getParameterCount(mRequest) < 4)
			return MTP_RESPONSE_INVALID_PARAMETER;
		offset2 = mRequest->getParameter(3, mRequest);
		offset = offset | (offset2<<32);
		length = mRequest->getParameter(4, mRequest);
	} else {
		if (mRequest->getParameterCount(mRequest) < 3)
			return MTP_RESPONSE_INVALID_PARAMETER;
		length = mRequest->getParameter(3, mRequest);
	}

	result = MtpDataBaseGetObjectFilePath(handle, pathBuf, pathBufLen, &fileLength, &format, mDataBase);
	if (result != MTP_RESPONSE_OK)
		return result;
	if (offset + length > fileLength)
		length = fileLength - offset;

	mtp_debug("doGetPartialObject open:%s \n", pathBuf);
	mfr.fd = open(pathBuf, O_RDONLY);
	mtp_debug("doGetPartialObject open fd:%d\n", mfr.fd);
	if (mfr.fd < 0)
		return MTP_RESPONSE_GENERAL_ERROR;
	mfr.offset = offset;
	mfr.length = length;
	mfr.command = mRequest->getOperationCode(mRequest);
	mfr.transaction_id = mRequest->getTransactionID(mRequest);

	/* ret = MtpDevHandleSendFile(&mfr, mServer->mDevHandle); */
	ret = ioctl(mServer->mFd, MTP_SEND_FILE_WITH_HEADER, (void *)&mfr);
	mtp_debug("MTP_SEND_FILE_WITH_HEADER returned %d", ret);
	close(mfr.fd);
	if (ret < 0) {
		if (errno == ECANCELED)
			return MTP_RESPONSE_TRANSACTION_CANCELLED;
		else
			return MTP_RESPONSE_GENERAL_ERROR;
	}

	return MTP_RESPONSE_OK;
}

static MtpResponseCode doGetObjectPropValue(struct MtpServer *mServer)
{
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	MtpObjectHandle handle;
	MtpObjectProperty property;

	if (!hasStorage(mServer))
		return MTP_RESPONSE_INVALID_OBJECT_HANDLE;

	if (mRequest->getParameterCount(mRequest) < 2)
		return MTP_RESPONSE_INVALID_PARAMETER;

	handle = mRequest->getParameter(1, mRequest);
	property = mRequest->getParameter(2, mRequest);
	mtp_debug("GetObectPropValue %u %s", handle, getObjectPropCodeName(property));

	return MtpDataBaseGetObjectPropertyValue(handle, property, mData, mDataBase);
}

static MtpResponseCode doSetObjectPropValue(struct MtpServer *mServer)
{
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	MtpObjectHandle handle;
	MtpObjectProperty property;

	if (!hasStorage(mServer))
		return MTP_RESPONSE_INVALID_OBJECT_HANDLE;

	if (mRequest->getParameterCount(mRequest) < 2)
		return MTP_RESPONSE_INVALID_PARAMETER;

	handle = mRequest->getParameter(1, mRequest);
	property = mRequest->getParameter(2, mRequest);
	mtp_debug("SetObectPropValue %u %s", handle, getObjectPropCodeName(property));

	return MtpDataBaseSetObjectPropertyValue(handle, property, mData, mDataBase);
}

static MtpResponseCode doGetObjectPropDesc(struct MtpServer *mServer)
{
	MtpObjectProperty propCode;
	MtpObjectFormat format;
	struct MtpProperty *mProperty = NULL;
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;

	if (mRequest->getParameterCount(mRequest) < 2)
		return MTP_RESPONSE_INVALID_PARAMETER;

	propCode = mRequest->getParameter(1, mRequest);
	format = mRequest->getParameter(2, mRequest);
	mtp_debug("GetObjectPropDesc %s %s", getObjectPropCodeName(propCode),
					getFormatCodeName(format));
	mProperty = mDataBase->getObjectPropertyDesc(propCode, format);
	if (!mProperty)
		return MTP_RESPONSE_OBJECT_PROP_NOT_SUPPORTED;
	mProperty->write(mData, mProperty);

	MtpPropertyRelease(mProperty);
	return MTP_RESPONSE_OK;
}

static MtpResponseCode doGetDevicePropValue(struct MtpServer *mServer)
{
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	MtpDeviceProperty property;

	if (mRequest->getParameterCount(mRequest) < 1)
		return MTP_RESPONSE_INVALID_PARAMETER;

	property = mRequest->getParameter(1, mRequest);
	mtp_debug("GetDevicePropValue %s", getDevicePropCodeName(property));

	return mDataBase->getDevicePropertyValue(property, mData);
}

static MtpResponseCode doGetDevicePropDesc(struct MtpServer *mServer)
{
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;
	struct MtpProperty *mProperty = NULL;
	MtpDeviceProperty propCode;

	if (mRequest->getParameterCount(mRequest) < 1)
		return MTP_RESPONSE_INVALID_PARAMETER;

	propCode = mRequest->getParameter(1, mRequest);
	mtp_debug("GetDevicePropDesc %s", getDevicePropCodeName(propCode));

	mProperty = mDataBase->getDevicePropertyDesc(propCode);
	if (!mProperty)
		return MTP_RESPONSE_DEVICE_PROP_NOT_SUPPORTED;
	mProperty->write(mData, mProperty);

	//TODO
	MtpPropertyRelease(mProperty);
	return MTP_RESPONSE_OK;
}
/*
#define CMDLINE_SIZE 2048
static char* get_cmdline_val(const char* name, char* out, int len)
{
        char line[CMDLINE_SIZE + 1], *c, *sptr;
        int fd = open("/proc/cmdline", O_RDONLY);
        ssize_t r = read(fd, line, sizeof(line) - 1);
        close(fd);

        if (r <= 0)
                return NULL;

        line[r] = 0;

        for (c = strtok_r(line, " \t\n", &sptr); c;
                        c = strtok_r(NULL, " \t\n", &sptr)) {
                char *sep = strchr(c, '=');
                ssize_t klen = sep - c;
                if (klen < 0 || strncmp(name, c, klen) || name[klen] != 0)
                        continue;

                strncpy(out, &sep[1], len);
                out[len-1] = 0;
                return out;
        }

        return NULL;
}
*/

static MtpResponseCode doGetDeviceInfo(struct MtpServer *mServer)
{
	char string[NAME_LENGTH_MAX];
	struct MtpPacket *mData = mServer->mDataPacket;
	struct MtpDataBase *mDataBase = mServer->mDataBase;

	/* fill in device info */
	mData->putData16(MTP_STANDARD_VERSION, mData);
	if (mServer->mPtp) {
		mData->putData32(0, mData);
	} else {
		/* MTP Vendor Extension ID */
		mData->putData32(6, mData);
	}
	mData->putData16(MTP_STANDARD_VERSION, mData);
	if (mServer->mPtp) {
		/* no extensions */
		strncpy(string, "", sizeof(string));
	} else {
		/* MTP extensions */
		strncpy(string, "microsoft.com: 1.0; android.com: 1.0;", sizeof(string));
	}
	mData->putString(string, mData);
	/* Functional Mode */
	mData->putData16(0, mData);

	/* Supported Operations */
	mData->putAData16(kSupportedOperationCodes,
			sizeof(kSupportedOperationCodes) / sizeof(uint16_t), mData);
	/* Supportes Events */
	mData->putAData16(kSupportedEventCodes,
			sizeof(kSupportedEventCodes) / sizeof(uint16_t), mData);
	/* Supported Devices Properties */
	mData->putAData16(mDataBase->mDeviceProperties->array,
			mDataBase->mDeviceProperties->size, mData);
	/* Supported Capture Formats */
	mData->putAData16(mDataBase->mCaptureFormats->array,
			mDataBase->mCaptureFormats->size, mData);
	/* Supported Playback Formats */
	mData->putAData16(mDataBase->mPlaybackFormats->array,
			mDataBase->mPlaybackFormats->size, mData);

	/* Manufacturer */
	mData->putString("Allwinner", mData);
	/* Model */
#ifdef MODEL_NAME
	mData->putString(MODEL_NAME, mData);
#else
	/*
	if (!gethostname(string, sizeof(string)))
		mData->putString(string, mData);
	else
	*/
	mData->putString("FreeRTOS", mData);
#endif
	/* Device Version */
	mData->putString("1.0", mData);
	/* Serial Number */

	/*
	if (!get_cmdline_val("androidboot.serialno", string, sizeof(string))
		|| !strcmp(string, "<NULL>"))
	*/
	strcpy(string, "20080411");
	mData->putString(string, mData);

	return MTP_RESPONSE_OK;
}

static MtpResponseCode doOpenSession(struct MtpServer *mServer)
{
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpPacket *mResponse = mServer->mResponsePacket;

	mtp_debug("");
	if (mServer->mSessionOpen) {
		mtp_debug("");
		mResponse->setParameter(1, mServer->mSessionID, mResponse);
		return MTP_RESPONSE_SESSION_ALREADY_OPEN;
	}
	if (mRequest->getParameterCount(mRequest) < 1) {
		mtp_debug("");
		return MTP_RESPONSE_INVALID_PARAMETER;
	}

	mServer->mSessionID = mRequest->getParameter(1, mRequest);

	//mDatabase->sessionStarted();
	mtp_debug("");
	mServer->mSessionOpen = true;
	mtp_debug("");

	return MTP_RESPONSE_OK;
}

static MtpResponseCode doCloseSession(struct MtpServer *mServer)
{
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpPacket *mResponse = mServer->mResponsePacket;

	mtp_debug("");

	if (!mServer->mSessionOpen)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	mServer->mSessionID = 0;
	mServer->mSessionOpen = false;

	return MTP_RESPONSE_OK;
}

static bool handleRequest(struct MtpServer *mServer)
{
	struct MtpPacket *mRequest = mServer->mRequestPacket;
	struct MtpPacket *mResponse = mServer->mResponsePacket;
	MtpResponseCode response;
	MtpOperationCode operation;
	int containertype;

	mtp_debug("");

	/* pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); */

	mServer->handle_status = 1;

	operation = mRequest->getOperationCode(mRequest);

	mResponse->reset(mResponse);

	containertype = mRequest->getContainerType(mRequest);
	if (containertype != MTP_CONTAINER_TYPE_COMMAND) {
		mtp_debug("wrong container type %d", containertype);
		return false;
	}
	mtp_debug("got command: %s (%x)", getOperationCodeName(operation), operation);

	switch(operation) {
		case MTP_OPERATION_GET_DEVICE_INFO:
			response = doGetDeviceInfo(mServer);
			break;
		case MTP_OPERATION_OPEN_SESSION:
			response = doOpenSession(mServer);
			break;
		case MTP_OPERATION_CLOSE_SESSION:
			response = doCloseSession(mServer);
			break;
		case MTP_OPERATION_GET_STORAGE_IDS:
			response = doGetStorageIDs(mServer);
			break;
		case MTP_OPERATION_GET_STORAGE_INFO:
			response = doGetStorageInfo(mServer);
			break;
		case MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED:
			response = doGetObjectPropsSupported(mServer);
			break;
		case MTP_OPERATION_GET_OBJECT_HANDLES:
			response = doGetObjectHandles(mServer);
			break;
		case MTP_OPERATION_GET_OBJECT_REFERENCES:
			response = doGetObjectReferences(mServer);
			break;
		case MTP_OPERATION_GET_OBJECT_PROP_DESC:
			response = doGetObjectPropDesc(mServer);
			break;
		case MTP_OPERATION_GET_OBJECT_PROP_VALUE:
			response = doGetObjectPropValue(mServer);
			break;
		case MTP_OPERATION_SET_OBJECT_PROP_VALUE:
			response = doSetObjectPropValue(mServer);
			break;
		case MTP_OPERATION_GET_OBJECT_PROP_LIST:
			response = doGetObjectPropList(mServer);
			break;
		case MTP_OPERATION_GET_OBJECT_INFO:
			response = doGetObjectInfo(mServer);
			break;
		case MTP_OPERATION_GET_OBJECT:
			response = doGetObject(mServer);
			break;
		case MTP_OPERATION_GET_PARTIAL_OBJECT:
		case MTP_OPERATION_GET_PARTIAL_OBJECT_64:
			response = doGetPartialObject(operation, mServer);
			break;
		case MTP_OPERATION_SEND_OBJECT_INFO:
			response = doSendObjectInfo(mServer);
			break;
		case MTP_OPERATION_SEND_OBJECT:
			response = doSendObject(mServer);
			break;
		case MTP_OPERATION_BEGIN_EDIT_OBJECT:
			response = doBeginEditObject(mServer);
			break;
		case MTP_OPERATION_TRUNCATE_OBJECT:
			response = doTruncateObject(mServer);
			break;
		case MTP_OPERATION_SEND_PARTIAL_OBJECT:
			response = doSendPartialObject(mServer);
			break;
		case MTP_OPERATION_END_EDIT_OBJECT:
			response = doEndEditObject(mServer);
			break;
		case MTP_OPERATION_DELETE_OBJECT:
			response = doDeleteObject(mServer);
			break;
		case MTP_OPERATION_GET_DEVICE_PROP_VALUE:
			response = doGetDevicePropValue(mServer);
			break;
		case MTP_OPERATION_GET_DEVICE_PROP_DESC:
			response = doGetDevicePropDesc(mServer);
			break;
		default:
			mtp_debug("TODO unknown operation!");
			break;
	}
	if (response == MTP_RESPONSE_TRANSACTION_CANCELLED)
		return false;
	mResponse->setResponseCode(response, mResponse);

	/* pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); */
	mServer->handle_status = 0;

	return true;
}



static void MtpServerRun(void *arg)
{
	/* struct MtpDevHandle *handle; */
	struct MtpServer *mServer = (struct MtpServer *)arg;
	struct MtpPacket *mRequest;
	struct MtpPacket *mResponse;
	struct MtpPacket *mData;
	int fd = 0;
	TIMESTAMP_INIT();

	if (!mServer)
		return;
	mtp_debug("");
retry:
	/* handle = mServer->mDevHandle; */
	mServer->mFd = open("/dev/mtp", O_RDWR);
	if (mServer->mFd < 0) {
		mtp_err("open /dev/mtp failed\n");
		hal_msleep(100);
		goto retry;
	}
	mtp_debug("open mtp fd:%d\n", mServer->mFd);
	fd = mServer->mFd;
	mRequest = mServer->mRequestPacket;
	mResponse = mServer->mResponsePacket;
	mData = mServer->mDataPacket;

	while(1) {
		int ret;
		MtpOperationCode operation;
		MtpTransactionID transaction;
		bool dataIn;

		mtp_debug("");
		/* ret = mRequest->read(handle, mRequest); */
		ret = mRequest->read(fd, mRequest);
		if (ret < 0) {
			mtp_debug("request read returned %d, errno=%d[%s]\n", ret, errno, strerror(errno));
			if (errno == ECANCELED) {
				mtp_info("read transfer is canceled\n");
				continue;
			}
			break;
		}

		mtp_debug("");
		operation = mRequest->getOperationCode(mRequest);
		transaction = mRequest->getTransactionID(mRequest);
		mtp_debug(" operation: %s (%04X)", getOperationCodeName(operation), operation);
		mRequest->dump(mRequest);

		dataIn = (operation == MTP_OPERATION_SEND_OBJECT_INFO
			    || operation == MTP_OPERATION_SET_OBJECT_REFERENCES
			    || operation == MTP_OPERATION_SET_OBJECT_PROP_VALUE
			    || operation == MTP_OPERATION_SET_DEVICE_PROP_VALUE);
		if (dataIn) {
			/* mData->read(handle, mData); */
			mData->read(fd, mData);
			if (ret < 0) {
				mtp_debug("data read returned %d, errno %d", ret, errno);
				if (errno = ECANCELED) {
					mtp_info("write transfer is canceled\n");
					continue;
				}
				break;
			}
			mtp_debug("received data:");
			mData->dump(mData);
		} else {
			mData->reset(mData);
		}

#if 1
		if (operation == MTP_OPERATION_SEND_OBJECT ||
			operation == MTP_OPERATION_SEND_OBJECT_INFO) {
			TIMESTAMP_START();
		}
#endif
		if (handleRequest(mServer)) {
			mtp_debug("hasData:%d", mData->hasData(mData) ? 1 : 0);
			if (!dataIn && mData->hasData(mData)) {
				mData->setOperationCode(operation, mData);
				mData->setTransactionID(transaction, mData);
				mtp_debug("sending data:");
				mData->dump(mData);
				/* ret = mData->write(handle, mData); */
				ret = mData->write(fd, mData);
				if (ret < 0) {
					mtp_debug("request write returned %d, errno: %d", ret, errno);
					break;
				}
			}
			mResponse->setTransactionID(transaction, mResponse);
			mtp_debug("sending response %04X", mResponse->getResponseCode(mResponse));
			/* ret = mResponse->write(handle, mResponse); */
			ret = mResponse->write(fd, mResponse);
			mResponse->dump(mResponse);
			if (ret < 0) {
				mtp_debug("request write returned %d, errno%d", ret, errno);
				break;
			}
		} else {
			mtp_debug("skipping response");
		}
#if 1
		if(operation == MTP_OPERATION_SEND_OBJECT ||
			operation == MTP_OPERATION_SEND_OBJECT_INFO) {
			TIMESTAMP_END();
		}
#endif
	}
	mtp_info("MtpServerRun exit\n");
	close(mServer->mFd);
	mServer->mFd = 0;
	while (1)
		hal_msleep(100);
}

static int run(struct MtpServer *mServer)
{
	int ret;
	/* pthread_t tid; */
	hal_thread_t tid;

	mtp_info("MtpServer begin run\n");
	mServer->status = MTP_STATUS_RUN;
	/* ret = pthread_create(&tid, NULL, &MtpServerRun, (void *)mServer); */
	tid = hal_thread_create(MtpServerRun, (void *)mServer, "MtpServer",
			HAL_THREAD_STACK_SIZE, HAL_THREAD_PRIORITY_SYS);
	if (!tid) {
		printf("pthread_create failed\n");
		return -1;
	}
	mServer->mTid = tid;
	return 0;
}

void MtpServerAddStorage(struct MtpStorage *mStorage, struct MtpServer *mServer)
{
	VectorAdd(mStorage, &mServer->mStorageList);
	mtp_debug("add new Storage (id:%u)", mStorage->mStorageID);
}

void MtpServerDelStorage(struct MtpStorage *mStorage, struct MtpServer *mServer)
{
	VectorRemove_by_object(mStorage, &mServer->mStorageList);
	mtp_debug("del Storage (id:%d) count:%d", mStorage->mStorageID, VectorSize(&mServer->mStorageList));
}

struct MtpServer *MtpServerInit(int fileGroup, int filePerm, int directoryPerm, int fd)
{
	struct MtpServer *mServer;

	mtp_debug("");
	mServer = (struct MtpServer *)calloc_wrapper(1, sizeof(struct MtpServer));
	memset(mServer, 0, sizeof(struct MtpServer));

	/*
	mServer->mDevHandle = MtpDevHandleInit(fd);
	if (!mServer->mDevHandle) {
		fprintf(stderr, "MtpDevHandle Init failed\n");
		free_wrapper(mServer);
		return NULL;
	}
	*/

	mServer->mSessionOpen = false;
	mServer->mFilePermission = filePerm;
	mServer->mDirectoryPermission = directoryPerm;

	mServer->mRequestPacket = MtpPacketInit(512, MTPREQUESTPACKET);
	mServer->mResponsePacket = MtpPacketInit(512, MTPRESPONSEPACKET);
	mServer->mDataPacket = MtpPacketInit(MTP_BUFFER_SIZE, MTPDATAPACKET);
	mServer->mEventPacket = MtpPacketInit(512, MTPEVENTPACKET);

	mServer->mDataBase = MtpDataBaseInit();

	mServer->run = run;

	mServer->mPtp = false;

	mServer->mSendObjectHandle = kInvalidObjectHandle;

	return mServer;
}

void MtpServerRelease(struct MtpServer *mServer)
{
	mtp_debug("");
	if (!mServer)
		return;

	if (mServer->mTid) {
		mtp_debug("");
		mServer->status = MTP_STATUS_EXIT;
		if (mServer->mFd)
			ioctl(mServer->mFd, MTP_EXIT, NULL);
		mtp_debug("");
		while (mServer->handle_status)
			hal_msleep(5);
		if (mServer->mTid) {
			hal_thread_stop(mServer->mTid);
			mtp_debug("");
			mServer->mTid = NULL;
		}
	}
	mtp_debug("");
	if (mServer->mFd)
		close(mServer->mFd);
	mtp_debug("");
	if (mServer->mRequestPacket)
		MtpPacketRelease(mServer->mRequestPacket);
	mtp_debug("");
	if (mServer->mResponsePacket)
		MtpPacketRelease(mServer->mResponsePacket);
	mtp_debug("");
	if (mServer->mDataPacket)
		MtpPacketRelease(mServer->mDataPacket);
	mtp_debug("");
	if (mServer->mEventPacket)
		MtpPacketRelease(mServer->mEventPacket);
	mtp_debug("");
	if (mServer->mDataBase)
		MtpDataBaseRelease(mServer->mDataBase);
	mtp_debug("");
	if (mServer->mSendObjectFilePath)
		free_wrapper(mServer->mSendObjectFilePath);
	mtp_debug("");
	VectorDestroy(&mServer->mStorageList);
	mtp_debug("");
	//VectorDestroy(&mServer->mObjectEditList);
	VectorDestroyWithObject(mServer->mObjectEditList, struct ObjectEdit, ObjectEditRelease);
	free_wrapper(mServer);
	memleak_print();
	memleak_exit();
}

void MtpServerSendEvent(MtpEventCode code, uint32_t param1, struct MtpServer *mServer)
{
	struct MtpPacket *mEvent = mServer->mEventPacket;
	struct MtpPacket *mRequest = mServer->mRequestPacket;

	if (!mServer->mSessionOpen)
		return;
	mEvent->setEventCode(code, mEvent);
	mEvent->setTransactionID(mRequest->getTransactionID(mRequest), mEvent);
	mEvent->setParameter(1, param1, mEvent);
	/* mEvent->write(mServer->mDevHandle, mEvent); */
	mEvent->write(mServer->mFd, mEvent);
}

