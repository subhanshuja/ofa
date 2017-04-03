/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "pch.h"

#include "socket.h"
#include "platform.h"

OP_EXTERN_C_BEGIN

#include "core/pi/Reachability.h"

#include "mini/c/shared/core/pi/Socket.h"

OP_EXTERN_C_END

#ifndef EPOLLONESHOT
# define EPOLLONESHOT 0x40000000
#endif

static const unsigned int EV_READ  = 0x01;
static const unsigned int EV_WRITE = 0x02;
static const unsigned int EV_CLOSE = 0x04;
static const unsigned int EV_ERR   = 0x08;

typedef struct _Event
{
    MOpSocket *socket;
    unsigned int events;
} Event;

struct _MOpSocket
{
    int fd;
    MOpSocketCallbackF callback;
    void *userData;
    BOOL waitConnect, waitWrite;

    int deleteMode;

    Event *event;
};

static MOP_STATUS MOpSocket_init(MOpSocket *sock, MOpSocketType type,
                                 MOpSocketCallbackF callback, void *userData);

static MOP_STATUS Socket_add(MOpSocket *sock,
                             BOOL watch_read, BOOL watch_write);
static MOP_STATUS Socket_chg(MOpSocket *sock,
                             BOOL watch_read, BOOL watch_write);
static MOP_STATUS Socket_del(MOpSocket *sock);

static void OnReachabilityChanged(void*, MOpReachabilityStatus);

static int epoll_fd = -1;
static size_t queue_fill, queue_size, queue_callback_fill;
static Event **queue_poll, *queue_callback;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_wait = PTHREAD_COND_INITIALIZER;

MOP_STATUS MOpSocket_create(MOpSocket* *sock, MOpSocketType type,
                            MOpSocketCallbackF callback, void *userData)
{
    MOP_STATUS status;
    if (!sock)
        return MOP_STATUS_NULLPTR;
    *sock = (MOpSocket *)calloc(1, sizeof(MOpSocket));
    if (!*sock)
        return MOP_STATUS_NOMEM;
    status = MOpSocket_init(*sock, type, callback, userData);
    if (status != MOP_STATUS_OK)
    {
        MOpSocket_release(*sock);
        *sock = NULL;
    }
    return status;
}

MOP_STATUS MOpSocket_init(MOpSocket *sock, MOpSocketType type,
                          MOpSocketCallbackF callback, void *userData)
{
    if (callback == NULL)
        return MOP_STATUS_NULLPTR;
    /* Only support TCP */
    if (type != MOP_ST_TCP)
        return MOP_STATUS_ERR;

    sock->fd = -1;
    sock->callback = callback;
    sock->userData = userData;

    return MOP_STATUS_OK;
}

void MOpSocket_release(MOpSocket *sock)
{
    if (sock)
    {
        MOpSocket_close(sock);

        if (sock->deleteMode == 0)
        {
            free(sock);
        }
        else
        {
            sock->deleteMode = 2;
        }
    }
}

MOP_STATUS MOpSocket_setCallback(MOpSocket *_this, MOpSocketCallbackF callback, void *userData)
{
    if (_this == NULL || callback == NULL)
        return MOP_STATUS_NULLPTR;

    _this->callback = callback;
    _this->userData = userData;
    return MOP_STATUS_OK;
}

static int make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    if (!(flags & O_NONBLOCK))
    {
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
            return -1;
    }
    return 0;
}

