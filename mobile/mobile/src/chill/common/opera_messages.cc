// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_messages.cc
// @final-synchronized

// Get basic type definitions.
#define IPC_MESSAGE_IMPL
#include "chill/common/opera_messages.h"

// Generate constructors.
#include "ipc/struct_constructor_macros.h"
#include "chill/common/opera_messages.h"  // NOLINT(build/include)

// Generate destructors.
#include "ipc/struct_destructor_macros.h"
#include "chill/common/opera_messages.h"  // NOLINT(build/include)

// Generate param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#include "chill/common/opera_messages.h"  // NOLINT(build/include)
}  // namespace IPC

// Generate param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#include "chill/common/opera_messages.h"  // NOLINT(build/include)
}  // namespace IPC

// Generate param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#include "chill/common/opera_messages.h"  // NOLINT(build/include)
}  // namespace IPC
