// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_MIGRATION_WAND_WAND_OBJECT_H_
#define COMMON_MIGRATION_WAND_WAND_OBJECT_H_

#include "base/strings/string16.h"

namespace opera {
namespace common {
namespace migration {

class WandReader;

/** Usually represents a field in a form that's suspected of being a login
 * prompt.
 */
struct WandObject {
  WandObject();
  WandObject(const WandObject&);
  ~WandObject();
  bool Parse(WandReader* reader);
  bool operator==(const WandObject& rhs) const;

  base::string16 name_;       ///< Name of formobject
  base::string16 value_;      ///< Value if not password
  bool is_guessed_username_;  ///< This form field is probably the username
  bool is_password_;    ///< Is password. Use .password_ instead of .value_.
  /// Is a textfield (if false, may still be a textfield from an old database)
  bool is_textfield_for_sure_;
  ///< Value was different than the default value when storing.
  bool is_changed_;
  base::string16 password_;  ///< Value if password
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_WAND_WAND_OBJECT_H_