MOP_STATUS MOpSocket_connect(MOpSocket *_this, MOpSocketAddress address)
{
    struct sockaddr_storage addr;
    socklen_t addr_len;

    if (address.type == MOP_SA_UNKNOWN)
    {
        MOP_ASSERT (!"Attempt to connect to unspecified socket address.");
        return MOP_STATUS_ERR;
    }

#ifdef MOP_HAVE_DEBUG_EX
    {
        char buffer[INET6_ADDRSTRLEN] = {0};
        const char *str = NULL;

        switch (address.type)
        {
        case MOP_SA_INET4:
            str = inet_ntop(AF_INET, &address.address.inet4,
                            buffer, sizeof(buffer));
            break;
        case MOP_SA_INET6:
            str = inet_ntop(AF_INET6, &address.address.inet6.in_addr,
                            buffer, sizeof(buffer));
            break;
        default:
            MOP_ASSERT(!"Unknown address type!");
            return MOP_STATUS_NOT_IMPLEMENTED;
        }
        if (!str)
        {
            MOP_DEBUG_PRINTEX(MOpDebugScopeSocket, MOpDebugSeverityDebug,
                              "Socket<?> Connecting to (system error: %d)",
                              errno);
        }
        else
        {
            MOP_DEBUG_PRINTEX(MOpDebugScopeSocket, MOpDebugSeverityDebug,
                              "Socket<?> Connecting to %s:%d",
                              str, ntohs(address.port));
        }
    }
#endif

    MOpSocket_close(_this);

    switch (address.type)
    {
    case MOP_SA_INET4:
    {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)&addr;
        addr_len = sizeof(struct sockaddr_in);
        addr_in->sin_family = AF_INET;
        addr_in->sin_port = address.port;
        addr_in->sin_addr.s_addr = address.address.inet4;
        break;
    }
    case MOP_SA_INET6:
    {
        struct sockaddr_in6 *addr_in = (struct sockaddr_in6 *)&addr;
        addr_len = sizeof(struct sockaddr_in6);
        memset(&addr, 0, addr_len);
        addr_in->sin6_family = AF_INET6;
        addr_in->sin6_port = address.port;
        addr_in->sin6_scope_id = address.address.inet6.scope_id;
        memcpy(addr_in->sin6_addr.s6_addr, address.address.inet6.in_addr, 16);
        break;
    }
    default:
        MOP_ASSERT(FALSE);
        return MOP_STATUS_NOT_IMPLEMENTED;
    }

    _this->fd = socket(((struct sockaddr*)&addr)->sa_family, SOCK_STREAM, 0);
    if (_this->fd == -1)
        return MOP_STATUS_ERR;

    if (make_nonblocking(_this->fd))
    {
        close(_this->fd);
        _this->fd = -1;
        return MOP_STATUS_ERR;
    }

    _this->waitWrite = FALSE;
    _this->waitConnect = FALSE;

    for (;;)
    {
        if (connect(_this->fd, (struct sockaddr*)&addr, addr_len) == 0 ||
            errno == EINPROGRESS)
        {
            /* Connect succeeded or was delayed */
            /* "Waiting" even if we succeeded will cause MOpSocket_recv and
             * MOpSocket_send to needlessly return WOULDBLK until poller is run
             * at least once but that is a very minor issue.
             * We can't call MOP_SE_CONNECT here because that would surprise
             * reksio. */
            _this->waitConnect = TRUE;
            return Socket_add(_this, FALSE, TRUE);
        }
        if (errno == EINTR)
            continue;

        close(_this->fd);
        _this->fd = -1;
        return MOP_STATUS_ERR;
    }
}

MOP_STATUS MOpSocket_close(MOpSocket *_this)
{
    if (!_this)
        return MOP_STATUS_NULLPTR;

    if (_this->fd >= 0)
    {
        Socket_del(_this);
        close(_this->fd);
        MOP_DEBUG_PRINTEX(MOpDebugScopeSocket, MOpDebugSeverityDebug,
                          "Socket<%d> Closed", _this->fd);
        _this->fd = -1;
    }
    return MOP_STATUS_OK;
}

/**
 * @return MOP_STATUS_OK, MOP_STATUS_NET_WOULDBLK, MOP_STATUS_NET_SOCK_CLOSED, MOP_STATUS_ERR
 */
MOP_STATUS MOpSocket_send(MOpSocket *_this, const void *data, UINT32 len, UINT32 *bytes_written)
{
    if (!_this)
        return MOP_STATUS_NULLPTR;
    if (_this->fd < 0)
        return MOP_STATUS_NET_SOCK_CLOSED;
    if (_this->waitConnect)
        return MOP_STATUS_NET_WOULDBLK;

    for (;;)
    {
        ssize_t ret = write(_this->fd, data, len);
        if (ret == -1)
        {
            switch (errno)
            {
            case EINTR:
                continue;
            case EWOULDBLOCK:
                Socket_chg(_this, TRUE, TRUE);
                return MOP_STATUS_NET_WOULDBLK;
            case EPIPE:
            case ENOTCONN:
                return MOP_STATUS_NET_SOCK_CLOSED;
            default:
                MOP_DEBUG_PRINTEX(MOpDebugScopePI, MOpDebugSeverityDebug,
                                  "Socket write() error: %s", strerror(errno));
                return MOP_STATUS_ERR;
            }
        }
        *bytes_written = ret;
        _this->waitWrite = FALSE;
        return MOP_STATUS_OK;
    }
}

