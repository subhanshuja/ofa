// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_TASK_RUNNER_H_
#define COMMON_BOOKMARKS_DUPLICATE_TASK_RUNNER_H_

#include <deque>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

#include "common/bookmarks/duplicate_task_runner_observer.h"
#include "common/bookmarks/duplicate_util.h"

namespace opera {
class DuplicateTask;

class DuplicateTaskRunner {
 public:
  enum QueueType { QT_FIFO, QT_LIFO };

  DuplicateTaskRunner(QueueType type, base::TimeDelta delay);
  ~DuplicateTaskRunner();

  // Post the given task onto the internal task queue. The runner will take
  // ownership of the task and will periodically execute it until the
  // task claims it is finished. The task will become cancelled on browser
  // termination, upon a ResetQueue() call or in case it decides to.
  void PostTask(scoped_refptr<DuplicateTask> task);

  // Clear the task queue, do not run any of the tasks in it any more.
  // The tasks will get cleaned up.
  void ResetQueue();

  // Whether the runner has any tasks in progress at the given time.
  bool IsQueueEmpty() const;

  void Pause();
  void Unpause();
  bool IsPaused();

  void SetObserver(TaskRunnerObserver* observer);

 protected:
  typedef std::deque<scoped_refptr<DuplicateTask>> TaskQueueT;

  void DoRunTask(scoped_refptr<DuplicateTask> task);

  // The task that is currently processed, depend on the queue type (FIFO,
  // LIFO).
  scoped_refptr<DuplicateTask> current_task();

  // Queue helper methods.
  void InsertTask(scoped_refptr<DuplicateTask> task);
  void RemoveTask(scoped_refptr<DuplicateTask> task);

  void DoProcessQueue();

  // Whether the runner has posted a task slice.
  bool run_slice_posted_;

  // Whether the queue has been Pause()d. The pending tasks will not be
  // processed until Unpause() is called.
  bool is_paused_;

  // Determines the order in which tasks are picked up from the queue.
  QueueType type_;

  // The task run slice post delay.
  base::TimeDelta task_post_delay_;

  TaskQueueT task_queue_;

  TaskRunnerObserver* observer_;

  base::WeakPtrFactory<DuplicateTaskRunner> weak_ptr_factory_;
};
}  // namespace opera
#endif  // COMMON_BOOKMARKS_DUPLICATE_TASK_RUNNER_H_
