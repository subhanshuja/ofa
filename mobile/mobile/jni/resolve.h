/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2013 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#ifndef IMPL_RESOLVE_H
#define IMPL_RESOLVE_H

#include <jni.h>

typedef void (* RequestSliceCallback)(JNIEnv *env, void *userdata);

/* Must be called before any other Resolve_ or MOpResolver_ method */
/* The requestSlice method must be thread safe */
MOP_STATUS Resolve_setup(RequestSliceCallback requestSlice, void *userdata);
MOP_STATUS Resolve_slice(void);

#endif /* IMPL_RESOLVE_H */

