// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/flags_json_storage.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/values.h"
#include "components/prefs/pref_filter.h"
#include "content/public/browser/browser_thread.h"

#define STORE_FILE_NAME "flags"

namespace opera {

FlagsJsonStorage::FlagsJsonStorage() {
  base::FilePath pref_pathname;
  if (PathService::Get(base::DIR_ANDROID_APP_DATA, &pref_pathname)) {
    pref_pathname = pref_pathname.Append(FILE_PATH_LITERAL(STORE_FILE_NAME));
    scoped_refptr<base::SequencedTaskRunner> sequenced_task_runner =
        JsonPrefStore::GetTaskRunnerForFile(
            pref_pathname, content::BrowserThread::GetBlockingPool());

    store_ = new JsonPrefStore(
      pref_pathname, sequenced_task_runner, std::unique_ptr<PrefFilter>());
    store_->ReadPrefs();
  } else {
    LOG(ERROR) << "Unable to locate the \"flags\" preference file.";
  }
}

std::set<std::string> FlagsJsonStorage::GetFlags() {
  DCHECK(store_.get());

  const base::Value* flags_value;
  if (!store_->GetValue("flags", &flags_value))
    return std::set<std::string>();

  const base::ListValue* flag_list;
  if (!flags_value->GetAsList(&flag_list))
    return std::set<std::string>();

  std::set<std::string> flags;
  for (base::ListValue::const_iterator it = flag_list->begin();
       it != flag_list->end();
       ++it) {
    std::string flag_name;
    if (!(*it)->GetAsString(&flag_name)) {
      LOG(WARNING) << "Invalid entry in \"" << STORE_FILE_NAME << "\" file.";
      continue;
    }
    flags.insert(flag_name);
  }

  return flags;
}

bool FlagsJsonStorage::SetFlags(std::set<std::string> flags) {
  DCHECK(store_.get());

  base::ListValue* flag_list = new base::ListValue();

  for (std::set<std::string>::const_iterator it = flags.begin();
       it != flags.end(); ++it) {
    flag_list->Append(new base::StringValue(*it));
  }

  store_->SetValue(
      "flags",
      base::WrapUnique(flag_list),
      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  store_->CommitPendingWrite();

  return true;
}

}  // namespace opera
