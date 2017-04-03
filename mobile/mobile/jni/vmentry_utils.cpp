/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <jni.h>
#include "jni_obfuscation.h"
#include "jniutils.h"

#include "bream/vm/c/inc/bream_const.h"
#include "bream/vm/c/inc/bream_host.h"
#include "com_opera_android_bream_VMEntry.h"
#include "bream.h"
#include "font.h"
#include "platform.h"
#include "vmentry_utils.h"
#include "vminstance_ext.h"

extern "C" {

#include "mini/c/shared/generic/vm_integ/ObjectWrapper.h"

}

static struct ExceptionMapEntry {
    const char* className;
    int breamException;
    jclass klass;
} exceptions[] = {
    {"java/lang/StackOverflowError",
     TYPE_BREAM_LANG_STACKOVERFLOWEXCEPTION, NULL},
    {"java/lang/AssertionError",
     TYPE_BREAM_LANG_ASSERTIONERROR, NULL},
    {"java/lang/ArithmeticException",
     TYPE_BREAM_LANG_ARITHMETICEXCEPTION, NULL},
    {"java/lang/ArrayIndexOutOfBoundsException",
     TYPE_BREAM_LANG_ARRAYINDEXOUTOFBOUNDSEXCEPTION, NULL},
    {"java/lang/NullPointerException",
     TYPE_BREAM_LANG_NULLPOINTEREXCEPTION, NULL},
    {"java/lang/IllegalArgumentException",
     TYPE_BREAM_LANG_ILLEGALARGUMENTEXCEPTION, NULL},
    {"java/lang/ClassCastException",
     TYPE_BREAM_LANG_CLASSCASTEXCEPTION, NULL},
    {"java/lang/OutOfMemoryError",
     TYPE_BREAM_LANG_OUTOFMEMORYEXCEPTION, NULL},
    {"java/lang/StringIndexOutOfBoundsException",
     TYPE_BREAM_LANG_STRINGINDEXOUTOFBOUNDSEXCEPTION, NULL},
    {"java/lang/IndexOutOfBoundsException",
     TYPE_BREAM_LANG_INDEXOUTOFBOUNDSEXCEPTION, NULL},
    {"java/io/EOFException",
     TYPE_BREAM_IO_EOFEXCEPTION, NULL},
    {"java/io/UTFDataFormatException",
     TYPE_BREAM_LANG_UTFDATAFORMATEXCEPTION, NULL},
    {"java/io/IOException",
     TYPE_BREAM_IO_IOEXCEPTION, NULL},
    // Note: The last entry is used as a fallback for unknown Bream exceptions
    {"java/lang/Throwable",
     TYPE_BREAM_LANG_THROWABLE, NULL},
};

static jclass stringClass;
// Used to pass Java exception message to Bream exception
static jmethodID id_getMessage;
static const size_t MAX_MSG = 128;
static char exception_message[MAX_MSG];

// Used for MOpObjectWrapper
static jfieldID id_handle;
static jfieldID id_wrapper_handle;
static jmethodID id_nativeInit;

// Used for Bream.Handle
static jfieldID id_handle_ptr;

void Java_com_opera_android_bream_VMEntry_nativeInit(JNIEnv *env, jobject o)
{
    scoped_localref<jclass> clazz(env, env->GetObjectClass(o));
    id_handle = env->GetFieldID(*clazz, F_com_opera_android_bream_VMEntry_handle_ID);

    clazz = scoped_localref<jclass>(env, env->FindClass("java/lang/String"));
    stringClass = (jclass)env->NewGlobalRef(*clazz);

    clazz = scoped_localref<jclass>(env, env->FindClass("java/lang/Throwable"));
    id_getMessage = env->GetMethodID(*clazz, "getMessage", "()Ljava/lang/String;");

    clazz = scoped_localref<jclass>(
        env, env->FindClass(C_com_opera_android_bream_VMInvokes$Wrapper));
    id_wrapper_handle = env->GetFieldID(*clazz, F_com_opera_android_bream_VMInvokes$Wrapper_handle_ID);
    id_nativeInit = env->GetMethodID(*clazz, M_com_opera_android_bream_VMInvokes$Wrapper_nativeInit_ID);

    clazz = scoped_localref<jclass>(
        env, env->FindClass(C_com_opera_android_bream_Bream$Handle));
    id_handle_ptr = env->GetFieldID(*clazz, F_com_opera_android_bream_Bream$Handle_mPtr_ID);
}

bream_exception_t *wrap_string(JNIEnv *env, bream_t *vm, jstring jstr, bream_ref_t ref)
{
    bream_string_t str;
    if (jstr == NULL)
    {
        *ref = 0;
        return NULL;
    }
    jnistring s = GetStringUTF8Chars(env, jstr);
    str.length = strlen(s.string());
    str.data = s.string();
    return bream_wrap_string(vm, str, ref);
}

