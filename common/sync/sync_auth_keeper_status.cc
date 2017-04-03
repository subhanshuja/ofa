// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_auth_keeper_status.h"

#include "base/strings/string_number_conversions.h"
#include "components/sync/api/time.h"

#include "common/account/account_util.h"
#include "common/sync/sync_auth_keeper_util.h"

namespace opera {
namespace sync {

std::array<std::string, 2> SyncAuthKeeperStatus::kIgnoredKeys = {
    {"account__current_session_duration", "timestamp"}};

SyncAuthKeeperStatus::SyncAuthKeeperStatus() {}

SyncAuthKeeperStatus::SyncAuthKeeperStatus(
    const SyncAuthKeeperStatus&) = default;

SyncAuthKeeperStatus::~SyncAuthKeeperStatus() {}

std::unique_ptr<base::DictionaryValue> SyncAuthKeeperStatus::AsDict() const {
  base::DictionaryValue value;
  for (auto it = props_.begin(); it != props_.end(); it++) {
    std::string val = opera::sync::kNotAvailableString;
    if (!it->second.empty()) {
      val = it->second;
    }
    value.SetString(it->first, val);
  }
  return value.CreateDeepCopy();
}

bool SyncAuthKeeperStatus::IsSameAs(const SyncAuthKeeperStatus& other) {
  if (props_.size() != other.props_.size())
    return false;
  auto this_it = props_.begin();
  auto other_it = other.props_.begin();

  while (this_it != props_.end()) {
    DCHECK(other_it != other.props_.end());
    if (this_it->first != other_it->first)
      return false;
    if (std::find(kIgnoredKeys.begin(), kIgnoredKeys.end(), this_it->first) ==
        kIgnoredKeys.end()) {
      if (this_it->second != other_it->second)
        return false;
    }

    this_it++;
    other_it++;
  }
  return true;
}

bool SyncAuthKeeperStatus::empty() const {
  return props_.empty();
}

std::string SyncAuthKeeperStatus::FormatBool(bool v) {
  if (v)
    return "true";
  return "false";
}

std::string SyncAuthKeeperStatus::FormatInt(int v) {
  return base::IntToString(v);
}

std::string SyncAuthKeeperStatus::FormatTime(base::Time v) {
  if (v == base::Time())
    return "";

  return syncer::GetTimeDebugString(v);
}

std::string SyncAuthKeeperStatus::FormatTimeDelta(base::TimeDelta v) {
  if (v == base::TimeDelta())
    return "";

  return opera::account_util::TimeDeltaToString(
      v, account_util::PRECISION_SECONDS);
}

}  // namespace sync
}  // namespace opera
