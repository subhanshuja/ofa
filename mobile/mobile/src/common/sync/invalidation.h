// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_INVALIDATION_H_
#define COMMON_SYNC_INVALIDATION_H_

#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "mobile/common/sync/invalidation_data.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace mobile {

class Invalidation : public InvalidationData {
 public:
  Invalidation();
  virtual ~Invalidation();

  void SetDeviceId(const std::string& device_id);

 private:
  bool first_;
};

}  // namespace mobile

#endif  // COMMON_SYNC_INVALIDATION_H_