bream_exception_t *wrap_intarray(JNIEnv *env, bream_t *vm, jintArray array, bream_ref_t ref)
{
    bream_exception_t *e;
    int *dst;
    jint *src;
    jint len;
    if (array == NULL)
    {
        *ref = 0;
        return NULL;
    }
    len = env->GetArrayLength(array);
    e = bream_intarray_new(vm, len, ref, &dst);
    if (e)
    {
        return e;
    }
    src = (jint*)env->GetPrimitiveArrayCritical(array, NULL);
    memcpy(dst, src, len * sizeof(int));
    env->ReleasePrimitiveArrayCritical(array, src, JNI_ABORT);
    return e;
}

bream_exception_t *wrap_bytearray(JNIEnv *env, bream_t *vm, jbyteArray array, bream_ref_t ref)
{
    bream_exception_t *e;
    bream_byte_t *dst;
    jbyte *src;
    jint len;
    if (array == NULL)
    {
        *ref = 0;
        return NULL;
    }
    len = env->GetArrayLength(array);
    e = bream_bytearray_new(vm, len, ref, &dst);
    if (e)
    {
        return e;
    }
    src = (jbyte*)env->GetPrimitiveArrayCritical(array, NULL);
    memcpy(dst, src, len);
    env->ReleasePrimitiveArrayCritical(array, src, JNI_ABORT);
    return e;
}

bream_exception_t *wrap_stringarray(JNIEnv *env, bream_t *vm, jobjectArray array, bream_ref_t ref)
{
    bream_exception_t* e;
    jint idx, len;
    if (array == NULL)
    {
        *ref = 0;
        return NULL;
    }
    len = env->GetArrayLength(array);
    e = bream_array_new(vm, len, ref, NULL);
    if (e)
    {
        return e;
    }
    for (idx = 0; idx < len; idx++)
    {
        bream_ptr_t value;
        bream_string_t str;
        scoped_localref<jstring> jstr(
            env, (jstring)env->GetObjectArrayElement(array, idx));
        if (!jstr)
        {
            continue;
        }
        jnistring s = GetAndReleaseStringUTF8Chars(jstr);
        str.length = strlen(s.string());
        str.data = s.string();
        e = bream_wrap_string(vm, str, &value);
        if (e)
        {
            return e;
        }
        bream_array_set(vm, *ref, value, idx);
    }
    return e;
}

bream_exception_t *wrap_bytearrayarray(JNIEnv *env, bream_t *vm, jobjectArray array, bream_ref_t ref)
{
    bream_exception_t* e;
    jint idx, len;
    if (array == NULL)
    {
        *ref = 0;
        return NULL;
    }
    len = env->GetArrayLength(array);
    e = bream_array_new(vm, len, ref, NULL);
    if (e)
    {
        return e;
    }
    for (idx = 0; idx < len; idx++)
    {
        bream_ptr_t value;
        scoped_localref<jbyteArray> jarray(
            env, (jbyteArray) env->GetObjectArrayElement(array, idx));
        if (!jarray)
        {
            continue;
        }
        e = wrap_bytearray(env, vm, *jarray, &value);
        if (e)
        {
            return e;
        }
        bream_array_set(vm, *ref, value, idx);
    }
    return e;
}

void releaseWrappedObject(void* obj)
{
    JNIEnv* env = getEnv();
    env->SetLongField((jobject)obj, id_wrapper_handle, 0);
    env->DeleteGlobalRef((jobject)obj);
}

MOpObjectWrapper* get_object_wrapper(JNIEnv* env, jobject obj)
{
    if (!obj) return NULL;
    MOpObjectWrapper *wrap = (MOpObjectWrapper*)env->GetLongField(obj, id_wrapper_handle);
    // Object wrappers start out with a refcount of one. This reference is
    // *transferred* to our first caller. If that reference is released before
    // we get here again, we'll simply create a new objectwrapper. But if the
    // reference is still alive, we can't just transfer it to another caller
    // without an addRef.
    if (wrap)
    {
        MOpObjectWrapper_addRef(wrap);
        return wrap;
    }

    jlong handle = env->CallLongMethod(obj, id_nativeInit);
    env->SetLongField(obj, id_wrapper_handle, handle);
    return (MOpObjectWrapper*)handle;
}

bream_exception_t *wrap_object(JNIEnv *env, bream_t *vm, jobject obj, bream_ref_t ref)
{
    if (obj == NULL)
    {
        *ref = 0;
        return NULL;
    }
    MOpObjectWrapper *wrap = get_object_wrapper(env, obj);
    bream_exception_t* e = MOpObjectWrapper_wrapObject(wrap, vm, ref);
    MOpObjectWrapper_release(wrap);
    return e;
}

