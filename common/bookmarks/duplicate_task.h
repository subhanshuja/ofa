// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_TASK_H_
#define COMMON_BOOKMARKS_DUPLICATE_TASK_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "common/bookmarks/duplicate_tracker.h"

namespace opera {
class DuplicateTaskRunner;

// Represents a time-sliced task that will be posted on
// the |runner_| thread until IsFinished() returns true.
// Actual work is done in RunImpl() implementation.
// Note that the time slice taken by a single task run
// should be as small as possible in order to make sure
// that the browser remains responsive, especially
// since the |runner_| is meant to operate on the UI thread.
class DuplicateTask : public base::RefCounted<DuplicateTask> {
 public:
  DuplicateTask();

  // Returns true in case the task has been cancelled and RunImpl()
  // should not be called, but rather the task should be abandoned
  // as soon as possible.
  virtual bool IsCancelled();

  // The actual worker method, should be kept as efficient as possible.
  virtual void RunImpl() = 0;

  // Should return true once the task is to be considered finished.
  // The task will be removed from the |runner_| task queue once this
  // is set, also, RunImpl() won't run anymore.
  virtual bool IsFinished() = 0;

  // Task name used for debugging purposes.
  virtual std::string name() const = 0;

 protected:
  friend class base::RefCounted<DuplicateTask>;
  virtual ~DuplicateTask();
};
}  // namespace opera
#endif  // COMMON_BOOKMARKS_DUPLICATE_TASK_H_
