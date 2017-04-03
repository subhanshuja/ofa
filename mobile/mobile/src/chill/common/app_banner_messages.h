// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

// Multiply-included message file, hence no include guard.
// NOLINT(build/header_guard)

#include <string>

#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/platform/modules/app_banner/WebAppBannerPromptReply.h"

#define IPC_MESSAGE_START ShellMsgStart

IPC_ENUM_TRAITS_MAX_VALUE(blink::WebAppBannerPromptReply,
                          blink::WebAppBannerPromptReply::Cancel)

// Asks the renderer whether an app banner should be shown. It will reply with
// OperaViewHostMsg_AppBannerPromptReply.
IPC_MESSAGE_ROUTED2(  // NOLINT(readability/fn_size)
    OperaViewMsg_AppBannerPromptRequest,
    int /* request_id */,
    std::string /* platform */)

// Tells the renderer that a banner has been accepted.
IPC_MESSAGE_ROUTED2(  // NOLINT(readability/fn_size)
    OperaViewMsg_AppBannerAccepted,
    int32_t /* request_id */,
    std::string /* platform */)

// Tells the renderer that a banner has been dismissed.
IPC_MESSAGE_ROUTED1(  // NOLINT(readability/fn_size)
    OperaViewMsg_AppBannerDismissed,
    int32_t /* request_id */)

// Tells the browser process whether the web page wants the banner to be shown.
// This is a reply from OperaViewMsg_AppBannerPromptRequest.
IPC_MESSAGE_ROUTED3(  // NOLINT(readability/fn_size)
    OperaViewHostMsg_AppBannerPromptReply,
    int /* request_id */,
    blink::WebAppBannerPromptReply /* reply */,
    std::string /* referrer */)

// Tells the browser to restart the app banner display pipeline.
IPC_MESSAGE_ROUTED1(  // NOLINT(readability/fn_size)
    OperaViewHostMsg_RequestShowAppBanner,
    int32_t /* request_id */)
