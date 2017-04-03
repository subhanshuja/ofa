// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/sync_manager_factory.h"

#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"

#include "mobile/common/sync/sync_manager_impl.h"

#include "net/url_request/url_request_context_getter.h"

namespace mobile {

// static
SyncManager* SyncManagerFactory::Create(
    const base::FilePath& base_path,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    content::BrowserContext* browser_context,
    InvalidationData* invalidation_data,
    AuthData* auth_data) {
  return new SyncManagerImpl(base_path,
                             ui_task_runner,
                             io_task_runner,
                             browser_context,
                             invalidation_data,
                             auth_data);
}

}  // namespace mobile
