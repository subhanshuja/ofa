// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

// No include guard, included multiple times.

#include "base/memory/shared_memory.h"
#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"

#define IPC_MESSAGE_START OperaCommonMsgStart

// Update Browser JS. Takes a SharedMemoryHandle and a size containing a
// blink::WebUChar* with the Browser JS content.
IPC_MESSAGE_CONTROL2(OpMsg_BrowserJsUpdated,
                     base::SharedMemoryHandle,
                     uint64_t)

// Sets User JS. Takes a SharedMemoryHandle and a size containing a
// blink::WebUChar* with the User JS content.
IPC_MESSAGE_CONTROL2(OpMsg_SetUserJs,
                     base::SharedMemoryHandle,
                     uint64_t)

// Update SitePrefs. Takes a SharedMemoryHandle and a size containing a
// blink::WebUChar* with the SitePrefs content.
IPC_MESSAGE_CONTROL2(OpMsg_SitePrefsUpdated,
                     base::SharedMemoryHandle,
                     uint64_t)

// Enable or disable browser.js for this renderer
IPC_MESSAGE_CONTROL1(OpMsg_BrowserJsEnable,
                     bool)

// Enable or disable site preferences for this renderer
IPC_MESSAGE_CONTROL1(OpMsg_SitePrefsEnable,
                     bool)
