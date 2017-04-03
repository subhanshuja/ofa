// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

// Multiply-included message file, hence no include guard.
// NOLINT(build/header_guard)

#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/ipc/gfx_param_traits.h"

#include "chill/common/webapps/web_application_info.h"

#define IPC_MESSAGE_START ShellMsgStart

IPC_ENUM_TRAITS_MAX_VALUE(opera::WebApplicationInfo::MobileCapable,
                          opera::WebApplicationInfo::MOBILE_CAPABLE_APPLE)

IPC_STRUCT_TRAITS_BEGIN(opera::WebApplicationInfo::IconInfo)
  IPC_STRUCT_TRAITS_MEMBER(url)
  IPC_STRUCT_TRAITS_MEMBER(width)
  IPC_STRUCT_TRAITS_MEMBER(height)
  IPC_STRUCT_TRAITS_MEMBER(data)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(opera::WebApplicationInfo)
  IPC_STRUCT_TRAITS_MEMBER(title)
  IPC_STRUCT_TRAITS_MEMBER(description)
  IPC_STRUCT_TRAITS_MEMBER(app_url)
  IPC_STRUCT_TRAITS_MEMBER(icons)
  IPC_STRUCT_TRAITS_MEMBER(mobile_capable)
IPC_STRUCT_TRAITS_END()

IPC_MESSAGE_ROUTED0(  // NOLINT(readability/fn_size)
    OpViewMsg_GetWebApplicationInfo)

IPC_MESSAGE_ROUTED1(  // NOLINT(readability/fn_size)
    OperaViewHostMsg_DidGetWebApplicationInfo,
    opera::WebApplicationInfo)