bream_exception_t *wrap_font(JNIEnv *env UNUSED, bream_t *vm UNUSED, jobject obj NDEBUG_UNUSED, bream_ref_t ref)
{
    // We don't have a good api for getting a font from the jobject. We could
    // just iterate all fonts and find one that points to the same object? Or
    // maybe try to find an "equivalent" one or same style with closest style.
    // Luckily, I think we never use this.
    BREAM_ASSERT(!obj);
    *ref = 0;
    return NULL;
}

jstring load_string(JNIEnv *env, bream_t *vm, bream_ptr_t ptr)
{
    bream_string_t str;
    if (ptr == 0)
    {
        return NULL;
    }
    str = bream_load_string(vm, ptr);
    return NewStringUTF8(env, str.data).pass();
}

jintArray load_intarray(JNIEnv *env, bream_t *vm, bream_ptr_t ptr)
{
    int size;
    jintArray ret;
    jint *data;
    if (ptr == 0)
    {
        return NULL;
    }
    size = bream_intarray_size(vm, ptr);
    ret = env->NewIntArray(size);
    data = (jint*)env->GetPrimitiveArrayCritical(ret, NULL);
    memcpy(data, bream_intarray_get_ptr(vm, ptr, 0), size * sizeof(int));
    env->ReleasePrimitiveArrayCritical(ret, data, 0);
    return ret;
}

jbyteArray load_bytearray(JNIEnv *env, bream_t *vm, bream_ptr_t ptr)
{
    int size;
    jbyteArray ret;
    jbyte *data;
    if (ptr == 0)
    {
        return NULL;
    }
    size = bream_bytearray_size(vm, ptr);
    ret = env->NewByteArray(size);
    data = (jbyte*)env->GetPrimitiveArrayCritical(ret, NULL);
    memcpy(data, bream_bytearray_get_ptr(vm, ptr, 0), size);
    env->ReleasePrimitiveArrayCritical(ret, data, 0);
    return ret;
}

jobjectArray load_stringarray(JNIEnv *env, bream_t *vm, bream_ptr_t ptr)
{
    jobjectArray ret;
    bream_ptr_t *data;
    size_t idx, size;
    if (ptr == 0)
    {
        return NULL;
    }
    size = bream_array_size(vm, ptr);
    data = bream_array_get_ptr(vm, ptr, 0);
    ret = env->NewObjectArray(size, stringClass, NULL);
    for (idx = 0; idx < size; idx++)
    {
        bream_string_t bstr;
        bream_ptr_t str = *data++;
        if (str == 0)
        {
            continue;
        }
        bstr = bream_load_string(vm, str);
        scoped_localref<jstring> jstr = NewStringUTF8(env, bstr.data);
        env->SetObjectArrayElement(ret, idx, *jstr);
    }
    return ret;
}

jobject load_object(JNIEnv *env UNUSED, bream_t *vm, bream_ptr_t ptr)
{
    MOpObjectWrapper* wrapper = (MOpObjectWrapper*)bream_load_object(vm, ptr);
    if (!wrapper)
    {
        return NULL;
    }
    return (jobject)MOpObjectWrapper_get(wrapper);
}

jobject load_font(JNIEnv *env UNUSED, bream_t *vm UNUSED, bream_ptr_t ptr)
{
    MOpObjectWrapper* wrapper = (MOpObjectWrapper*)bream_load_object(vm, ptr);
    MOpFont* mopfont = (MOpFont*)MOpObjectWrapper_get(wrapper);
    return mopfont ? AndroidFont::GetFont(mopfont)->paint() : NULL;
}

bream_t *VMEntry_getVM(JNIEnv *, jobject)
{
    return g_bream_vm;
}

static jclass getExceptionClass(JNIEnv* env, ExceptionMapEntry* entry)
{
    if (!entry->klass)
    {
        scoped_localref<jclass> klass(env, env->FindClass(entry->className));
        if (!klass)
        {
            return NULL;
        }
        entry->klass = (jclass)env->NewGlobalRef(*klass);
    }
    return entry->klass;
}

static jclass findException(JNIEnv* env, bream_t* vm, bream_exception_t* e)
{
    const size_t count = sizeof(exceptions) / sizeof(exceptions[0]);
    bream_ptr_t object = bream_get_exception_object(vm, e);
    bream_type_ptr_t type;
    if (object)
    {
        type = bream_get_object_type(vm, object);
    }
    else
    {
        type = bream_get_type_ptr(vm, bream_get_exception_type(vm, e));
    }

    for (size_t i = 0; i < count; i++)
    {
        bream_type_ptr_t base = bream_get_type_ptr(vm, exceptions[i].breamException);
        if (bream_is_subtype_of(vm, type, base))
        {
            return getExceptionClass(env, &exceptions[i]);
        }
    }
    // Every exception should be a Throwable. This case is not supposed to be
    // possible.
    BREAM_ASSERT(!"Bream exception was not a Throwable");
    return getExceptionClass(env, &exceptions[count - 1]);
}

