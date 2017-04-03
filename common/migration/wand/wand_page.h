// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_WAND_WAND_PAGE_H_
#define COMMON_MIGRATION_WAND_WAND_PAGE_H_

#include <vector>

#include "base/strings/string16.h"

#include "common/migration/wand/wand_object.h"

namespace opera {
namespace common {
namespace migration {

/** A WandPage represents a single form on a webpage in which login data may
 * have been entered.
 * We usually cannot be *sure*  if an arbitrary form is a login prompt, so
 * we were doing some guessing and heuristics.
 * A WandPage contains WandObjects, which usually represent fields in the form.
 */
struct WandPage {
  WandPage();
  WandPage(const WandPage&);
  ~WandPage();
  bool Parse(WandReader* reader, int32_t wand_version);
  bool operator==(const WandPage& rhs) const;

  std::vector<WandObject> objects_;
  // These were not documented in Presto's code
  base::string16 url_;
  base::string16 topdoc_url_;
  base::string16 url_action_;
  base::string16 submitname_;
  uint32_t form_number_;
  uint32_t flags_;  ///< See the static ints below for values
  int32_t offset_x_;
  int32_t offset_y_;
  int32_t document_x_;
  int32_t document_y_;

  // flags_ values:
  // Don't remember passwords on this page
  static int WAND_FLAG_NEVER_ON_THIS_PAGE;
  // Remember passwords on this entire server
  static int WAND_FLAG_ON_THIS_SERVER;
  // These two were used if WAND_ECOMMERCE_SUPPORT was defined
  static int WAND_FLAG_STORE_ECOMMERCE;
  static int WAND_FLAG_STORE_NOTHING_BUT_ECOMMERCE;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_WAND_WAND_PAGE_H_
