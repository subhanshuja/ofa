/* -*- Mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#define UNWRAP_OBJECT(_arg, _target)                                    \
    ((_target *)MOpObjectWrapper_get((MOpObjectWrapper *)bream_load_object(vm, _arg)))

#define VMHelpers_IO_toException(_status)                               \
    ((_status) == MOP_STATUS_ERR ? TYPE_BREAM_IO_IOEXCEPTION : VMHelpers_toException((_status)))

BREAMAPI(NATIVE_BREAM_UTIL_NATIVESYSTEM_GETCURRENTTIME) "long NativeSystem.getCurrentTime()"
{
    INT64 result = MOpGenSystemInfo_getMSFromEpoch();
    RETURN_LONG(result);
}

BREAMAPI(NATIVE_BREAM_UTIL_NATIVESYSTEM_GETTIMEZONE) "long NativeSystem.getTimezone()"
{
    INT64 result = MOpGenSystemInfo_getLocalTimeOffset();
    result *= 1000;
    RETURN_LONG(result);
}

BREAMAPI(NATIVE_BREAM_UTIL_NATIVESYSTEM_GETCURRENTTICK) "int NativeSystem.getCurrentTick()"
{
    RETURN_VALUE(MOpSystemInfo_getTickCount());
}

BREAMAPI(NATIVE_BREAM_UTIL_NATIVESYSTEM_SIGNALBREAMOOM) "void NativeSystem.signalBreamOOM()"
{
    RETURN();
}

BREAMAPI(NATIVE_BREAM_UTIL_NATIVESYSTEM_EXIT) "void NativeSystem.exit()"
{
    ((MOpVMInstanceExternal*)inst)->exited = TRUE;
    RETURN();
}

// Boot Manager: mostly completely uninteresting. There's (supposedly) so little
// stuff to load.

BREAMAPI(NATIVE_MOBILE_API_NATIVEBOOTMANAGER_BOOTSTAGE) "void NativeBootManager.bootStage(String text, int stageStart, int stageEnd)"
{
}

BREAMAPI(NATIVE_MOBILE_API_NATIVEBOOTMANAGER_BOOTPROGRESS) "void NativeBootManager.bootProgress(int progress)"
{
}

BREAMAPI(NATIVE_MOBILE_API_NATIVEBOOTMANAGER_BOOTCOMPLETE) "void NativeBootManager.bootComplete()"
{
}

BREAMAPI(NATIVE_MOBILE_API_NATIVEBOOTMANAGER_CLOSESPLASHSCREEN) "void NativeBootManager.closeSplashScreen()"
{
}

BREAMAPI(NATIVE_MOBILE_API_NATIVEBOOTMANAGER_MAINVIEWVISIBLE) "void NativeBootManager.mainViewVisible()"
{
}

BREAMAPI(NATIVE_MOBILE_API_NATIVEBOOTMANAGER_STARTNATIVESETUP) "int NativeBootManager.startNativeSetup()"
{
    RETURN_VALUE(0);
}

BREAMAPI(NATIVE_MOBILE_API_NATIVEBOOTMANAGER_GETNATIVESETUPPROGRESS) "int NativeBootManager.getNativeSetupProgress()"
{
    RETURN_VALUE(100);
}

BREAMAPI(NATIVE_MOBILE_API_NATIVEBOOTMANAGER_GETNATIVESETUPERROR) "int NativeBootManager.getNativeSetupError()"
{
    RETURN_VALUE(-1);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEINPUTSTREAM_LOADRESOURCE) "NativeByteArray NativeInputStream.loadResource(int res)"
{
    /* TODO: Remove when we've removed all references to resources in bream */
    RETURN_PTR(0);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEINPUTSTREAM_OPENRESOURCESTREAM) "NativeInputStream NativeInputStream.openResourceStream(int res)"
{
    /* TODO: Remove when we've removed all references to resources in bream */
    RETURN_PTR(0);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEINPUTSTREAM_READ) "int NativeInputStream.read(NativeByteArray data, int offset, int length, NativeInputStream this)"
{
    MOpDataIStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIStream);
    bream_byte_t* data;
    UINT32 ret;
    MOP_STATUS status;
    CHECK_NULL(stream);
    CHECK_BYTEARRAY(ARG(data), ARG(offset), ARG(length));
    data = bream_bytearray_get_ptr(vm, ARG(data), ARG(offset));
    status = MOpDataIStream_read(stream, data, ARG(length), &ret);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN_VALUE(ret);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEINPUTSTREAM_SKIP) "int NativeInputStream.skip(int bytes, NativeInputStream this)"
{
    MOpDataIStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIStream);
    INT32 ret;
    MOP_STATUS status;
    CHECK_NULL(stream);
    if (ARG(bytes) < 0)
        THROW(TYPE_BREAM_LANG_ILLEGALARGUMENTEXCEPTION, NULL);
    status = MOpDataIStreamEx_skip(stream, ARG(bytes), &ret);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN_VALUE(ret);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEINPUTSTREAM_CLOSE) "void NativeInputStream.close(NativeInputStream this)"
{
    MOpDataIStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIStream);
    CHECK_NULL(stream);
    stream->release(stream, FALSE);
    RETURN();
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEINPUTSTREAM_READBYTE) "byte NativeInputStream.readByte(NativeInputStream this)"
{
    MOpDataIStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIStream);
    INT8 ret;
    MOP_STATUS status;
    CHECK_NULL(stream);
    status = MOpDataIStreamEx_readByte(stream, &ret);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN_VALUE(ret);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEINPUTSTREAM_READSHORT) "int NativeInputStream.readShort(NativeInputStream this)"
{
    MOpDataIStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIStream);
    INT16 ret;
    MOP_STATUS status;
    CHECK_NULL(stream);
    status = MOpDataIStreamEx_readShort(stream, &ret);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN_VALUE(ret);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEINPUTSTREAM_READINT) "int NativeInputStream.readInt(NativeInputStream this)"
{
    MOpDataIStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIStream);
    INT32 ret;
    MOP_STATUS status;
    CHECK_NULL(stream);
    status = MOpDataIStreamEx_readInt(stream, &ret);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN_VALUE(ret);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEINPUTSTREAM_READLONG) "long NativeInputStream.readLong(NativeInputStream this)"
{
    MOpDataIStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIStream);
    INT64 ret;
    MOP_STATUS status;
    CHECK_NULL(stream);
    status = MOpDataIStreamEx_readLong(stream, &ret);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN_LONG(ret);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEINPUTSTREAM_READUTF8) "String NativeInputStream.readUTF8(NativeInputStream this)"
{
    MOpDataIStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIStream);
    bream_byte_t* data;
    bream_ptr_t ret;
    UINT16 len;
    bream_exception_t* e = NULL;
    UINT32 got;
    MOP_STATUS status;
    CHECK_NULL(stream);
    do {
        status = MOpDataIStreamEx_readShort(stream, (INT16*)&len);
        MOP_BREAKIF(status);
        e = bream_start_string(vm, len, &data);
        if (e != NULL)
            break;
        status = MOpDataIStream_read(stream, data, len, &got);
        if (status == MOP_STATUS_OK && got != len)
            status = MOP_STATUS_ERR;
        if (status != MOP_STATUS_OK) {
            /* Always call commit string even in error but make sure
             * the string is valid anyway */
            memset(data, ' ', len);
        }
        e = bream_commit_string(vm, &ret);
    } while (FALSE);

    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETHROW_IF(e);
    RETURN_PTR(ret);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEOUTPUTSTREAM_OPENFILEOUTPUTSTREAM) "NativeOutputStream NativeOutputStream.openFileOutputStream(String path)"
{
    CHECK_NULL(ARG(path));
    RETURN_PTR(0);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEOUTPUTSTREAM_WRITE) "void NativeOutputStream.write(NativeByteArray data, int offset, int length, NativeOutputStream this)"
{
    MOpDataIOStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIOStream);
    bream_byte_t* data;
    UINT32 ret;
    MOP_STATUS status;
    CHECK_NULL(stream);
    CHECK_BYTEARRAY(ARG(data), ARG(offset), ARG(length));
    data = bream_bytearray_get_ptr(vm, ARG(data), ARG(offset));
    status = MOpDataIOStream_write(stream, data, ARG(length), &ret);
    if (status == MOP_STATUS_OK && ret != ARG(length))
        status = MOP_STATUS_ERR;
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN();
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEOUTPUTSTREAM_CLOSE) "void NativeOutputStream.close(NativeOutputStream this)"
{
    MOpDataIOStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIOStream);
    CHECK_NULL(stream);
    stream->base.release(&stream->base, FALSE);
    RETURN();
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEOUTPUTSTREAM_WRITEBYTE) "void NativeOutputStream.writeByte(int i, NativeOutputStream this)"
{
    MOpDataIOStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIOStream);
    MOP_STATUS status;
    CHECK_NULL(stream);
    status = MOpDataIOStreamEx_writeByte(stream, (INT8)ARG(i));
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN();
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEOUTPUTSTREAM_WRITESHORT) "void NativeOutputStream.writeShort(int i, NativeOutputStream this)"
{
    MOpDataIOStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIOStream);
    MOP_STATUS status;
    CHECK_NULL(stream);
    status = MOpDataIOStreamEx_writeShort(stream, (INT16)ARG(i));
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN();
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEOUTPUTSTREAM_WRITEINT) "void NativeOutputStream.writeInt(int i, NativeOutputStream this)"
{
    MOpDataIOStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIOStream);
    MOP_STATUS status;
    CHECK_NULL(stream);
    status = MOpDataIOStreamEx_writeInt(stream, ARG(i));
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN();
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEOUTPUTSTREAM_WRITELONG) "void NativeOutputStream.writeLong(long l, NativeOutputStream this)"
{
    MOpDataIOStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIOStream);
    INT64 value;
    MOP_STATUS status;
    CHECK_NULL(stream);
    CHECK_NULL(ARG(l));
    value = bream_load_long(vm, ARG(l));
    status = MOpDataIOStreamEx_writeLong(stream, value);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN();
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEOUTPUTSTREAM_WRITEUTF8) "void NativeOutputStream.writeUTF8(String str, NativeOutputStream this)"
{
    MOpDataIOStream *stream = UNWRAP_OBJECT(ARG(this), MOpDataIOStream);
    bream_string_t value;
    MOP_STATUS status;
    CHECK_NULL(stream);
    CHECK_NULL(ARG(str));
    value = bream_load_string(vm, ARG(str));
    status = MOpDataIOStreamEx_writeUtf8(stream, (const UINT8*)value.data,
                                         value.length);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_IO_toException(status), NULL);
    RETURN();
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEDATASTORE_GETRECORDINPUTSTREAM) "NativeInputStream NativeDataStore.getRecordInputStream(Type type, String id)"
{
    MOpDataStore* ds = MOpVMInstance_getDataStore(inst);
    MOpObjectWrapper* wrap;
    bream_string_t id;
    MOpDataIStream* stream;
    MOP_STATUS status;
    bream_ptr_t ret;
    bream_exception_t *e;
    CHECK_NULL(ARG(id));
    if (ARG(type) < 0 || ARG(type) > 255)
        THROW(TYPE_BREAM_LANG_ILLEGALARGUMENTEXCEPTION, NULL);
    id = bream_load_string(vm, ARG(id));
    status = MOpDataStore_getRecordInputStream(ds, (UINT8)ARG(type), id.data, &stream);
    if (status == MOP_STATUS_NOTFOUND)
        RETURN_PTR(0);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_toException(status), NULL);
    status = MOpObjectWrapper_createInputStream(&wrap, stream);
    if (status != MOP_STATUS_OK) {
        MOpDataIStream_release(stream);
        THROW(VMHelpers_toException(status), NULL);
    }
    e = MOpObjectWrapper_wrapObject(wrap, vm, &ret);
    MOpObjectWrapper_release(wrap);
    RETHROW_IF(e);
    RETURN_PTR(ret);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEDATASTORE_GETRECORD) "NativeByteArray NativeDataStore.getRecord(Type type, String id)"
{
    MOpDataStore* ds = MOpVMInstance_getDataStore(inst);
    bream_ptr_t array;
    bream_string_t id;
    bream_byte_t* data;
    bream_exception_t* e;
    UINT32 length;
    MOP_STATUS status;
    CHECK_NULL(ARG(id));
    if (ARG(type) < 0 || ARG(type) > 255)
        THROW(TYPE_BREAM_LANG_ILLEGALARGUMENTEXCEPTION, NULL);
    id = bream_load_string(vm, ARG(id));
    length = MOpDataStore_recordValueSize(ds, (UINT8)ARG(type), id.data);
    if (length == 0)
        RETURN_PTR(0);
    e = bream_nativebytearray_allocate(vm, length, &array, &data);
    RETHROW_IF(e);
    id = bream_load_string(vm, ARG(id));
    status = MOpDataStore_getRecordEx(ds, (UINT8)ARG(type), id.data,
                                      (UINT8*)data, &length);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_toException(status), NULL);
    RETURN_PTR(array);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEDATASTORE_GETRECORDDELAYED) "boolean NativeDataStore.getRecordDelayed(Type type, String id, NativeDelayedRecordReceiver receiver)"
{
    CHECK_NULL(ARG(id));
    CHECK_NULL(ARG(receiver));
    if (ARG(type) < 0 || ARG(type) > 255)
        THROW(TYPE_BREAM_LANG_ILLEGALARGUMENTEXCEPTION, NULL);
    RETURN_VALUE(FALSE);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEDATASTORE_GETRECORDOUTPUTSTREAM) "NativeOutputStream NativeDataStore.getRecordOutputStream(Type type, String id)"
{
    MOpDataStore* ds = MOpVMInstance_getDataStore(inst);
    MOpObjectWrapper* wrap;
    bream_string_t id;
    MOpDataIOStream* stream;
    MOP_STATUS status;
    bream_ptr_t ret;
    bream_exception_t *e;
    CHECK_NULL(ARG(id));
    if (ARG(type) < 0 || ARG(type) > 255)
        THROW(TYPE_BREAM_LANG_ILLEGALARGUMENTEXCEPTION, NULL);
    id = bream_load_string(vm, ARG(id));
    status = MOpDataStore_getRecordOutputStream(ds, (UINT8)ARG(type), id.data, &stream);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_toException(status), NULL);
    status = MOpObjectWrapper_createOutputStream(&wrap, stream);
    if (status != MOP_STATUS_OK) {
        MOpDataIOStream_release(stream);
        THROW(VMHelpers_toException(status), NULL);
    }
    e = MOpObjectWrapper_wrapObject(wrap, vm, &ret);
    MOpObjectWrapper_release(wrap);
    RETHROW_IF(e);
    RETURN_PTR(ret);
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEDATASTORE_SETRECORD) "void NativeDataStore.setRecord(Type type, String id, NativeByteArray data)"
{
    MOpDataStore* ds = MOpVMInstance_getDataStore(inst);
    bream_string_t id;
    MOP_STATUS status;
    size_t length;
    UINT8* data;
    CHECK_NULL(ARG(id));
    CHECK_NULL(ARG(data));
    if (ARG(type) < 0 || ARG(type) > 255)
        THROW(TYPE_BREAM_LANG_ILLEGALARGUMENTEXCEPTION, NULL);
    id = bream_load_string(vm, ARG(id));
    length = bream_nativebytearray_size(vm, ARG(data));
    data = (UINT8*) bream_nativebytearray_get_ptr(vm, ARG(data), 0);
    status = MOpDataStore_setRecord(ds, (UINT8)ARG(type), id.data,
                                    data, length);
    if (status != MOP_STATUS_OK)
        THROW(VMHelpers_toException(status), NULL);
    RETURN();
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEDATASTORE_RECORDEXISTS) "boolean NativeDataStore.recordExists(Type type, String id)"
{
    MOpDataStore* ds = MOpVMInstance_getDataStore(inst);
    bream_string_t id;
    CHECK_NULL(ARG(id));
    if (ARG(type) < 0 || ARG(type) > 255)
        THROW(TYPE_BREAM_LANG_ILLEGALARGUMENTEXCEPTION, NULL);
    id = bream_load_string(vm, ARG(id));
    RETURN_VALUE(MOpDataStore_recordExists(ds, (UINT8)ARG(type), id.data));
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEDATASTORE_REMOVERECORD) "void NativeDataStore.removeRecord(Type type, String id)"
{
    MOpDataStore* ds = MOpVMInstance_getDataStore(inst);
    bream_string_t id;
    CHECK_NULL(ARG(id));
    if (ARG(type) < 0 || ARG(type) > 255)
        THROW(TYPE_BREAM_LANG_ILLEGALARGUMENTEXCEPTION, NULL);
    id = bream_load_string(vm, ARG(id));
    MOpDataStore_removeRecord(ds, (UINT8)ARG(type), id.data);
    RETURN();
}

BREAMAPI(NATIVE_BREAM_IO_API_NATIVEDATASTORE_REMOVEALL) "void NativeDataStore.removeAll(Type type)"
{
    MOpDataStore* ds = MOpVMInstance_getDataStore(inst);
    if (ARG(type) < 0 || ARG(type) > 255)
        THROW(TYPE_BREAM_LANG_ILLEGALARGUMENTEXCEPTION, NULL);
    MOpDataStore_removeAllRecordsForType(ds, (UINT8)ARG(type));
    RETURN();
}

BREAMAPI(NATIVE_BREAM_UTIL_NATIVESYSTEM_SETTIMER) "void NativeSystem.setTimer(int time)"
{
    MOpVMInstanceExternal_setTimer((MOpVMInstanceExternal *) inst, ARG(time));
    RETURN();
}

