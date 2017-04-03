/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2013 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>

#include "pch.h"

#include "platform.h"
#include "resolve.h"

OP_EXTERN_C_BEGIN

#include "mini/c/shared/core/pi/Resolver.h"

OP_EXTERN_C_END

/* Number of threads that resolve */
static const size_t RESOLVERS = 4;

struct _MOpResolver
{
    MOpResolverCallbackF callback;
    void *userData;

    unsigned int requests;
};

typedef struct Request Request;

struct Request
{
    Request *next;

    MOpResolver *resolver;
    char *name;

    MOpSocketAddress host;
    MOP_STATUS status;
};

typedef struct Queue
{
    Request *head;
    pthread_mutex_t mutex;
} Queue;

static Queue request_queue = { NULL, PTHREAD_MUTEX_INITIALIZER };
static pthread_cond_t request_queue_nonempty = PTHREAD_COND_INITIALIZER;
static Queue response_queue = { NULL, PTHREAD_MUTEX_INITIALIZER };

typedef struct ResolverData
{
    JavaVM *jvm;
    jint jvmVersion;
    RequestSliceCallback callback;
    void *userdata;
} ResolverData;

static MOP_STATUS MOpResolver_init(MOpResolver *resolver,
                                   MOpResolverCallbackF callback,
                                   void *userData);

MOP_STATUS MOpResolver_create(MOpResolver** resolver,
                              MOpResolverCallbackF callback, void* userData)
{
    MOP_STATUS status;
    if (!resolver)
        return MOP_STATUS_NULLPTR;
    *resolver = (MOpResolver *)calloc(1, sizeof(MOpResolver));
    if (!*resolver)
        return MOP_STATUS_NOMEM;
    status = MOpResolver_init(*resolver, callback, userData);
    if (status != MOP_STATUS_OK)
    {
        MOpResolver_release(*resolver);
        *resolver = NULL;
    }
    return status;
}

MOP_STATUS MOpResolver_init(MOpResolver *resolver,
                            MOpResolverCallbackF callback,
                            void *userData)
{
    if (!callback)
        return MOP_STATUS_NULLPTR;
    resolver->callback = callback;
    resolver->userData = userData;
    return MOP_STATUS_OK;
}

void MOpResolver_release(MOpResolver* resolver)
{
    if (!resolver)
        return;

    resolver->callback = NULL;

    if (resolver->requests == 0) {
        free(resolver);
    }
}

MOP_STATUS MOpResolver_getHostByName(MOpResolver* resolver, const TCHAR* name)
{
    Request* request;
    if (!resolver || !name)
        return MOP_STATUS_NULLPTR;

    request = (Request *)calloc(1, sizeof(Request));
    if (!request)
        return MOP_STATUS_NOMEM;

    request->resolver = resolver;
    request->name = strdup(name);
    if (!request->name)
    {
        free(request);
        return MOP_STATUS_NOMEM;
    }

    resolver->requests++;

    pthread_mutex_lock(&request_queue.mutex);
    request->next = request_queue.head;
    request_queue.head = request;
    if (!request->next)
        pthread_cond_broadcast(&request_queue_nonempty);
    pthread_mutex_unlock(&request_queue.mutex);

    return MOP_STATUS_OK;
}

static void *resolver(void *args);

MOP_STATUS Resolve_setup(RequestSliceCallback requestSlice, void *userdata)
{
    JNIEnv *env;
    pthread_attr_t attr;
    ResolverData *data;
    size_t i;

    data = (ResolverData *)malloc(sizeof(ResolverData));
    if (!data)
    {
        return MOP_STATUS_NOMEM;
    }
    env = getEnv();
    data->jvmVersion = env->GetVersion();
    if (env->GetJavaVM(&data->jvm))
    {
        free(data);
        return MOP_STATUS_ERR;
    }
    data->callback = requestSlice;
    data->userdata = userdata;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (i = 0; i < RESOLVERS; i++)
    {
        pthread_t thread;
        if (pthread_create(&thread, &attr, resolver, data))
        {
            if (i > 0)
            {
                /* If we could create at least one worker, it's ok */
                break;
            }
            pthread_attr_destroy(&attr);
            free(data);
            return MOP_STATUS_ERR;
        }
    }

    pthread_attr_destroy(&attr);
    return MOP_STATUS_OK;
}

