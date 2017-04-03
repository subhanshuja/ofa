// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_WEBDATA_H_
#define COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_WEBDATA_H_

#include <map>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/webdata/common/web_data_results.h"
#include "components/webdata/common/web_database_service.h"
#include "components/webdata/common/web_data_service_base.h"
#include "components/webdata/common/web_data_service_consumer.h"
#include "components/webdata/common/web_database.h"

#include "common/oauth2/util/token.h"

namespace base {
class SingleThreadTaskRunner;
}

class WebDataServiceConsumer;

namespace opera {
namespace oauth2 {

class AuthToken;
class TokenCacheWebDataBackend;

class TokenCacheWebData : public WebDataServiceBase {
 public:
  TokenCacheWebData(scoped_refptr<WebDatabaseService> wdbs,
                    scoped_refptr<base::SingleThreadTaskRunner> ui_thread,
                    scoped_refptr<base::SingleThreadTaskRunner> db_thread,
                    const ProfileErrorCallback& callback);

  TokenCacheWebData(scoped_refptr<base::SingleThreadTaskRunner> ui_thread,
                    scoped_refptr<base::SingleThreadTaskRunner> db_thread);

  void ClearTokens();

  // Set a token to use for a specified service.
  void SaveTokens(std::vector<scoped_refptr<const AuthToken>>);

  // Null on failure. Success is WDResult<std::vector<scoped_refptr<const
  // AuthToken>>>
  Handle LoadTokens(WebDataServiceConsumer* consumer);

 protected:
  ~TokenCacheWebData() override;

 private:
  scoped_refptr<TokenCacheWebDataBackend> token_backend_;

  DISALLOW_COPY_AND_ASSIGN(TokenCacheWebData);
};

}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_WEBDATA_H_
