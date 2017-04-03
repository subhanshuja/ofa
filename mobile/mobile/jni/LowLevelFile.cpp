/* -*- Mode: c; indent-tabs-mode: nil; c-basic-offset: 2; -*-
 *
 * Copyright (C) 2014 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <errno.h>
#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bream/vm/c/graphics/internal_io.h"
#include "platform.h"
#include "LowLevelFile.h"

extern "C" {
#include "core/encoding/tstring.h"

typedef struct _LowLevelDirectoryEnumStruct LowLevelDirectoryEnumStruct;
struct _LowLevelDirectoryEnumStruct {
  DIR* handle;
  int filter;
  SCHAR* dirName; /* name of directory searched */
};
#define LLF_ENUM_TYPE LowLevelDirectoryEnumStruct *

typedef struct {
  FILE* file;
  jobject pfd;
  jobject channel;
  jobject buffer;
  int position;
  int bufferPosition;
  int size;
  int bufferFill;
} llf_t;
#define LLF_TYPE llf_t*

#include "core/pi/LowLevelFile.h"
}

#define MOP_STORAGE_DIR_LEN (sizeof(MOP_STORAGE_DIR) - 1)
#define MOP_PRESTORAGE_DIR_LEN (sizeof(MOP_PRESTORAGE_DIR) - 1)

#define BUFFER_SIZE 2048

/* java.nio.channels.FileChannel */
static jmethodID id_close;
static jmethodID id_position;
static jmethodID id_read;
static jmethodID id_size;

/* android.os.ParcelFileDescriptor */
static jmethodID id_PFD_close;

/* java.nio.Buffer */
static jmethodID id_clear;

static inline char* RuStorage_resolveName(const char* path,
                                          char* dest,
                                          size_t dest_size) {
  int prefix_len = 0;

  if (strncmp(path, MOP_STORAGE_DIR, MOP_STORAGE_DIR_LEN) == 0) {
    prefix_len = MOP_STORAGE_DIR_LEN;
  } else if (strncmp(path, MOP_PRESTORAGE_DIR, MOP_PRESTORAGE_DIR_LEN) == 0) {
    prefix_len = MOP_PRESTORAGE_DIR_LEN;
  }

  if (prefix_len > 0) {
    char* tmp = internal_io_path(path + prefix_len);
    snprintf(dest, dest_size, "%s", tmp);
    internal_io_free(tmp);
  } else {
    snprintf(dest, dest_size, "%s", path);
  }

  return dest;
}

static BOOL isContentUri(const char* path) {
  return !strncmp(path, "content://", 10);
}

/**
 * Creates tree of directories, each with given access mode.
 */
static MOpStatus mkpath(const char* path, int mode) {
  char* t = strdup(path);
  char* tend = t + strlen(t);
  char* a = t;
  char* b;

  for (; a < tend; a = b + 1) {
    b = strchr(a, '/');
    if (!b) {
      b = tend;
    }
    *b = '\0';
    if (strlen(a) > 0) {
      if (mkdir(t, mode) == -1) {
        if (errno != EEXIST) {
          free(t);
          return MOP_STATUS_ERR;
        }
      }
    }
    *b = '/';
  }

  free(t);
  return MOP_STATUS_OK;
}

static jobject createByteBuffer() {
  void* data = malloc(BUFFER_SIZE + 2);
  if (!data) {
    return NULL;
  }
  JNIEnv* env = getEnv();
  jobject byteBuffer = scoped_localref<jobject>(
    env, env->NewDirectByteBuffer(data, BUFFER_SIZE)).createGlobalRef();
  if (!byteBuffer) {
#ifndef NDEBUG
    env->ExceptionDescribe();
#endif
    env->ExceptionClear();
    free(data);
  }
  return byteBuffer;
}

