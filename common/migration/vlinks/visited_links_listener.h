// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_VLINKS_VISITED_LINKS_LISTENER_H_
#define COMMON_MIGRATION_VLINKS_VISITED_LINKS_LISTENER_H_
#include <vector>

#include "common/migration/vlinks/visited_link.h"
#include "base/memory/ref_counted.h"

namespace opera {
namespace common {
namespace migration {

class VisitedLinksListener
    : public base::RefCountedThreadSafe<VisitedLinksListener> {
 public:
  /**
   * @note This listener will be called on the thread that does file input
   * and parsing. If you plan to do long-running work in response to this call,
   * delegate it to an appropriate thread. The caller assumes this method
   * is fast.
   */
  virtual void OnVisitedLinksArrived(
      const std::vector<VisitedLink>& visited_links) = 0;

 protected:
  friend class base::RefCountedThreadSafe<VisitedLinksListener>;
  virtual ~VisitedLinksListener() {}
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_VLINKS_VISITED_LINKS_LISTENER_H_