/**
 * @return MOP_STATUS_OK, MOP_STATUS_NET_WOULDBLK, MOP_STATUS_NET_SOCK_CLOSED, MOP_STATUS_ERR
 */
MOP_STATUS MOpSocket_recv(MOpSocket *_this, void *buffer, UINT32 length, UINT32  *bytes_received)
{
    if (!_this)
        return MOP_STATUS_NULLPTR;
    if (_this->fd < 0)
        return MOP_STATUS_NET_SOCK_CLOSED;
    if (_this->waitConnect)
        return MOP_STATUS_NET_WOULDBLK;

    for (;;)
    {
        ssize_t ret = read(_this->fd, buffer, length);
        if (ret == 0)
        {
            MOpSocket_close(_this);
            return MOP_STATUS_NET_SOCK_CLOSED;
        }
        if (ret == -1)
        {
            switch (errno)
            {
            case EINTR:
                continue;
            case EWOULDBLOCK:
                Socket_chg(_this, TRUE, _this->waitWrite);
                return MOP_STATUS_NET_WOULDBLK;
            case EPIPE:
            case ENOTCONN:
                return MOP_STATUS_NET_SOCK_CLOSED;
            default:
                MOP_DEBUG_PRINTEX(MOpDebugScopePI, MOpDebugSeverityDebug,
                                  "Socket read() error: %s", strerror(errno));
                return MOP_STATUS_ERR;
            }
        }
        else if ((UINT32)ret == length)
        {
            Socket_chg(_this, TRUE, _this->waitWrite);
        }
        *bytes_received = ret;
        return MOP_STATUS_OK;
    }
}

typedef struct poller_data_t
{
    JavaVM *jvm;
    jint jvm_version;
    RequestSliceCallback callback;
    void *userdata;
} poller_data_t;

static void *poller(void *args)
{
    poller_data_t *data = (poller_data_t *)args;
    struct epoll_event events[20];

    /* Attach this thread to the JVM */
    JNIEnv *env;
    struct JavaVMAttachArgs attachArgs;
    memset(&attachArgs, 0, sizeof(attachArgs));
    attachArgs.version = data->jvm_version;
    attachArgs.name = "SocketThread";
    if (data->jvm->AttachCurrentThread(&env, &attachArgs) != 0)
        return NULL;

    for (;;)
    {
        int ret = epoll_wait(epoll_fd, events,
                             sizeof(events) / sizeof(events[0]), -1);
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            MOP_DEBUG_PRINTEX(MOpDebugScopePI, MOpDebugSeverityDebug,
                              "Socket epoll_wait() error: %s", strerror(errno));
            break;
        }
        if (!pthread_mutex_lock(&queue_mutex))
        {
            struct epoll_event *e;
            for (e = events; ret-- > 0; e++)
            {
                Event *ev = (Event*)e->data.ptr;
                if (e->events & (EPOLLIN | EPOLLPRI))
                {
                    ev->events |= EV_READ;
                }
                if (e->events & EPOLLOUT)
                {
                    ev->events |= EV_WRITE;
                }
                if (e->events & EPOLLHUP)
                {
                    ev->events |= EV_CLOSE;
                }
                if (e->events & EPOLLERR)
                {
                    ev->events |= EV_ERR;
                }
            }

            /* All events queued, now signal callback to get Socket_slice
             * to be called. Then wait for Socket_slice to be done before
             * polling again.
             * This means that any events triggered between now and
             * when Socket_slice is called will not be noticed until after
             * Socket_slice but it saves the work of having to remove
             * a socket with data available from the polled list and also
             * keeps this thread from taking 100% CPU as it loops round and
             * round
             */

            data->callback(env, data->userdata);

            pthread_cond_wait(&queue_wait, &queue_mutex);

            pthread_mutex_unlock(&queue_mutex);
        }
    }

    data->jvm->DetachCurrentThread();
    free(data);
    return NULL;
}