void initLowLevelFile(JNIEnv* env) {
  scoped_localref<jclass> clazz(
      env, env->FindClass("java/nio/channels/FileChannel"));
  MOP_ASSERT(clazz);
  id_close = env->GetMethodID(*clazz, "close", "()V");
  id_position = env->GetMethodID(*clazz, "position", "()J");
  id_read = env->GetMethodID(*clazz, "read", "(Ljava/nio/ByteBuffer;J)I");
  id_size = env->GetMethodID(*clazz, "size", "()J");

  clazz = scoped_localref<jclass>(env, env->FindClass("android/os/ParcelFileDescriptor"));
  MOP_ASSERT(clazz);
  id_PFD_close = env->GetMethodID(*clazz, "close", "()V");

  clazz = scoped_localref<jclass>(env, env->FindClass("java/nio/Buffer"));
  MOP_ASSERT(clazz);
  id_clear = env->GetMethodID(*clazz, "clear", "()Ljava/nio/Buffer;");
}

LLF_TYPE llf_open(const TCHAR* fileName, int mode) {
  const char* unix_mode;
  char unix_name[PATH_MAX];
  LLF_TYPE file = static_cast<llf_t*>(calloc(1, sizeof(llf_t)));

  switch (mode) {
    case LLF_OM_READ:
      unix_mode = "r";
      break;
    case LLF_OM_WRITE:
      unix_mode = "w";
      break;
    case LLF_OM_OVERWRITE:
      unix_mode = "w+";
      break;
    case LLF_OM_APPEND:
      unix_mode = "a";
      break;
    case LLF_OM_UPDATE:
      unix_mode = "r+";
      break;
    default:
      MOP_ASSERT(!"not supported mode for llf_open");
      goto error;
  }

  if (isContentUri(fileName)) {
    MOP_ASSERT(mode == LLF_OM_READ);
    file->buffer = createByteBuffer();
    if (!file->buffer) {
      goto error;
    }
    file->pfd = getParcelFileDescriptor(fileName, unix_mode).createGlobalRef();
    if (!file->pfd) {
      llf_close(file);
      return LLF_BAD_HANDLER;
    }
    file->channel = openParcelFileDescriptor(file->pfd).createGlobalRef();
    if (!file->channel) {
      llf_close(file);
      return LLF_BAD_HANDLER;
    }
    file->size = getEnv()->CallLongMethod(file->channel, id_size);
    return file;
  }

  RuStorage_resolveName(fileName, unix_name, sizeof(unix_name));

  switch (mode) {
    case LLF_OM_WRITE:
    case LLF_OM_OVERWRITE:
    case LLF_OM_APPEND: {
      char* c = strdup(unix_name);
      char* d = dirname(c);
      if (d) {
        mkpath(d, 0755);
      }
      free(c);
      break;
    }
  }

  file->file = fopen(unix_name, unix_mode);
  if (!file->file) {
    goto error;
  }
  return file;

error:
  free(file);
  return LLF_BAD_HANDLER;
}

void llf_close(LLF_TYPE handler) {
  if (handler) {
    if (handler->file) {
      fclose(handler->file);
    }
    if (handler->channel) {
      JNIEnv* env = getEnv();
      env->CallVoidMethod(handler->channel, id_close);
      env->DeleteGlobalRef(handler->channel);
    }
    if (handler->pfd) {
      JNIEnv* env = getEnv();
      env->CallVoidMethod(handler->pfd, id_PFD_close);
      env->DeleteGlobalRef(handler->pfd);
    }
    if (handler->buffer) {
      JNIEnv* env = getEnv();
      void* buffer = env->GetDirectBufferAddress(handler->buffer);
      env->DeleteGlobalRef(handler->buffer);
      free(buffer);
    }
    free(handler);
  }
}

BOOL llf_eof(LLF_TYPE handler) {
  MOP_ASSERT(handler);
  if (handler->channel) {
    return handler->position == handler->size;
  }
  MOP_ASSERT(handler->file);
  return feof(handler->file) != 0;
}

int llf_pos(LLF_TYPE handler) {
  MOP_ASSERT(handler);
  if (handler->channel) {
    return handler->position;
  }
  MOP_ASSERT(handler->file);
  return ftell(handler->file);
}

