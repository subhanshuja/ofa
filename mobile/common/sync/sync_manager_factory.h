// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNC_MANAGER_FACTORY_H_
#define MOBILE_COMMON_SYNC_SYNC_MANAGER_FACTORY_H_

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "mobile/common/sync/sync_manager.h"

namespace base {
class SequencedTaskRunner;
class SingleThreadTaskRunner;
}

namespace content {
class BrowserContext;
}

namespace net {
class URLRequestContextGetter;
}

namespace mobile {

class AuthData;
class InvalidationData;

class SyncManagerFactory {
public:
  // Either browser_context or request_context_getter can be NULL, not both
  static SyncManager* Create(
      const base::FilePath& base_path,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      scoped_refptr<base::SequencedTaskRunner> io_task_runner,
      content::BrowserContext* browser_context,
      InvalidationData* invalidation_data,
      AuthData* auth_data);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_SYNC_SYNC_MANAGER_FACTORY_H_
