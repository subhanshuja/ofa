// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <cstdint>
#include <memory>

#include "base/files/scoped_temp_dir.h"
#if defined(OS_MACOSX)
#include "components/os_crypt/os_crypt.h"
#endif  // OS_MACOSX
#include "components/webdata/common/web_database.h"
#include "sql/statement.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/token_cache/token_cache_table.h"
#include "common/oauth2/test/testing_constants.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {
namespace {

struct SQLTokenData {
  std::string client_name;
  std::string token;
  std::string encoded_scopes;
  int64_t expiration_date;
};

std::vector<SQLTokenData> DirectlyGetTokenData(WebDatabase* db) {
  std::vector<SQLTokenData> ret;
  sql::Statement s(db->GetSQLConnection()->GetUniqueStatement(
      "SELECT client_name, encoded_scopes, token, expiration_date FROM "
      "access_tokens"));
  while (s.Step()) {
    SQLTokenData token_data;
    token_data.client_name = s.ColumnString(0);
    token_data.encoded_scopes = s.ColumnString(1);
    token_data.token = s.ColumnString(2);
    std::string encrypted_expiration_date = s.ColumnString(3);
    int64_t expiration_date_int = -1;
    if (crypt::OSDecryptInt64(encrypted_expiration_date,
                              &expiration_date_int)) {
      token_data.expiration_date = expiration_date_int;
    } else {
      token_data.expiration_date = base::Time().ToInternalValue();
    }
    ret.push_back(token_data);
  }
  return ret;
}

void DirectlySetTokenData(WebDatabase* db, std::vector<SQLTokenData> tokens) {
  for (const auto& token : tokens) {
    sql::Statement s(db->GetSQLConnection()->GetUniqueStatement(
        "INSERT INTO access_tokens (client_name, encoded_scopes, token, "
        "expiration_date) VALUES (?, ?, ?, ?)"));
    s.BindString(0, token.client_name);
    s.BindString(1, token.encoded_scopes);
    s.BindString(2, token.token);
    s.BindString(3, crypt::OSEncryptInt64(token.expiration_date));
    ASSERT_TRUE(s.Run());
  }
}
}  // namespace

class TokenCacheTableTest : public testing::Test {
 public:
  TokenCacheTableTest() {}
  ~TokenCacheTableTest() override {}

 protected:
  void SetUp() override {
#if defined(OS_MACOSX)
    OSCrypt::UseMockKeychain(true);
#endif
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    file_ = temp_dir_.GetPath().AppendASCII("TestWebDatabase");
    table_.reset(new TokenCacheTable);
    db_.reset(new WebDatabase);
    db_->AddTable(table_.get());
    ASSERT_EQ(sql::INIT_OK, db_->Init(file_));
  }

  base::FilePath file_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<TokenCacheTable> table_;
  std::unique_ptr<WebDatabase> db_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TokenCacheTableTest);
};

TEST_F(TokenCacheTableTest, IsSyncable) {
  EXPECT_FALSE(table_->IsSyncable());
}

TEST_F(TokenCacheTableTest, LoadTokensEncrypted) {
  std::vector<SQLTokenData> tokens;

  tokens.push_back(
      SQLTokenData{crypt::OSEncrypt(test::kMockClientName1),
                   crypt::OSEncrypt(test::kMockTokenValue1),
                   crypt::OSEncrypt(test::kMockScopeSet1.encode()),
                   test::kMockValidExpirationTime.ToInternalValue()});
  DirectlySetTokenData(db_.get(), tokens);

  std::vector<scoped_refptr<const AuthToken>> loaded;
  ASSERT_TRUE(table_->LoadTokens(loaded));

  ASSERT_EQ(1U, loaded.size());
  const auto& token = loaded.front();
  EXPECT_EQ(test::kMockClientName1, token->client_name());
  EXPECT_EQ(test::kMockTokenValue1, token->token());
  EXPECT_EQ(test::kMockScopeSet1, token->scopes());
  EXPECT_EQ(test::kMockValidExpirationTime, token->expiration_time());
}

TEST_F(TokenCacheTableTest, LoadTokensUnencrypted) {
  std::vector<SQLTokenData> tokens;

  tokens.push_back(
      SQLTokenData{test::kMockClientName1, test::kMockTokenValue1,
                   test::kMockScopeSet1.encode(),
                   test::kMockValidExpirationTime.ToInternalValue()});
  DirectlySetTokenData(db_.get(), tokens);

  std::vector<scoped_refptr<const AuthToken>> loaded;
  ASSERT_TRUE(table_->LoadTokens(loaded));

  ASSERT_EQ(0U, loaded.size());
}

