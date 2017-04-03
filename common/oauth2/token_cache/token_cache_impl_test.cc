// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <memory>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#if defined(OS_MACOSX)
#include "components/os_crypt/os_crypt.h"
#endif  // OS_MACOSX
#include "components/webdata/common/web_database.h"
#include "content/public/test/test_browser_thread.h"
#include "sql/statement.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/token_cache/testing_webdata_client.h"
#include "common/oauth2/token_cache/token_cache_impl.h"
#include "common/oauth2/token_cache/token_cache_table.h"
#include "common/oauth2/test/testing_constants.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

class TokenCacheImplTest : public testing::Test {
 public:
  TokenCacheImplTest() {}

  ~TokenCacheImplTest() override {}

 protected:
  void SetUp() override {
#if defined(OS_MACOSX)
    OSCrypt::UseMockKeychain(true);
#endif
    testing_webdata_client_.reset(new TestingWebdataClient);
    token_cache_.reset(new TokenCacheImpl(nullptr));
    token_cache_->Initialize(testing_webdata_client_.get());
  }

  void TearDown() override {
    testing_webdata_client_.reset();
    base::RunLoop().RunUntilIdle();
  }

  base::MessageLoopForUI message_loop_;
  std::unique_ptr<TestingWebdataClient> testing_webdata_client_;
  std::unique_ptr<TokenCacheImpl> token_cache_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TokenCacheImplTest);
};

TEST_F(TokenCacheImplTest, DiagnosticSnapshotContents) {
  token_cache_->PutToken(test::kValidToken1);

  std::unique_ptr<base::DictionaryValue> expected_snapshot(
      new base::DictionaryValue);
  std::unique_ptr<base::ListValue> tokens(new base::ListValue);
  std::unique_ptr<base::DictionaryValue> token1_detail(
      new base::DictionaryValue);
  token1_detail->SetString("client_name", test::kMockClientName1);
  token1_detail->SetDouble("expiration_time",
                           test::kMockValidExpirationTime.ToJsTime());
  token1_detail->SetString("scopes", test::kMockScopeSet1.encode());
  tokens->Append(std::move(token1_detail));
  expected_snapshot->Set("tokens", std::move(tokens));

  EXPECT_EQ("token_cache", token_cache_->GetDiagnosticName());
  const auto& actual = token_cache_->GetDiagnosticSnapshot();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(expected_snapshot->Equals(actual.get()))
      << "Actual: " << *actual << "Expected: " << *expected_snapshot;
}

TEST_F(TokenCacheImplTest, PutTokenGetTokenWorks) {
  const auto put_token = test::kValidToken1;
  token_cache_->PutToken(put_token);
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());

  const auto got_token =
      token_cache_->GetToken(test::kMockClientName1, test::kMockScopeSet1);
  ASSERT_EQ(got_token, put_token);
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());
}

TEST_F(TokenCacheImplTest, EvictTokenWorks) {
  const auto put_token = test::kValidToken1;
  token_cache_->PutToken(put_token);
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());

  auto got_token =
      token_cache_->GetToken(test::kMockClientName1, test::kMockScopeSet1);
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());
  ASSERT_EQ(got_token, put_token);

  token_cache_->EvictToken(put_token);
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());

  got_token =
      token_cache_->GetToken(test::kMockClientName1, test::kMockScopeSet1);
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());
  ASSERT_EQ(nullptr, got_token.get());
}

TEST_F(TokenCacheImplTest, ClearWorks) {
  const auto put_token = test::kValidToken1;
  token_cache_->PutToken(put_token);
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());

  auto got_token =
      token_cache_->GetToken(test::kMockClientName1, test::kMockScopeSet1);
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());
  ASSERT_EQ(got_token, put_token);

  token_cache_->Clear();
  EXPECT_EQ(1, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());

  got_token =
      token_cache_->GetToken(test::kMockClientName1, test::kMockScopeSet1);
  EXPECT_EQ(1, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());
  ASSERT_EQ(nullptr, got_token.get());
}

TEST_F(TokenCacheImplTest, StoreWorks) {
  token_cache_->PutToken(test::kValidToken1);
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(0, testing_webdata_client_->save_count());

  token_cache_->Store();
  EXPECT_EQ(1, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(1, testing_webdata_client_->save_count());
  auto testing_tokens = testing_webdata_client_->GetTestingTokens();
  ASSERT_EQ(1U, testing_tokens.size());
  const auto token0 = testing_tokens.at(0);
  EXPECT_EQ(*test::kValidToken1, *token0);
}

TEST_F(TokenCacheImplTest, LoadWorks) {
  testing_webdata_client_->SaveTokens({test::kValidToken1});

  base::WaitableEvent done(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                           base::WaitableEvent::InitialState::NOT_SIGNALED);
  token_cache_->Load(
      base::Bind(&base::WaitableEvent::Signal, base::Unretained(&done)));
  base::RunLoop().RunUntilIdle();
  done.Wait();
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(1, testing_webdata_client_->load_count());
  EXPECT_EQ(1, testing_webdata_client_->save_count());

  const auto token =
      token_cache_->GetToken(test::kMockClientName1, test::kMockScopeSet1);
  EXPECT_EQ(*test::kValidToken1, *token);
}

TEST_F(TokenCacheImplTest, DontStoreExpired) {
  token_cache_->PutToken(test::kValidToken1);
  token_cache_->PutToken(test::kExpiredToken);
  token_cache_->Store();
  EXPECT_EQ(1, testing_webdata_client_->clear_count());
  EXPECT_EQ(0, testing_webdata_client_->load_count());
  EXPECT_EQ(1, testing_webdata_client_->save_count());

  auto testing_tokens = testing_webdata_client_->GetTestingTokens();
  ASSERT_EQ(1U, testing_tokens.size());
  const auto token0 = testing_tokens.at(0);
  EXPECT_EQ(*test::kValidToken1, *token0);
}

TEST_F(TokenCacheImplTest, DontLoadExpired) {
  testing_webdata_client_->SaveTokens(
      {test::kValidToken1, test::kExpiredToken});

  base::WaitableEvent done(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                           base::WaitableEvent::InitialState::NOT_SIGNALED);
  token_cache_->Load(
      base::Bind(&base::WaitableEvent::Signal, base::Unretained(&done)));
  base::RunLoop().RunUntilIdle();
  done.Wait();
  EXPECT_EQ(0, testing_webdata_client_->clear_count());
  EXPECT_EQ(1, testing_webdata_client_->load_count());
  EXPECT_EQ(1, testing_webdata_client_->save_count());

  const auto token =
      token_cache_->GetToken(test::kMockClientName1, test::kMockScopeSet1);
  EXPECT_EQ(*test::kValidToken1, *token);
}

}  // namespace oauth2
}  // namespace opera