MOP_STATUS Resolve_slice(void)
{
    Request *request;

    pthread_mutex_lock(&response_queue.mutex);
    request = response_queue.head;
    response_queue.head = NULL;
    pthread_mutex_unlock(&response_queue.mutex);

    while (request)
    {
        Request *next = request->next;
        MOpResolver *resolver = request->resolver;

        if (resolver->callback)
            resolver->callback(resolver, request->host, request->status,
                               resolver->userData);
        if (--resolver->requests == 0 && !resolver->callback)
        {
            free(resolver);
        }
        free(request);
        request = next;
    }

    return MOP_STATUS_OK;
}

static void *resolver(void *args)
{
    ResolverData *data = (ResolverData *)args;
    struct addrinfo hints;

    /* Attach this thread to the JVM */
    JNIEnv *env;
    struct JavaVMAttachArgs attachArgs;
    memset(&attachArgs, 0, sizeof(attachArgs));
    attachArgs.version = data->jvmVersion;
    attachArgs.name = "ResolverThread";
    if (data->jvm->AttachCurrentThread(&env, &attachArgs) != 0)
        return NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_protocol = 0;

    for (;;)
    {
        Request *request;
        struct addrinfo *result;
        int err;
        pthread_mutex_lock(&request_queue.mutex);
        for (;;)
        {
            request = request_queue.head;
            if (request)
            {
                request_queue.head = request->next;
                request->next = NULL;
                break;
            }
            pthread_cond_wait(&request_queue_nonempty, &request_queue.mutex);
        }
        pthread_mutex_unlock(&request_queue.mutex);

        err = getaddrinfo(request->name, NULL, &hints, &result);
        MOpDebug("Resolved %s: err=%d (%s)", request->name, err, gai_strerror(err));
        if (err == 0)
        {
            struct addrinfo *addr;
            request->host.type = MOP_SA_UNKNOWN;
            for (addr = result;
                 request->host.type == MOP_SA_UNKNOWN && addr;
                 addr = addr->ai_next)
            {
                switch (addr->ai_addr->sa_family)
                {
                case AF_INET:
                {
                    struct sockaddr_in *addr_in =
                            (struct sockaddr_in *)addr->ai_addr;
                    request->host.type = MOP_SA_INET4;
                    request->host.address.inet4 = addr_in->sin_addr.s_addr;
                    break;
                }
                case AF_INET6:
                {
                    struct sockaddr_in6 *addr_in =
                            (struct sockaddr_in6 *)addr->ai_addr;
                    request->host.type = MOP_SA_INET6;
                    request->host.address.inet6.scope_id =
                            addr_in->sin6_scope_id;
                    memcpy(request->host.address.inet6.in_addr,
                           addr_in->sin6_addr.s6_addr, 16);
                    break;
                }
                default:
                    break;
                }
            }
            freeaddrinfo(result);
            request->status = request->host.type != MOP_SA_UNKNOWN ?
                    MOP_STATUS_OK : MOP_STATUS_ERR;
        }
        else
        {
            switch (err)
            {
            case EAI_MEMORY:
                request->status = MOP_STATUS_NOMEM;
                break;
            default:
                request->status = MOP_STATUS_ERR;
                break;
            }
        }

        pthread_mutex_lock(&response_queue.mutex);
        request->next = response_queue.head;
        response_queue.head = request;
        pthread_mutex_unlock(&response_queue.mutex);

        data->callback(env, data->userdata);
    }

    data->jvm->DetachCurrentThread();
    return NULL;
}

