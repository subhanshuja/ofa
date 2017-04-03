/* -*- Mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Opera Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <jni.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "obmlview.h"
#include "platform.h"
#include "vminstance_ext.h"

#include "bream/vm/c/graphics/image_info.h"
#include "bream/vm/c/graphics/internal_io.h"

#include "mini/c/shared/core/components/DataStore.h"
#include "mini/c/shared/core/components/DynamicContent.h"
#include "mini/c/shared/core/components/TransferManager.h"
#include "mini/c/shared/core/document/DocumentLite.h"
#include "mini/c/shared/core/document/TextSelector.h"
#include "mini/c/shared/core/encoding/tstring.h"
#include "mini/c/shared/core/font/Font.h"
#include "mini/c/shared/core/font/FontsCollection.h"
#include "mini/c/shared/core/hardcore/types/Point.h"
#include "mini/c/shared/core/obml/ObmlDocument.h"
#include "mini/c/shared/core/pi/ScreenGraphicsContext.h"
#include "mini/c/shared/core/pi/SystemInfo.h"
#include "mini/c/shared/core/pi/HttpHeaders.h"
#include "mini/c/shared/core/protocols/cs/FileDownload.h"
#include "mini/c/shared/core/protocols/obsp/Obsp.h"
#include "mini/c/shared/core/protocols/obsp/ObspConnection.h"
#include "mini/c/shared/core/renderer/FoldData.h"
#include "mini/c/shared/core/settings/SettingsManager.h"
#include "mini/c/shared/core/stream/DataStream.h"
#include "mini/c/shared/generic/pi/GenericSystemInfo.h"
#include "mini/c/shared/generic/vm_integ/ContextMenuInfo.h"
#include "mini/c/shared/generic/vm_integ/ObjectWrapper.h"
#include "mini/c/shared/generic/vm_integ/ObmlSelectWidget.h"
#include "mini/c/shared/generic/vm_integ/SavedPages.h"
#include "mini/c/shared/generic/vm_integ/VMEntries.h"
#include "mini/c/shared/generic/vm_integ/VMFunctions.h"
#include "mini/c/shared/generic/vm_integ/VMHelpers.h"
#include "mini/c/shared/generic/vm_integ/VMInstance.h"
#include "mini/c/shared/generic/vm_integ/VMNativeRequest.h"
#include "mini/c/shared/generic/vm_integ/VMString.h"

#ifdef MOP_FEATURE_ODP
#include "mini/c/shared/core/components/Portal.h"
#include "mini/c/shared/core/hardcore/GlobalMemory.h"
#include "mini/c/shared/core/renderer/RendererGroup.h"
#include "mini/c/shared/core/renderer/RendererTiling.h"
#include "mini/c/shared/generic/vm_integ/TilesContainer.h"
#endif /* MOP_FEATURE_ODP */

static const char* ioexception_message(int _errno)
{
    return strerror(_errno);
}

/**
 * Belongs in bream.cpp but want to build invokes in C to
 * avoid warnings etc. extern "C" is not enough.
 */
BOOL handle_invoke(MOpVMInstance* inst, bream_t* vm, int function) {
    switch (function) {
#    define INVOKES_FILE_TO_INCLUDE "generated_invokes.inc"
#    define VMINSTANCE inst
#    define BREAM_VM vm
#    include "mini/c/shared/generic/vm_integ/VMInvokeDefs.h"
#    undef BREAM_VM
#    undef VMINSTANCE
#    undef INVOKES_FILE_TO_INCLUDE
    default:
        return FALSE;
    }
    return TRUE;
}