/**
 * On some phones the sockets aren't closed when the connection is lost, like
 * when entering airplane mode. The UI is using failing sockets to detect e.g.
 * download failure.
 *
 * To be sure that the information is propagated all the way to the UI we
 * listen in on connectivity loss and shut down the open sockets.
 */
void OnReachabilityChanged(void*, MOpReachabilityStatus status)
{
    if (status & MOP_REACHABILITY_REACHABLE)
        return;

    if (pthread_mutex_lock(&queue_mutex))
        return;

    for (size_t i = 0; i < queue_fill; i++)
        shutdown(queue_poll[i]->socket->fd, SHUT_RDWR);

    pthread_mutex_unlock(&queue_mutex);
}

MOP_STATUS Socket_setup(RequestSliceCallback request_slice, void *userdata)
{
    pthread_attr_t attr;
    pthread_t thread;
    poller_data_t *data;
    JNIEnv *env;
    MOP_ASSERT(epoll_fd < 0);
    epoll_fd = epoll_create(10);
    if (epoll_fd < 0)
        return MOP_STATUS_ERR;
    data = (poller_data_t *)malloc(sizeof(poller_data_t));
    if (!data)
    {
        close(epoll_fd);
        epoll_fd = -1;
        return MOP_STATUS_NOMEM;
    }
    env = getEnv();
    data->jvm_version = env->GetVersion();
    if (env->GetJavaVM(&data->jvm))
    {
        close(epoll_fd);
        epoll_fd = -1;
        free(data);
        return MOP_STATUS_ERR;
    }
    data->callback = request_slice;
    data->userdata = userdata;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread, &attr, poller, data))
    {
        pthread_attr_destroy(&attr);
        close(epoll_fd);
        free(data);
        epoll_fd = -1;
        return MOP_STATUS_ERR;
    }
    pthread_attr_destroy(&attr);

    MOpReachability_monitorStatus(
        MOpReachability_get(), OnReachabilityChanged, NULL);

    return MOP_STATUS_OK;
}

MOP_STATUS Socket_slice(void)
{
    static BOOL inSlice = FALSE;
    size_t i;
    Event **pev, *cev;

    if (inSlice)
    {
        MOP_ASSERT(!inSlice);
    }
    inSlice = TRUE;

    if (pthread_mutex_lock(&queue_mutex))
    {
        inSlice = FALSE;
        return MOP_STATUS_ERR;
    }

    pev = queue_poll;
    cev = queue_callback;
    i = 0;
    while (i < queue_fill)
    {
        if ((*pev)->events)
        {
            cev->events = (*pev)->events;
            cev->socket = (*pev)->socket;
            (*pev)->events = 0;
            cev++;
        }
        pev++;
        i++;
    }
    queue_callback_fill = cev - queue_callback;

    pthread_mutex_unlock(&queue_mutex);

    /* Mark all nodes in the queue as not free:able. */
    cev = queue_callback;
    for (i = 0; i < queue_callback_fill; i++, cev++)
    {
        cev->socket->deleteMode |= 1;
    }

    cev = queue_callback;
    for (i = 0; i < queue_callback_fill; i++, cev++)
    {
        MOpSocket* sock = cev->socket;
        if (sock->waitConnect)
        {
            sock->waitConnect = FALSE;
            if (cev->events & EV_WRITE)
            {
                Socket_chg(sock, TRUE, FALSE);
                sock->callback(sock, MOP_SE_CONNECT, MOP_STATUS_OK, 0,
                               sock->userData);
            }
            else
                sock->callback(sock, MOP_SE_CONNECT, MOP_STATUS_ERR, 0,
                               sock->userData);
        }
        else
        {
            if (cev->events & EV_READ)
                sock->callback(sock, MOP_SE_CANREAD, 0, 0, sock->userData);
            if (cev->events & EV_WRITE)
            {
                Socket_chg(sock, TRUE, FALSE);
                sock->callback(sock, MOP_SE_CANWRITE, 0, 0, sock->userData);
            }
            if (cev->events & EV_CLOSE)
                sock->callback(sock, MOP_SE_CLOSE, 0, 0, sock->userData);
            if (cev->events & EV_ERR)
                sock->callback(sock, MOP_SE_ERROR, 0, 0, sock->userData);
        }
    }

    cev = queue_callback;
    for (i = 0; i < queue_callback_fill; i++, cev++)
    {
        if (cev->socket->deleteMode == 2)
        {
            free(cev->socket);
        }
        else
        {
            cev->socket->deleteMode = 0;
        }
    }
    queue_callback_fill = 0;

    /* Signal poller that all queued events was handled */
    pthread_cond_signal(&queue_wait);

    inSlice = FALSE;

    return MOP_STATUS_OK;
}

