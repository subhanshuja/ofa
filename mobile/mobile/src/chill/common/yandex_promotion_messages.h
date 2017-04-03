// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

// Multiply-included message file, hence no include guard.
// NOLINT(build/header_guard)

#include <string>

#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"

#define IPC_MESSAGE_START ShellMsgStart

// Ask the browser process whether we can ask to change default search engine.
// It will reply with YandexMsg_CanAskToSetAsDefaultReply.
IPC_MESSAGE_ROUTED2(  // NOLINT(readability/fn_size)
    YandexMsg_CanAskToSetAsDefault,
    uint32_t /* request_id */,
    std::string /* engine */)

IPC_MESSAGE_ROUTED2(  // NOLINT(readability/fn_size)
    YandexMsg_CanAskToSetAsDefaultReply,
    uint32_t /* request_id */,
    bool /* asking is allowed */)

// Ask the browser process to change default search engine.
// It will reply with YandexMsg_AskToSetAsDefaultReply.
IPC_MESSAGE_ROUTED2(  // NOLINT(readability/fn_size)
    YandexMsg_AskToSetAsDefault,
    uint32_t /* request_id */,
    std::string /* engine */)

IPC_MESSAGE_ROUTED1(  // NOLINT(readability/fn_size)
    YandexMsg_AskToSetAsDefaultReply,
    uint32_t /* request_id */)
