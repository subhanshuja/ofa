// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software AS

#ifndef CHILL_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_JSON_STORAGE_H_
#define CHILL_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_JSON_STORAGE_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/values.h"

namespace opera {

class PushMessagingJsonStorage {
 public:
  // Loads the push messages storage synchronously, in the current thread.
  explicit PushMessagingJsonStorage(const base::FilePath& dir);

  base::DictionaryValue* GetAppIdMap();
  base::DictionaryValue* GetDebtMap();

  // Saves the push message storage asynchronously on the file thread.
  void Save();

 private:
  base::DictionaryValue* GetMap(const std::string& key);
  std::unique_ptr<base::DictionaryValue> Load(const base::FilePath& path);

  base::FilePath filepath_;
  std::unique_ptr<base::DictionaryValue> value_;

  DISALLOW_COPY_AND_ASSIGN(PushMessagingJsonStorage);
};

}  // namespace opera

#endif  // CHILL_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_JSON_STORAGE_H_
