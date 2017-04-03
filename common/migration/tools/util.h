// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_TOOLS_UTIL_H_
#define COMMON_MIGRATION_TOOLS_UTIL_H_

#include "libxml/parser.h"
#include "base/numerics/safe_conversions.h"

namespace opera {
namespace common {
namespace migration {

/** Simple anchor to auto-clean xml doc. */

class XmlDocAnchor {
 public:
  explicit XmlDocAnchor(xmlDocPtr doc) : doc_(doc) {}
  ~XmlDocAnchor() { xmlFreeDoc(doc_); }

 private:
  xmlDocPtr doc_;
};


inline xmlDocPtr xmlReadMemory(const std::string &buffer, const char *URL) {
  return ::xmlReadMemory(buffer.c_str(), base::checked_cast<int>(buffer.size()),
                         URL, NULL, 0);
}


}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_TOOLS_UTIL_H_
