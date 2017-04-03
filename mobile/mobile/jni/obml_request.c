/* -*- Mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2015 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "obml_request.h"

#include <jni.h>
#include <string.h>

#include "config/pch.h"
#include "bream.h"
#include "vminstance_ext.h"

#include "mini/c/shared/generic/vm_integ/VMEntries.h"

void MakeBlindOBMLRequest(const char* url) {
  MOpVMEntry_nativeui_api_makeBlindRequest(&g_vm_instance.base, url,
                                           strlen(url));
}
