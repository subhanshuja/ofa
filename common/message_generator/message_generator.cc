// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

// Type definitions.
#ifndef IPC_MESSAGE_IMPL
# define IPC_MESSAGE_IMPL
#endif
#include "common/message_generator/all_messages.h"

// Constructors.
#include "ipc/struct_constructor_macros.h"
#include "common/message_generator/all_messages.h"

// Destructors.
#include "ipc/struct_destructor_macros.h"
#include "common/message_generator/all_messages.h"

// Param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#include "common/message_generator/all_messages.h"
}

// Param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#include "common/message_generator/all_messages.h"
}

// Param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#include "common/message_generator/all_messages.h"
}
