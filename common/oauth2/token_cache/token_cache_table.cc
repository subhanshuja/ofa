// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/token_cache/token_cache_table.h"

#include "components/webdata/common/web_database.h"
#include "sql/statement.h"
#include "sql/transaction.h"

#include "common/oauth2/util/token.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {
namespace {
WebDatabaseTable::TypeKey GetKey() {
  // We just need a unique constant. Use the address of a static that
  // COMDAT folding won't touch in an optimizing linker.
  static int table_key = 0;
  return reinterpret_cast<void*>(&table_key);
}
}  // namespace

TokenCacheTable::TokenCacheTable() {}

TokenCacheTable::~TokenCacheTable() {}

TokenCacheTable* TokenCacheTable::FromWebDatabase(WebDatabase* db) {
  return static_cast<TokenCacheTable*>(db->GetTable(GetKey()));
}

WebDatabaseTable::TypeKey TokenCacheTable::GetTypeKey() const {
  return GetKey();
}

bool TokenCacheTable::CreateTablesIfNecessary() {
  return CreateAccessTokensTable();
}

bool TokenCacheTable::IsSyncable() {
  return false;
}

bool TokenCacheTable::MigrateToVersion(int version,
                                       bool* update_compatible_version) {
  return true;
}

bool TokenCacheTable::CreateAccessTokensTable() {
  if (!db_->DoesTableExist("access_tokens")) {
    if (!db_->Execute("CREATE TABLE access_tokens ("
                      "client_name VARCHAR, "
                      "encoded_scopes VARCHAR, "
                      "token VARCHAR, "
                      "expiration_date VARCHAR)")) {
      NOTREACHED();
      return false;
    }
  }
  return true;
}

bool TokenCacheTable::SaveTokens(
    std::vector<scoped_refptr<const AuthToken>> tokens) {
  for (const auto& token : tokens) {
    DCHECK(token);
    if (!SaveToken(token))
      return false;
  }
  return true;
}

bool TokenCacheTable::LoadTokens(
    std::vector<scoped_refptr<const AuthToken>>& tokens) {
  DCHECK(tokens.empty());

  const char* sql_statement =
      "SELECT client_name, encoded_scopes, token, expiration_date FROM "
      "access_tokens";

  sql::Statement s(db_->GetUniqueStatement(sql_statement));

  if (!s.is_valid())
    return false;

  while (s.Step()) {
    std::string client_name, encoded_scopes, token, encrypted_expiration_time;
    base::Time expiration_time;

    client_name = crypt::OSDecrypt(s.ColumnString(0));
    encoded_scopes = crypt::OSDecrypt(s.ColumnString(1));
    token = crypt::OSDecrypt(s.ColumnString(2));
    encrypted_expiration_time = s.ColumnString(3);
    int64_t expiration_time_int = -1;
    if (crypt::OSDecryptInt64(encrypted_expiration_time,
                              &expiration_time_int)) {
      expiration_time = base::Time::FromInternalValue(expiration_time_int);
    } else {
      expiration_time = base::Time::Now();
    }

    scoped_refptr<const AuthToken> access_token =
        new AuthToken(client_name, token, ScopeSet::FromEncoded(encoded_scopes),
                      expiration_time);
    if (!access_token->is_valid()) {
      VLOG(1) << "Broken token loaded, omitting.";
      continue;
    }

    if (access_token->is_expired()) {
      VLOG(1) << "Expired token loaded, omitting.";
      continue;
    }

    tokens.push_back(access_token);
  }
  return s.Succeeded();
}

bool TokenCacheTable::ClearTokens() {
  return db_->Execute("DELETE FROM access_tokens");
}

bool TokenCacheTable::SaveToken(scoped_refptr<const AuthToken> token) {
  DCHECK(token);

  if (!token->is_valid()) {
    VLOG(1) << "Won't store invalid token.";
    return false;
  }

  if (token->is_expired()) {
    VLOG(1) << "Won't store expired token.";
    return false;
  }

  const std::string encrypted_token = crypt::OSEncrypt(token->token());
  const std::string encrypted_expiration_time =
      crypt::OSEncryptInt64(token->expiration_time().ToInternalValue());
  const std::string encrypted_client_name =
      crypt::OSEncrypt(token->client_name());
  const std::string encrypted_encoded_scopes =
      crypt::OSEncrypt(token->scopes().encode());

  sql::Statement s(db_->GetUniqueStatement(
      "INSERT INTO access_tokens "
      "(token, expiration_date, client_name, encoded_scopes) "
      "VALUES (?, ?, ?, ?)"));
  s.BindString(0, encrypted_token);
  s.BindString(1, encrypted_expiration_time);
  s.BindString(2, encrypted_client_name);
  s.BindString(3, encrypted_encoded_scopes);
  return s.Run();
}

}  // namespace oauth2
}  // namespace opera