MOP_STATUS llf_seek(LLF_TYPE handler,
                    int offset,
                    enum LowLevelFileSeekMode mode) {
  int ret = 0;

  MOP_ASSERT(handler);
  if (handler->channel) {
    switch (mode) {
      case LLF_SEEK_START:
        handler->position = offset;
        break;

      case LLF_SEEK_CURRENT:
        handler->position += offset;
        break;

      case LLF_SEEK_END:
        handler->position = handler->size + offset;
        break;
    }
    return MOP_STATUS_OK;
  }

  MOP_ASSERT(handler->file);

  switch (mode) {
    case LLF_SEEK_START:
      ret = fseek(handler->file, offset, SEEK_SET);
      break;

    case LLF_SEEK_CURRENT:
      ret = fseek(handler->file, offset, SEEK_CUR);
      break;

    case LLF_SEEK_END:
      ret = fseek(handler->file, offset, SEEK_END);
      break;
  }

  return ret == 0 ? MOP_STATUS_OK : MOP_STATUS_ERR;
}

int llf_write(const void* data, int len, LLF_TYPE handler) {
  MOP_ASSERT(handler);
  MOP_ASSERT(handler->file);
  return fwrite(data, 1, len, handler->file);
}

int llf_read(void* data, int len, LLF_TYPE handler) {
  MOP_ASSERT(handler);
  if (handler->file) {
    return fread(data, 1, len, handler->file);
  }

  MOP_ASSERT(handler->channel);
  JNIEnv* env = getEnv();
  char* readBuffer =
      static_cast<char*>(env->GetDirectBufferAddress(handler->buffer));
  int bufferReadStart = handler->position - handler->bufferPosition;
  int bufferFill =
      handler->bufferPosition + handler->bufferFill - handler->position;
  int readStart = handler->position;
  if (bufferReadStart >= 0 && bufferFill > 0) {
    if (bufferFill >= len) {
      memcpy(data, &readBuffer[bufferReadStart], len);
      handler->position += len;
      return len;
    }
    memcpy(data, &readBuffer[bufferReadStart], bufferFill);
    handler->position += bufferFill;
    len -= bufferFill;
  }
  int read;
  do {
    if (handler->position >= handler->size) {
      break;
    }
    env->DeleteLocalRef(env->CallObjectMethod(handler->buffer, id_clear));
    read = env->CallIntMethod(handler->channel,
                              id_read,
                              handler->buffer,
                              static_cast<jlong>(handler->position));
    if (env->ExceptionCheck()) {
#ifndef NDEBUG
      env->ExceptionDescribe();
#endif
      env->ExceptionClear();
      break;
    }
    if (read <= 0) {
      break;
    }
    handler->bufferPosition = handler->position;
    handler->bufferFill = read;
    void* dst = static_cast<char*>(data) + handler->position - readStart;
    if (read >= len) {
      memcpy(dst, readBuffer, len);
      handler->position += len;
      break;
    }
    memcpy(dst, readBuffer, read);
    handler->position += read;
    len -= read;
  } while (read == BUFFER_SIZE);
  return handler->position - readStart;
}

int llf_remove(const TCHAR* fileName) {
  char unix_name[PATH_MAX];
  return remove(RuStorage_resolveName(fileName, unix_name, sizeof(unix_name)));
}

int llf_rename(const TCHAR* src, const TCHAR* dst) {
  char unix_src[PATH_MAX], unix_dst[PATH_MAX];

  return rename(RuStorage_resolveName(src, unix_src, sizeof(unix_src)),
                RuStorage_resolveName(dst, unix_dst, sizeof(unix_dst)));
}

int llf_exists(const TCHAR* name) {
  char unix_name[PATH_MAX];

  if (isContentUri(name)) {
    scoped_localref<jobject> pfd = getParcelFileDescriptor(name, "r");
    if (!pfd)
    {
      return 1;
    }
    getEnv()->CallVoidMethod(*pfd, id_PFD_close);
    return 0;
  }

  /* F_OK just requests checking for the existence of the file. */
  return access(RuStorage_resolveName(name, unix_name, sizeof(unix_name)),
                F_OK);
}

