/*
** Copyright (C) 1995-2012 Opera Software AS.  All rights reserved.
**
** This file is part of the Opera web browser.  It may not be distributed
** under any circumstances.
**
*/

// Reksio data store implementation compatible with the old Mobile Android
// data store format.

#include "pch.h"

#if defined(MOP_FEATURE_DATASTORE) && defined(MOP_TWEAK_PLATFORM_DATASTORE)

#include <limits.h>

OP_EXTERN_C_BEGIN
#include "core/components/DataStore.h"
#include "core/encoding/tstring.h"
#include "core/pi/LowLevelFile.h"
#include "core/stream/DataStream.h"
#include "core/stream/FileStream.h"
#include "core/util/MOpFile.h"
OP_EXTERN_C_END

struct _MOpDataStore {
  TCHAR* dir;
};


static MOP_STATUS MOpDataStore_init(MOpDataStore* _this, const TCHAR* dir) {
  _this->dir = mop_tstrdup(dir);
  MOP_RETURNIF_NOMEM(_this->dir);
  return MOP_STATUS_OK;
}

MOP_STATUS MOpDataStore_create(MOpDataStore** _this, const TCHAR* dir,
                               const TCHAR* file UNUSED,
                               struct _MOpKeychain* chain UNUSED) {
  *_this = static_cast<MOpDataStore*>(mop_calloc(1, sizeof(MOpDataStore)));
  MOP_RETURNIF_NOMEM(*_this);

  MOP_STATUS status = MOpDataStore_init(*_this, dir);
  if (status != MOP_STATUS_OK) {
    MOpDataStore_release(*_this);
    *_this = NULL;
    return status;
  }

  return MOP_STATUS_OK;
}

void MOpDataStore_release(MOpDataStore* _this) {
  if (_this) {
    mop_free(_this->dir);
    mop_free(_this);
  }
}

MOP_STATUS MOpDataStore_setSecureType(MOpDataStore* _this UNUSED,
                                      UINT8 type UNUSED) {
  return MOP_STATUS_NOT_IMPLEMENTED;
}

static MOP_STATUS MOpDataStore_getPath(MOpDataStore* _this, int type,
                                       const char* key, char* dest,
                                       size_t dest_size) {
  MOP_ASSERT((type < 0 && !key) || type >= 0);

  int len;

  if (type < 0)
    len = snprintf(NULL, 0, "%s", _this->dir);
  else if (!key)
    len = snprintf(NULL, 0, "%s/%X", _this->dir, type);
  else
    len = snprintf(NULL, 0, "%s/%X/%s", _this->dir, type, key);

  if (len < 0 || static_cast<size_t>(len + 1) > dest_size)
    return MOP_STATUS_ERR;

  if (type < 0)
    snprintf(dest, dest_size, "%s", _this->dir);
  else if (!key)
    snprintf(dest, dest_size, "%s/%X", _this->dir, type);
  else
    snprintf(dest, dest_size, "%s/%X/%s", _this->dir, type, key);

  return MOP_STATUS_OK;
}

BOOL MOpDataStore_recordExists(MOpDataStore* _this, UINT8 type,
                               const TCHAR* key) {
  char path[PATH_MAX];
  MOP_RETURNIF(MOpDataStore_getPath(_this, type, key, path, sizeof(path)));
  return MOpFile_exists(path);
}

MOP_STATUS MOpDataStore_setRecord(MOpDataStore* _this, UINT8 type,
                                  const TCHAR* key,
                                  UINT8* data, UINT32 length) {
  char path[PATH_MAX];
  MOP_RETURNIF(MOpDataStore_getPath(_this, type, key, path, sizeof(path)));
  return MOpFile_saveFile(path, data, length, FALSE);
}

MOP_STATUS MOpDataStore_getRecord(MOpDataStore* _this, UINT8 type,
                                  const TCHAR* key,
                                  UINT8** data, UINT32* length) {
  char path[PATH_MAX];
  MOP_RETURNIF(MOpDataStore_getPath(_this, type, key, path, sizeof(path)));
  return MOpFile_loadFile(path, reinterpret_cast<void**>(data), length);
}

UINT32 MOpDataStore_recordValueSize(MOpDataStore* _this, UINT8 type,
                                    const TCHAR* key) {
  char path[PATH_MAX];
  if (MOpDataStore_getPath(_this, type, key, path,
                           sizeof(path)) != MOP_STATUS_OK)
    return 0;
  return MOpFile_getSize(path);
}

MOP_STATUS MOpDataStore_getRecordEx(MOpDataStore* _this, UINT8 type,
                                    const TCHAR* key,
                                    UINT8* data, UINT32* length) {
  char path[PATH_MAX];
  MOP_RETURNIF(MOpDataStore_getPath(_this, type, key, path, sizeof(path)));
  return MOpFile_loadFileToBuf(path, data, length);
}

void MOpDataStore_removeRecord(MOpDataStore* _this, UINT8 type,
                               const TCHAR* key) {
  char path[PATH_MAX];
  if (MOpDataStore_getPath(_this, type, key, path,
                           sizeof(path)) != MOP_STATUS_OK)
    return;
  llf_remove(path);
}

static void MOpDataStore_removeDir(const char* dir) {
  LLF_ENUM_TYPE handler = llf_enumInit(dir, LLF_ENUM_DIRS | LLF_ENUM_FILES);
  if (handler == LLF_BAD_ENUM)
    return;

  LLFileInfo info;

  while (llf_enumNext(handler, &info) == 0) {
    if ((info.attr & LLFILE_DIR) != 0)
      MOpDataStore_removeDir(info.path);

    llf_remove(info.path);
  }

  llf_enumClose(handler);
}

void MOpDataStore_removeAllRecordsForType(MOpDataStore* _this, UINT8 type) {
  char path[PATH_MAX];
  if (MOpDataStore_getPath(_this, type, NULL, path,
                           sizeof(path)) != MOP_STATUS_OK)
    return;
  MOpDataStore_removeDir(path);
}

void MOpDataStore_removeAllRecords(MOpDataStore* _this) {
  char path[PATH_MAX];
  if (MOpDataStore_getPath(_this, -1, NULL, path,
                           sizeof(path)) != MOP_STATUS_OK)
    return;
  MOpDataStore_removeDir(path);
}

MOP_STATUS MOpDataStore_getRecordInputStream(MOpDataStore* _this, UINT8 type,
                                             const TCHAR* key,
                                             MOpDataIStream** stream) {
  char path[PATH_MAX];
  MOP_RETURNIF(MOpDataStore_getPath(_this, type, key, path, sizeof(path)));

  MOP_STATUS status = MOpFileStream_create(stream, path);
  // This is what the shared implementation does, comply.
  if (status == MOP_STATUS_NOFILE)
    status = MOP_STATUS_NOTFOUND;

  return status;
}

MOP_STATUS MOpDataStore_getRecordOutputStream(MOpDataStore* _this, UINT8 type,
                                              const TCHAR* key,
                                              MOpDataIOStream** stream) {
  char path[PATH_MAX];
  MOP_RETURNIF(MOpDataStore_getPath(_this, type, key, path, sizeof(path)));
  return MOpFileStream_createOutput(stream, path);
}

#endif  // MOP_FEATURE_DATASTORE && MOP_TWEAK_PLATFORM_DATASTORE