TEST_F(TokenCacheTableTest, LoadTokensExpired) {
  std::vector<SQLTokenData> tokens;

  tokens.push_back(
      SQLTokenData{crypt::OSEncrypt(test::kMockClientName1),
                   crypt::OSEncrypt(test::kMockTokenValue1),
                   crypt::OSEncrypt(test::kMockScopeSet1.encode()),
                   test::kMockExpiredExpirationTime.ToInternalValue()});
  DirectlySetTokenData(db_.get(), tokens);

  std::vector<scoped_refptr<const AuthToken>> loaded;
  ASSERT_TRUE(table_->LoadTokens(loaded));

  ASSERT_EQ(0U, loaded.size());
}

TEST_F(TokenCacheTableTest, LoadTokensInvalid) {
  std::vector<SQLTokenData> tokens;

  tokens.push_back(SQLTokenData{
      crypt::OSEncrypt(std::string()), crypt::OSEncrypt(test::kMockTokenValue1),
      crypt::OSEncrypt(test::kMockScopeSet1.encode()),
      test::kMockExpiredExpirationTime.ToInternalValue()});
  DirectlySetTokenData(db_.get(), tokens);

  std::vector<scoped_refptr<const AuthToken>> loaded;
  ASSERT_TRUE(table_->LoadTokens(loaded));

  ASSERT_EQ(0U, loaded.size());
}

TEST_F(TokenCacheTableTest, SaveTokens) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kValidToken1);
  ASSERT_TRUE(table_->SaveTokens(tokens));

  std::vector<SQLTokenData> token_data = DirectlyGetTokenData(db_.get());
  ASSERT_EQ(1U, token_data.size());
}

TEST_F(TokenCacheTableTest, SaveTokensRepeated) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  // We will store repeated tokens for same client/scope combination
  // as the OS-level encryption on Windows makes protecting this with a
  // primary key useless and we'd like a consistent behaviour on all
  // platforms.
  tokens.push_back(test::kValidToken3);
  tokens.push_back(test::kValidToken4);
  ASSERT_TRUE(table_->SaveTokens(tokens));

  std::vector<SQLTokenData> token_data = DirectlyGetTokenData(db_.get());
  ASSERT_EQ(2U, token_data.size());
  EXPECT_EQ(test::kValidToken3->client_name(),
            crypt::OSDecrypt(token_data.front().client_name));
  EXPECT_EQ(test::kValidToken3->token(),
            crypt::OSDecrypt(token_data.front().token));
  EXPECT_EQ(test::kValidToken3->scopes().encode(),
            crypt::OSDecrypt(token_data.front().encoded_scopes));
  EXPECT_EQ(test::kValidToken3->expiration_time().ToInternalValue(),
            token_data.front().expiration_date);

  EXPECT_EQ(test::kValidToken4->client_name(),
            crypt::OSDecrypt(token_data.back().client_name));
  EXPECT_EQ(test::kValidToken4->token(),
            crypt::OSDecrypt(token_data.back().token));
  EXPECT_EQ(test::kValidToken4->scopes().encode(),
            crypt::OSDecrypt(token_data.back().encoded_scopes));
  EXPECT_EQ(test::kValidToken4->expiration_time().ToInternalValue(),
            token_data.back().expiration_date);
}

TEST_F(TokenCacheTableTest, SaveTokensEncrypted) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kValidToken1);
  ASSERT_TRUE(table_->SaveTokens(tokens));

  std::vector<SQLTokenData> token_data = DirectlyGetTokenData(db_.get());
  ASSERT_EQ(1U, token_data.size());

  EXPECT_EQ(test::kValidToken1->client_name(),
            crypt::OSDecrypt(token_data.front().client_name));
  EXPECT_EQ(test::kValidToken1->token(),
            crypt::OSDecrypt(token_data.front().token));
  EXPECT_EQ(test::kValidToken1->scopes().encode(),
            crypt::OSDecrypt(token_data.front().encoded_scopes));
  EXPECT_EQ(test::kValidToken1->expiration_time().ToInternalValue(),
            token_data.front().expiration_date);
}

TEST_F(TokenCacheTableTest, SaveTokensInvalid) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kInvalidToken);
  ASSERT_FALSE(table_->SaveTokens(tokens));

  std::vector<SQLTokenData> token_data = DirectlyGetTokenData(db_.get());
  ASSERT_EQ(0U, token_data.size());
}

TEST_F(TokenCacheTableTest, SaveTokensExpired) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kExpiredToken);
  ASSERT_FALSE(table_->SaveTokens(tokens));

  std::vector<SQLTokenData> token_data = DirectlyGetTokenData(db_.get());
  ASSERT_EQ(0U, token_data.size());
}

TEST_F(TokenCacheTableTest, ClearTokens) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kValidToken1);
  tokens.push_back(test::kValidToken2);
  ASSERT_TRUE(table_->SaveTokens(tokens));

  std::vector<SQLTokenData> token_data = DirectlyGetTokenData(db_.get());
  EXPECT_EQ(2U, token_data.size());

  ASSERT_TRUE(table_->ClearTokens());
  token_data = DirectlyGetTokenData(db_.get());
  ASSERT_EQ(0U, token_data.size());
}

}  // namespace oauth2
}  // namespace opera
