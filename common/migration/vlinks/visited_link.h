// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_VLINKS_VISITED_LINK_H_
#define COMMON_MIGRATION_VLINKS_VISITED_LINK_H_

#include <stdint.h>
#include <vector>
#include <string>

#include "url/gurl.h"

namespace opera {
namespace common {
namespace migration {

class DataStreamReader;
class VisitedLink {
 public:
  struct RelativeLink {
    std::string url_;
    time_t last_visited_;
  };
  typedef std::vector<RelativeLink> RelativeLinks;

  VisitedLink();
  VisitedLink(const VisitedLink&);
  ~VisitedLink();
  bool Parse(DataStreamReader* reader);
  GURL GetUrl() const;
  time_t GetLastVisitedTime() const;

  /** Returns relative links visited from this link.
   *
   * These are not fully qualified domain names.
   */
  const RelativeLinks& GetRelativeLinks() const;

 private:
  bool ParseRelativeLink(DataStreamReader* reader);
  GURL url_;
  time_t last_visited_;
  RelativeLinks relative_links_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_VLINKS_VISITED_LINK_H_