static char* getExceptionMessage(JNIEnv* env UNUSED, bream_t* vm, bream_exception_t* e)
{
    bream_ptr_t object = bream_get_exception_object(vm, e);
    if (object)
    {
        char* msg = NULL;
        bream_ptr_t ptr;
        ptr = bream_get_field_ptr(vm, object, NATIVE_BREAM_LANG_THROWABLE_MSG);
        if (ptr)
        {
            bream_string_t str = bream_load_string(vm, ptr);
            msg = strdup(str.data);
        }
        ptr = bream_get_field_ptr(vm, object, NATIVE_BREAM_LANG_THROWABLE_ST);
        if (ptr)
        {
            bream_string_t str = bream_load_string(vm, ptr);
            if (msg)
            {
                size_t len = strlen(msg);
                char* ret = (char*)malloc(len + 2 + str.length + 2);
                memcpy(ret, msg, len);
                free(msg);
                memcpy(ret + len, " (", 2);
                memcpy(ret + len + 2, str.data, str.length);
                memcpy(ret + len + 2 + str.length, ")", 2);
                msg = ret;
            }
            else
            {
                msg = (char*)malloc(1 + str.length + 2);
                msg[0] = '(';
                memcpy(msg + 1, str.data, str.length);
                memcpy(msg + 1 + str.length, ")", 2);
            }
        }
        return msg;
    }
    else
    {
        const char* msg = bream_get_exception_msg(vm, e);
        return msg ? strdup(msg) : NULL;
    }
}

void throw_exception(JNIEnv *env, bream_t *vm, bream_exception_t *e)
{
    jclass exception_class;
    exception_class = findException(env, vm, e);
    // If exception_class is NULL, findException caused another exception.
    // Leave that one thrown instead of whatever we were trying to throw.
    if (exception_class)
    {
        char* exception_msg = getExceptionMessage(env, vm, e);
        if (env->ThrowNew(exception_class, exception_msg) < 0)
        {
            BREAM_ASSERT(!"Unable to throw exception");
        }
        free(exception_msg);
    }
}

void throw_java_exception(JNIEnv *env, const char *clazz, const char *message)
{
    jclass exception_class = NULL;
    const size_t count = sizeof(exceptions) / sizeof(exceptions[0]);
    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(exceptions[i].className, clazz) == 0)
        {
            exception_class = getExceptionClass(env, &exceptions[i]);
            break;
        }
    }
    if (!exception_class)
    {
        exception_class = getExceptionClass(env, &exceptions[count - 1]);
    }
    if (env->ThrowNew(exception_class, message) < 0)
    {
        BREAM_ASSERT(!"Unable to throw exception");
    }
}

bool check_exception(JNIEnv* env, bream_t *vm)
{
    if (env->ExceptionCheck())
    {
        size_t i;
        jthrowable throwable = env->ExceptionOccurred();
#ifndef NDEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
        for (i = 0; i < sizeof(exceptions) / sizeof(exceptions[0]); i++)
        {
            if (!exceptions[i].klass)
            {
                scoped_localref<jclass> klass(
                    env, env->FindClass(exceptions[i].className));
                if (!klass)
                {
                    env->ExceptionClear();
                    continue;
                }
                exceptions[i].klass = (jclass)env->NewGlobalRef(*klass);
            }
            if (env->IsInstanceOf(throwable, exceptions[i].klass))
            {
                const char* bream_message = exceptions[i].className;

                // Extract the message from Java exception.
                scoped_localref<jstring> java_message(env,
                    (jstring)env->CallObjectMethod(throwable, id_getMessage));
                if (env->ExceptionCheck())
                {
                    // Will have to live without message
                    env->ExceptionClear();
                }
                else if (java_message)
                {
                    jnistring s = GetStringUTF8Chars(env, java_message.get());
                    size_t msg_length = MIN(MAX_MSG - 1, strlen(s.string()));
                    memcpy(exception_message, s.string(), msg_length);
                    exception_message[msg_length] = 0;
                    bream_message = exception_message;
                }
                // The message passed to bream_throw_new must live until
                // Bream is done with it, which is a point in time unknown
                // to us. Therefore we only pass it pointer to static data.
                bream_throw_new(vm, exceptions[i].breamException,
                                bream_message);
                break;
            }
        }
        return false;
    }
    return true;
}

bream_handle_t *get_object_handle(JNIEnv *env, jobject obj)
{
    return (bream_handle_t*)env->GetLongField(obj, id_handle_ptr);
}
