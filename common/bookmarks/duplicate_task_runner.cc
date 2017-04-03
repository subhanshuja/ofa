// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/duplicate_task_runner.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/notification_service.h"

#include "common/bookmarks/duplicate_task.h"
#include "common/bookmarks/duplicate_util.h"

namespace opera {
DuplicateTaskRunner::DuplicateTaskRunner(QueueType type, base::TimeDelta delay)
    : run_slice_posted_(false),
      is_paused_(false),
      type_(type),
      task_post_delay_(delay),
      observer_(nullptr),
      weak_ptr_factory_(this) {
  ResetQueue();
}

DuplicateTaskRunner::~DuplicateTaskRunner() {
  ResetQueue();
}

void DuplicateTaskRunner::PostTask(scoped_refptr<DuplicateTask> task) {
  DCHECK(task);
  VLOG(6) << "Task posted: " << task->name();
  InsertTask(task);
  if (!run_slice_posted_)
    DoProcessQueue();
}

void DuplicateTaskRunner::ResetQueue() {
  VLOG(3) << "Reset queue called, queue is cleared.";
  run_slice_posted_ = false;
  is_paused_ = false;
  task_queue_.clear();
}

bool DuplicateTaskRunner::IsQueueEmpty() const {
  return task_queue_.empty();
}

void DuplicateTaskRunner::Pause() {
  VLOG(3) << "Pausing queue.";
  DCHECK(!IsPaused());
  is_paused_ = true;
}

void DuplicateTaskRunner::Unpause() {
  VLOG(3) << "Unpausing queue.";
  DCHECK(IsPaused());
  DCHECK(!run_slice_posted_);
  is_paused_ = false;
  DoProcessQueue();
}

bool DuplicateTaskRunner::IsPaused() {
  return is_paused_;
}

void DuplicateTaskRunner::SetObserver(TaskRunnerObserver* observer) {
  observer_ = observer;
}

void DuplicateTaskRunner::DoRunTask(scoped_refptr<DuplicateTask> task) {
  if (!run_slice_posted_) {
    VLOG(2) << "Queue was reset in the meantime.";
    DCHECK(task_queue_.empty());
    return;
  }

  if (!task->IsFinished() && !task->IsCancelled()) {
    VLOG(5) << "Running " << task->name();
    task->RunImpl();
  } else if (task->IsFinished()) {
    VLOG(5) << "IsFinished " << task->name();
  } else if (task->IsCancelled()) {
    VLOG(5) << "IsCancelled " << task->name();
  }

  run_slice_posted_ = false;
  DoProcessQueue();
}

scoped_refptr<DuplicateTask> DuplicateTaskRunner::current_task() {
  if (task_queue_.empty())
    return nullptr;

  if (type_ == QT_FIFO)
    return task_queue_.back().get();
  else if (type_ == QT_LIFO)
    return task_queue_.front().get();

  NOTREACHED();
  return nullptr;
}

void DuplicateTaskRunner::InsertTask(scoped_refptr<DuplicateTask> task) {
  DCHECK(task);
  DCHECK(std::find(task_queue_.begin(), task_queue_.end(), task) ==
         task_queue_.end());
  VLOG(6) << "InsertTask " << task->name();
  task_queue_.push_front(task);
}

void DuplicateTaskRunner::RemoveTask(scoped_refptr<DuplicateTask> task) {
  DCHECK(task);

  auto task_it = std::find(task_queue_.begin(), task_queue_.end(), task);
  DCHECK(task_it != task_queue_.end());
  DCHECK(*task_it == current_task());
  VLOG(6) << "RemoveTask " << task->name();
  task_queue_.erase(task_it);
}

void DuplicateTaskRunner::DoProcessQueue() {
  DCHECK(!run_slice_posted_);
  if (IsPaused()) {
    VLOG(3) << "Queue processing is paused.";
    return;
  }

  scoped_refptr<DuplicateTask> task = current_task();
  VLOG(6) << "Current task in queue is " << task->name();
  while (task) {
    if (task->IsCancelled() || task->IsFinished()) {
      VLOG(6) << "Task has finished: " << task->name();
      observer_->OnTaskFinished(this, task);
      RemoveTask(task);
      task = current_task();
    } else {
      run_slice_posted_ = true;
      VLOG(6) << "Run slice posted for " << task->name();
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE, base::Bind(&DuplicateTaskRunner::DoRunTask,
                                weak_ptr_factory_.GetWeakPtr(), task),
          task_post_delay_);
      return;
    }
  }

  VLOG(3) << "Queue is empty.";
  DCHECK(task_queue_.empty());
  observer_->OnQueueEmpty(this);
}
}  // namespace opera
