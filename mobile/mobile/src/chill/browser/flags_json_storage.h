// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_FLAGS_JSON_STORAGE_H_
#define CHILL_BROWSER_FLAGS_JSON_STORAGE_H_

#include <set>
#include <string>

#include "base/memory/ref_counted.h"
#include "components/prefs/json_pref_store.h"

namespace opera {

class FlagsJsonStorage : public base::RefCountedThreadSafe<FlagsJsonStorage> {
 public:
  // Constructs and object for accessing the flag preferences. The
  // constructor will immediately load the preferences from the JSON storage
  // synchronously, in the current thread.
  FlagsJsonStorage();

  // Retrieves the flags as a set of strings.
  virtual std::set<std::string> GetFlags();

  // Stores the |flags| and returns true on success.
  virtual bool SetFlags(std::set<std::string> flags);

 private:
  virtual ~FlagsJsonStorage() {}
  friend class base::RefCountedThreadSafe<FlagsJsonStorage>;

  scoped_refptr<JsonPrefStore> store_;

  DISALLOW_COPY_AND_ASSIGN(FlagsJsonStorage);
};

}  // namespace opera

#endif  // CHILL_BROWSER_FLAGS_JSON_STORAGE_H_
