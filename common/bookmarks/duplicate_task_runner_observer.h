// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_TASK_RUNNER_OBSERVER_H_
#define COMMON_BOOKMARKS_TASK_RUNNER_OBSERVER_H_
namespace opera {
class DuplicateTask;
class DuplicateTaskRunner;

class TaskRunnerObserver {
 public:
  // Called right after the last slice of the last task in the queue
  // has finished processing.
  virtual void OnQueueEmpty(DuplicateTaskRunner* runner) = 0;

  // Called right before a task is removed from the queue, either
  // because it has fininshed or was cancelled. Any references to the
  // task pointer after this call are illegal.
  virtual void OnTaskFinished(DuplicateTaskRunner* runner,
                              scoped_refptr<DuplicateTask> task) = 0;
};
}
#endif  // COMMON_BOOKMARKS_TASK_RUNNER_OBSERVER_H_
