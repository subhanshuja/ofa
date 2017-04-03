// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software AS

#include "chill/browser/push_messaging/push_messaging_json_storage.h"

#include "base/files/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_thread.h"

namespace {
const char kPushMsgFile[] = "pushmsg";

const char kAppIdMap[] = "appid";
const char kDebtMap[] = "debt";

void SaveOnFileThread(const base::FilePath& path, const std::string& str) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);
  int str_size = static_cast<int>(str.size());
  bool success = (base::WriteFile(path, str.data(), str.size()) == str_size);
  DCHECK(success);
}

}  // namespace

namespace opera {

PushMessagingJsonStorage::PushMessagingJsonStorage(const base::FilePath& dir)
    : filepath_(dir.Append(kPushMsgFile)), value_(Load(filepath_)) {}

base::DictionaryValue* PushMessagingJsonStorage::GetAppIdMap() {
  return GetMap(kAppIdMap);
}

base::DictionaryValue* PushMessagingJsonStorage::GetDebtMap() {
  return GetMap(kDebtMap);
}

void PushMessagingJsonStorage::Save() {
  if (!value_)
    return;

  std::string serialized;
  JSONStringValueSerializer json(&serialized);
  bool success = json.Serialize(*value_);
  DCHECK(success);

  content::BrowserThread::PostTask(
      content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&SaveOnFileThread, filepath_, serialized));
}

base::DictionaryValue* PushMessagingJsonStorage::GetMap(
    const std::string& key) {
  if (!value_) {
    value_.reset(new base::DictionaryValue);
    value_->SetWithoutPathExpansion(
        kAppIdMap, base::WrapUnique(new base::DictionaryValue));
    value_->SetWithoutPathExpansion(
        kDebtMap, base::WrapUnique(new base::DictionaryValue));
  }

  base::DictionaryValue* map = nullptr;

  if (value_->GetDictionaryWithoutPathExpansion(key, &map))
    return map;

  NOTREACHED();
  return nullptr;
}

std::unique_ptr<base::DictionaryValue> PushMessagingJsonStorage::Load(
    const base::FilePath& path) {
  JSONFileValueDeserializer json(path);
  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(json.Deserialize(nullptr, nullptr));

  // We support loading both the old and the new formats. The old format was
  // simply the app-id-map, stored as the top-level dictionary of the file:
  //
  //  {
  //    <app-id-map contents>
  //  }
  //
  // The new format adds a new outer dictionary, moving the previous data into
  // a field:
  //
  //  {
  //    kAppIdMap: { <app-id-map contents> },
  //    kDebtMap: { <debt-map contents> },
  //  }

  // If the keys kAppIdMap and kDebtMap are found, the file is already in
  // the new format.

  if (!dict || (dict->HasKey(kAppIdMap) && dict->HasKey(kDebtMap)))
    return dict;

  // If the keys kAppIdMap and kDebtMap are not found, the file is in the old
  // format. Migrate to the new format by loading the top level
  // DictionaryValue into the kAppIdMap field.

  std::unique_ptr<base::DictionaryValue> value =
      base::WrapUnique(new base::DictionaryValue);

  value->SetWithoutPathExpansion(kAppIdMap, std::move(dict));
  value->SetWithoutPathExpansion(kDebtMap,
                                 base::WrapUnique(new base::DictionaryValue));

  return value;
}

}  // namespace opera