static uint32_t event_mask(BOOL watch_read, BOOL watch_write)
{
    return (watch_read ? EPOLLIN | EPOLLPRI : 0) |
        (watch_write ? EPOLLOUT : 0) | EPOLLONESHOT;
}

MOP_STATUS Socket_add(MOpSocket *sock, BOOL watch_read, BOOL watch_write)
{
    struct epoll_event e;
    Event *ev;
    MOP_ASSERT(watch_read || watch_write);
    MOP_ASSERT(sock->fd >= 0);
    MOP_ASSERT(epoll_fd >= 0);
    MOP_ASSERT(!sock->event);
    sock->waitWrite = watch_write;
    e.events = event_mask(watch_read, watch_write);
    if (pthread_mutex_lock(&queue_mutex))
        return MOP_STATUS_ERR;
    if (queue_fill == queue_size)
    {
        size_t ns = queue_size * 2;
        Event **qp, *qc;
        if (ns < 10)
            ns = 10;
        qp = (Event**)realloc(queue_poll, ns * sizeof(Event*));
        if (!qp)
        {
            pthread_mutex_unlock(&queue_mutex);
            return MOP_STATUS_NOMEM;
        }
        queue_poll = qp;
        memset(queue_poll + queue_size, 0, (ns - queue_size) * sizeof(Event*));
        qc = (Event*)realloc(queue_callback, ns * sizeof(Event));
        if (!qc)
        {
            pthread_mutex_unlock(&queue_mutex);
            return MOP_STATUS_NOMEM;
        }
        queue_callback = qc;
        memset(queue_callback + queue_size, 0,
               (ns - queue_size) * sizeof(Event));
        queue_size = ns;
    }
    if (!queue_poll[queue_fill])
    {
        queue_poll[queue_fill] = (Event*)malloc(sizeof(Event));
    }
    ev = queue_poll[queue_fill++];
    sock->event = ev;
    e.data.ptr = ev;
    ev->socket = sock;
    ev->events = 0;
    pthread_mutex_unlock(&queue_mutex);
    return !epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock->fd, &e) ?
        MOP_STATUS_OK : MOP_STATUS_ERR;
}

MOP_STATUS Socket_chg(MOpSocket *sock, BOOL watch_read, BOOL watch_write)
{
    struct epoll_event e;
    MOP_ASSERT(watch_read || watch_write);
    MOP_ASSERT(sock->fd >= 0);
    MOP_ASSERT(epoll_fd >= 0);
    MOP_ASSERT(sock->event);
    sock->waitWrite = watch_write;
    e.events = event_mask(watch_read, watch_write);
    e.data.ptr = sock->event;
    return !epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sock->fd, &e) ?
        MOP_STATUS_OK : MOP_STATUS_ERR;
}

MOP_STATUS Socket_del(MOpSocket *sock)
{
    MOP_ASSERT(sock->fd >= 0);
    MOP_ASSERT(epoll_fd >= 0);
    if (pthread_mutex_lock(&queue_mutex))
        return MOP_STATUS_ERR;
    Event **pev = queue_poll;
    size_t i;
    for (i = 0; i < queue_fill; i++, pev++)
    {
        if ((*pev)->socket == sock)
        {
            Event *x = *pev;
            queue_fill--;
            memmove(pev, pev + 1, (queue_fill - i) * sizeof(Event*));
            queue_poll[queue_fill] = x;
            break;
        }
    }
    Event *cev = queue_callback;
    for (i = 0; i < queue_callback_fill; i++, cev++)
    {
        if (cev->socket == sock)
        {
            cev->events = 0;
            break;
        }
    }
    sock->event = NULL;
    pthread_mutex_unlock(&queue_mutex);
    return !epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock->fd, NULL) ?
        MOP_STATUS_OK : MOP_STATUS_ERR;
}
