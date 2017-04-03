// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_WEB_STORAGE_WEB_STORAGE_LISTENER_H_
#define COMMON_MIGRATION_WEB_STORAGE_WEB_STORAGE_LISTENER_H_
#include <string>
#include <vector>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"

namespace opera {
namespace common {
namespace migration {

/** Listens for extracted URL and key-value pairs.
 */
class WebStorageListener
    : public base::RefCountedThreadSafe<WebStorageListener> {
 public:
  typedef std::pair<base::string16, base::string16> KeyValuePair;

  /** Called when an origin URL has been extracted along with its key-value
   * pairs.
   * @returns whether processing passed data completed successfully.
   */
  virtual bool OnUrlImported(const std::string& url,
                         const std::vector<KeyValuePair>& key_value_pairs) = 0;

 protected:
  friend class base::RefCountedThreadSafe<WebStorageListener>;
  virtual ~WebStorageListener() {}

};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_WEB_STORAGE_WEB_STORAGE_LISTENER_H_
