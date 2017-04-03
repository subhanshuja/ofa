// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/duplicate_task.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"

#include "common/bookmarks/duplicate_util.h"

namespace opera {
DuplicateTask::DuplicateTask() {}

DuplicateTask::~DuplicateTask() {}

bool DuplicateTask::IsCancelled() {
  return false;
}
}  // namespace opera
