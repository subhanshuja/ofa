/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "platform.h"
#include "obmlview.h"
#include "workerthread.h"

#include "bream/vm/c/inc/bream_host.h"
#include "bream/vm/c/inc/bream_const.h"

OP_EXTERN_C_BEGIN

#include "bream/vm/c/graphics/thread.h"
#include "mini/c/shared/generic/pi/WorkerThread.h"

OP_EXTERN_C_END

struct _MOpWorkerThread
{
    void *userdata;
    ThreadMutex* mutex;
    ThreadCondition* condition;
    WorkerThread_iterate iterate;

    JavaVM* jvm;
    jint jvm_version;
    MOpWorkerThread_Priority prio;
    BOOL started;
    pthread_t thread;
};

static void* run(void* args);

typedef struct InMainNode InMainNode;

struct InMainNode
{
    void (* callback)(void* userdata);
    void* userdata;
    InMainNode* next;
};

static ThreadMutex inmain_mutex = THREAD_STATIC_MUTEX_INIT;
static InMainNode* inmain_queue = NULL;

static MOP_STATUS MOpWorkerThread_init(MOpWorkerThread* thread,
                                       void *userdata,
                                       ThreadMutex* mutex,
                                       ThreadCondition* condition,
                                       WorkerThread_iterate iterate)
{
    if (!mutex || !condition || !iterate)
        return MOP_STATUS_NULLPTR;

    thread->userdata = userdata;
    thread->mutex = mutex;
    thread->condition = condition;
    thread->iterate = iterate;

    JNIEnv *env = getEnv();
    thread->jvm_version = env->GetVersion();
    if (env->GetJavaVM(&thread->jvm))
        return MOP_STATUS_ERR;
    return MOP_STATUS_OK;
}

MOP_STATUS MOpWorkerThread_create(MOpWorkerThread** thread,
                                  void *userdata,
                                  ThreadMutex* mutex,
                                  ThreadCondition* condition,
                                  WorkerThread_iterate iterate)
{
    MOP_STATUS status;

    if (!thread)
        return MOP_STATUS_NULLPTR;

    *thread = (MOpWorkerThread *) mop_calloc(1, sizeof(MOpWorkerThread));
    if (!*thread)
        return MOP_STATUS_NOMEM;

    status = MOpWorkerThread_init(*thread, userdata, mutex, condition, iterate);
    if (status != MOP_STATUS_OK)
    {
        MOpWorkerThread_release(*thread);
        *thread = NULL;
    }
    return status;
}

void MOpWorkerThread_release(MOpWorkerThread* thread)
{
    if (thread)
    {
        if (thread->started)
        {
            pthread_join(thread->thread, NULL);
        }
        mop_free(thread);
    }
}

BOOL MOpWorkerThread_start(MOpWorkerThread* thread,
                           MOpWorkerThread_Priority priority)
{
    pthread_attr_t attr;

    if (thread->started)
    {
        MOP_ASSERT(FALSE);
        return FALSE;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    thread->prio = priority;

    if (pthread_create(&thread->thread, &attr, run, thread))
    {
        pthread_attr_destroy(&attr);
        return FALSE;
    }
    thread->started = TRUE;
    pthread_attr_destroy(&attr);
    return TRUE;
}

BOOL MOpWorkerThread_join(MOpWorkerThread* thread)
{
    BOOL ret = FALSE;
    if (!thread->started)
    {
        MOP_ASSERT(FALSE);
    }
    else
    {
        ret = pthread_join(thread->thread, NULL) != 0;
        thread->started = FALSE;
    }
    return ret;
}

void MOpWorkerThread_inMain()
{
    InMainNode* node = NULL, * start;
    if (ThreadMutex_lock(&inmain_mutex))
    {
        node = inmain_queue;
        inmain_queue = NULL;
        ThreadMutex_unlock(&inmain_mutex);
    }

    /* Reverse the list */
    start = NULL;
    while (node)
    {
        InMainNode* next = node->next;
        node->next = start;
        start = node;
        node = next;
    }

    /* Call each callback */
    while (start)
    {
        InMainNode* next = start->next;
        start->callback(start->userdata);
        delete start;
        start = next;
    }
}

void MOpWorkerThread_callInMain(void (* callback)(void* userdata),
                                void* userdata)
{
    BOOL request = FALSE;
    InMainNode* node = new InMainNode();
    node->callback = callback;
    node->userdata = userdata;

    if (ThreadMutex_lock(&inmain_mutex))
    {
        node->next = inmain_queue;
        request = inmain_queue == NULL;
        inmain_queue = node;
        ThreadMutex_unlock(&inmain_mutex);
    }
    else
    {
        delete node;
    }

    if (request)
        ObmlView_requestCallInMain();
}

static void iterate(MOpWorkerThread* thread, JNIEnv* env UNUSED)
{
    if (!ThreadMutex_lock(thread->mutex))
    {
        return;
    }
    for (;;)
    {
        switch (thread->iterate(thread->userdata))
        {
        case WORKERTHREAD_ITERATE_ABORT:
            return;
        case WORKERTHREAD_ITERATE_DONE:
            ThreadMutex_unlock(thread->mutex);
            return;
        case WORKERTHREAD_ITERATE_CONTINUE:
            break;
        case WORKERTHREAD_ITERATE_IDLE:
            if (!ThreadCondition_wait(thread->condition, thread->mutex))
            {
                return;
            }
            break;
        }
    }
}

void* run(void* args)
{
    MOpWorkerThread* thread = (MOpWorkerThread*)args;

    int nice = 0;
    switch (thread->prio)
    {
    case MOP_WORKERTHREAD_LOW_PRIORITY:
        nice = 10; /* Process.THREAD_PRIORITY_BACKGROUND */
        break;
    case MOP_WORKERTHREAD_NORMAL_PRIORITY:
        nice = 0; /* Process.THREAD_PRIORITY_DEFAULT */
        break;
    case MOP_WORKERTHREAD_HIGH_PRIORITY:
        nice = -19; /* Process.THREAD_PRIORITY_URGENT_AUDIO */
        break;
    }
    if (nice != 0)
    {
        pid_t tid = syscall(__NR_gettid);
        if (setpriority(PRIO_PROCESS, tid, nice) != 0)
        {
            /* LOG_ERROR("Unable to modify thread scheduling policy."); */
        }
    }

    /* Attach this thread to the JVM */
    struct JavaVMAttachArgs attachArgs;
    memset(&attachArgs, 0, sizeof(attachArgs));
    attachArgs.version = thread->jvm_version;
    attachArgs.name = "WorkerThread";
    JNIEnv* env;
    if (thread->jvm->AttachCurrentThread(&env, &attachArgs) != 0)
        return NULL;

    iterate(thread, env);

    thread->jvm->DetachCurrentThread();
    return NULL;
}
