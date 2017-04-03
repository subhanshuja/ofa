/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#ifndef VMINSTANCE_EXT_H
#define VMINSTANCE_EXT_H

#include "bream/vm/c/inc/bream_host.h"

OP_EXTERN_C_BEGIN
#include "mini/c/shared/generic/vm_integ/VMInstance.h"

typedef struct _MOpVMInstanceExternal
{
    MOpVMInstance base;
    struct _MOpTimer* timer;
    struct _MOpDataStore* ds;
    char* gogi_saved_page_path;
    bream_callback_t callback;
    jobject bream;
    jobject invokes;
    BOOL exited;
} MOpVMInstanceExternal;

void MOpVMInstanceExternal_setTimer(MOpVMInstanceExternal* _this, UINT32 target);
OP_EXTERN_C_END

#endif // VMINSTANCE_EXT_H
