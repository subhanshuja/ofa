// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_TABLE_H_
#define COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_TABLE_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "components/webdata/common/web_database_table.h"

#include "common/oauth2/util/token.h"

class WebDatabase;

namespace opera {
namespace oauth2 {

class TokenCacheTable : public WebDatabaseTable {
 public:
  TokenCacheTable();
  ~TokenCacheTable() override;

  // Retrieves the TokenCacheTable* owned by |db|.
  static TokenCacheTable* FromWebDatabase(WebDatabase* db);

  // WebDatabaseTable:
  WebDatabaseTable::TypeKey GetTypeKey() const override;
  bool CreateTablesIfNecessary() override;
  bool IsSyncable() override;
  bool MigrateToVersion(int version, bool* update_compatible_version) override;

  bool SaveTokens(std::vector<scoped_refptr<const AuthToken>> tokens);
  bool LoadTokens(std::vector<scoped_refptr<const AuthToken>>& tokens);
  bool ClearTokens();

 private:
   bool SaveToken(scoped_refptr<const AuthToken> token);

  bool CreateAccessTokensTable();
  DISALLOW_COPY_AND_ASSIGN(TokenCacheTable);
};

}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_TABLE_H_
