/* -*- Mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

extern "C" {
#include "core/pi/Resources.h"
#include "core/stream/StaticStream.h"
}

#include "platform.h"

static const uint8_t res_elements[] = {
#include "elements.inc"
};

static const void* getReadOnlyAsset(const char* name, size_t* size)
{
    if (strcmp(name, "elements.png") == 0) {
        *size = sizeof(res_elements);
        return res_elements;
    }
    return NULL;
}

MOP_STATUS MOpPlatformResStream_create(MOpDataIStream** datastream, const TCHAR* resourceID)
{
    const size_t prefixlen = strlen(REKSIO_ANDROID_PRESTORAGE_PREFIX);
    if (strncmp(resourceID, REKSIO_ANDROID_PRESTORAGE_PREFIX, prefixlen) == 0 &&
        resourceID[prefixlen] == ':') {
        size_t size;
        const void* ptr = getReadOnlyAsset(resourceID + prefixlen + 1, &size);
        if (!ptr)
            return MOP_STATUS_NOFILE;
        return MOpStaticStream_create(datastream, (void*)ptr, size);
    }
    return MOP_STATUS_NOFILE;
}
