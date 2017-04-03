/* -*- Mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2013 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include "jni_obfuscation.h"
#include "com_opera_android_browser_obml_Platform.h"

extern "C" {

#include "core/pi/Reachability.h"

}

struct _MOpReachability {} static reachabilityInstance;

typedef struct _Callback Callback;
struct _Callback
{
    Callback *next;
    MOpReachabilityCallback cb;
    void *userData;
};

static MOpReachabilityStatus status = MOP_REACHABILITY_UNKNOWN;
static Callback *firstCallback;


MOP_STATUS MOpReachability_create(MOpReachability **reachability)
{
    *reachability = &reachabilityInstance;
    return MOP_STATUS_OK;
}

void MOpReachability_release(const void *reachability UNUSED)
{
}

MOpReachability *MOpReachability_get()
{
    return &reachabilityInstance;
}

MOP_STATUS MOpReachability_getStatus(MOpReachability *_this UNUSED,
                                     MOpReachabilityCallback cb, void *userData)
{
    cb(userData, status);
    return MOP_STATUS_OK;
}

MOP_STATUS MOpReachability_monitorStatus(MOpReachability *_this UNUSED,
                                         MOpReachabilityCallback cb, void *userData)
{
    Callback *callback = (Callback*)malloc(sizeof(Callback));
    if (!callback)
    {
        return MOP_STATUS_NOMEM;
    }
    callback->cb = cb;
    callback->userData = userData;
    callback->next = firstCallback;
    firstCallback = callback;
    return MOP_STATUS_OK;
}

void MOpReachability_cancel(MOpReachability *_this UNUSED,
                            MOpReachabilityCallback cb, void *userData)
{
    Callback **pprev = &firstCallback;
    for (;;)
    {
        Callback *callback = *pprev;
        if (!callback)
        {
            return;
        }
        if (callback->cb == cb && callback->userData == userData)
        {
            *pprev = callback->next;
            free(callback);
            return;
        }
        pprev = &callback->next;
    }
}

MOP_STATUS MOpReachability_getLastError(const MOpReachability *_this UNUSED)
{
    return status == MOP_REACHABILITY_ERROR ? MOP_STATUS_ERR : MOP_STATUS_OK;
}

MOP_STATUS MOpReachability_requestConnection(MOpReachability *_this UNUSED)
{
    return MOP_STATUS_OK;
}

void Java_com_opera_android_browser_obml_Platform_connectivityChanged(JNIEnv *, jclass,
        jboolean success, jboolean networkAvailable, jboolean connected, jboolean mobile)
{
    MOpReachabilityStatus newStatus =
            success ?
            (networkAvailable ?
             (connected ?
              (mobile ? MOP_REACHABILITY_REACHABLE_CELL : MOP_REACHABILITY_REACHABLE_WIFI) :
              MOP_REACHABILITY_NO_CONNECTION) :
             MOP_REACHABILITY_NO_NETWORK) :
            MOP_REACHABILITY_ERROR;
    if (status != newStatus)
    {
        status = newStatus;
        for (Callback *callback = firstCallback; callback; callback = callback->next)
        {
            callback->cb(callback->userData, status);
        }
    }
}