int llf_mkdir(const TCHAR* dir) {
  char unix_name[PATH_MAX];
  return mkdir(RuStorage_resolveName(dir, unix_name, sizeof(unix_name)), 0755);
}

LLF_ENUM_TYPE llf_enumInit(const TCHAR* dir, int filter) {
  char unix_name[PATH_MAX];

  // When going up in file browser to top directory, usually
  // dir is empty string. Change it to "/":
  if (strcmp(dir, "") == 0) {
    dir = "/";
  }

  // Workaround for llf_exists(info.name) instead of llf_exists(info.path)
  // in some code of MOpFile.c
  chdir(dir);

  LowLevelDirectoryEnumStruct* ret;

  ret = (LowLevelDirectoryEnumStruct*)mop_malloc(
      sizeof(LowLevelDirectoryEnumStruct));
  if (!ret) {
    return LLF_BAD_ENUM;
  }

  ret->dirName =
      mop_strdup(RuStorage_resolveName(dir, unix_name, sizeof(unix_name)));
  if (!ret->dirName) {
    mop_free(ret);
    return LLF_BAD_ENUM;
  }

  ret->handle = opendir(ret->dirName);
  ret->filter = filter;

  if (!ret->handle) {
    mop_free(ret->dirName);
    mop_free(ret);
    return LLF_BAD_ENUM;
  }
  return ret;
}

int llf_enumNext(LLF_ENUM_TYPE dirEnum, LLFileInfo* info) {
  struct dirent entry_s;
  struct dirent* entry;

  if (dirEnum->handle == NULL) {
    return -1;
  }

  MOP_ASSERT(info && "llf_enumNext: info == NULL!");
  MOP_ASSERT(dirEnum && "llf_enumNext: handler == NULL");
  if (!info || !dirEnum) {
    return -1;
  }

  while (TRUE) {
    if (readdir_r(dirEnum->handle, &entry_s, &entry) == -1) {
      return -1;
    }
    if (!entry) {
      break;
    }
    if (entry->d_name[0] == '.') {
      continue;
    }
    if ((!(dirEnum->filter & LLF_ENUM_DIRS) && entry->d_type == DT_DIR) ||
        (!(dirEnum->filter & LLF_ENUM_FILES) &&
         (entry->d_type == DT_REG || entry->d_type == DT_LNK))) {
      continue;
    }
    break;
  }

  if (entry) {
    // Create complete absolute path:
    char abs_path[PATH_MAX + 1];
    memset(abs_path, 0, PATH_MAX + 1);
    strncat(abs_path, dirEnum->dirName, PATH_MAX);
    if (abs_path[strlen(abs_path) - 1] != '/') {
      strncat(abs_path, "/", PATH_MAX - strlen(abs_path));
    }
    strncat(abs_path, entry->d_name, PATH_MAX - strlen(abs_path));

    struct stat st;
    if (stat(abs_path, &st) == -1) {
      return -1;
    }

    mop_fromutf8ex(entry->d_name, -1, info->name, LLFILE_MAX_NAME_SIZE);
    mop_fromutf8ex(abs_path, -1, info->path, LLFILE_MAX_PATH_SIZE);

    info->size = st.st_size;
    info->creationDate = st.st_mtime; /* use modification time on unix */
    info->attr = (entry->d_type == DT_DIR) ? LLFILE_DIR : LLFILE_NORMAL;

    // access() tests real access rights, but we want effective access rights.
    BOOL writeable = ((st.st_mode & S_IWOTH) ||
                      (st.st_uid == geteuid() && (st.st_mode & S_IWUSR)) ||
                      (st.st_gid == getegid() && (st.st_mode & S_IWGRP)));
    if (!writeable) {
      info->attr += LLFILE_READONLY;
    }

    return 0;
  }
  return -1;
}

void llf_enumClose(LLF_ENUM_TYPE es) {
  if (es->handle) {
    closedir(es->handle);
  }
  if (es->dirName) {
    mop_free(es->dirName);
  }
  mop_free(es);
}
