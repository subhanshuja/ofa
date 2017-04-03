// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/token_cache/token_cache_webdata.h"

#include <memory>

#include "base/bind.h"
#include "base/memory/ptr_util.h"

#include "common/oauth2/token_cache/token_cache_table.h"

namespace opera {
namespace oauth2 {
class TokenCacheWebDataBackend
    : public base::RefCountedDeleteOnMessageLoop<
          TokenCacheWebDataBackend> {
 public:
  TokenCacheWebDataBackend(
      scoped_refptr<base::SingleThreadTaskRunner> db_thread)
      : base::RefCountedDeleteOnMessageLoop<TokenCacheWebDataBackend>(
            db_thread) {}

  WebDatabase::State ClearTokens(WebDatabase* db) {
    if (TokenCacheTable::FromWebDatabase(db)->ClearTokens()) {
      return WebDatabase::COMMIT_NEEDED;
    }
    return WebDatabase::COMMIT_NOT_NEEDED;
  }

  WebDatabase::State SaveTokens(
      std::vector<scoped_refptr<const AuthToken>> tokens,
      WebDatabase* db) {
    if (TokenCacheTable::FromWebDatabase(db)->SaveTokens(tokens)) {
      return WebDatabase::COMMIT_NEEDED;
    }
    return WebDatabase::COMMIT_NOT_NEEDED;
  }

  std::unique_ptr<WDTypedResult> LoadTokens(WebDatabase* db) {
    std::vector<scoped_refptr<const AuthToken>> tokens;
    TokenCacheTable::FromWebDatabase(db)->LoadTokens(tokens);
    return base::WrapUnique(
        new WDResult<std::vector<scoped_refptr<const AuthToken>>>(TOKEN_RESULT,
                                                                  tokens));
  }

 protected:
  virtual ~TokenCacheWebDataBackend() {}

 private:
  friend class base::RefCountedDeleteOnMessageLoop<
      TokenCacheWebDataBackend>;
  friend class base::DeleteHelper<TokenCacheWebDataBackend>;
};

TokenCacheWebData::TokenCacheWebData(
    scoped_refptr<WebDatabaseService> wdbs,
    scoped_refptr<base::SingleThreadTaskRunner> ui_thread,
    scoped_refptr<base::SingleThreadTaskRunner> db_thread,
    const ProfileErrorCallback& callback)
    : WebDataServiceBase(wdbs, callback, ui_thread),
      token_backend_(new TokenCacheWebDataBackend(db_thread)) {}

TokenCacheWebData::~TokenCacheWebData() {}

void TokenCacheWebData::ClearTokens() {
  wdbs_->ScheduleDBTask(
      FROM_HERE,
      Bind(&TokenCacheWebDataBackend::ClearTokens, token_backend_));
}

void TokenCacheWebData::SaveTokens(
    std::vector<scoped_refptr<const AuthToken>> tokens) {
  wdbs_->ScheduleDBTask(FROM_HERE,
                        Bind(&TokenCacheWebDataBackend::SaveTokens,
                             token_backend_, tokens));
}

// Null on failure. Success is WDResult<std::vector<scoped_refptr<const
// AuthToken>>>
WebDataServiceBase::Handle TokenCacheWebData::LoadTokens(
    WebDataServiceConsumer* consumer) {
  return wdbs_->ScheduleDBTaskWithResult(
      FROM_HERE,
      base::Bind(&TokenCacheWebDataBackend::LoadTokens, token_backend_),
      consumer);
}

}  // namespace oauth2
}  // namespace opera
