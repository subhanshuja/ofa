/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#ifndef IMPL_SOCKET_H
#define IMPL_SOCKET_H

#include <jni.h>

typedef void (* RequestSliceCallback)(JNIEnv *env, void *userdata);

/* Must be called before any other Socket_ or MOpSocket_ method */
/* The requestSlice method must be thread safe */
MOP_STATUS Socket_setup(RequestSliceCallback requestSlice, void *userdata);
MOP_STATUS Socket_slice(void);

#endif /* IMPL_SOCKET_H */
