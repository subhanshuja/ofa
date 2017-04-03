// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <vector>

#include "base/bind.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/os_crypt/os_crypt.h"
#include "components/webdata/common/web_database_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/token_cache/token_cache_table.h"
#include "common/oauth2/test/testing_constants.h"
#include "common/oauth2/token_cache/token_cache_webdata.h"

namespace opera {
namespace oauth2 {
namespace {

template <class T>
class TokenWebDataServiceConsumer : public WebDataServiceConsumer {
 public:
  explicit TokenWebDataServiceConsumer(base::RunLoop* run_loop)
      : handle_(0), run_loop_(run_loop) {}
  virtual ~TokenWebDataServiceConsumer() {}

  virtual void OnWebDataServiceRequestDone(WebDataServiceBase::Handle handle,
                                           const WDTypedResult* result) {
    handle_ = handle;
    const WDResult<T>* wrapped_result = static_cast<const WDResult<T>*>(result);
    result_ = wrapped_result->GetValue();

    run_loop_->QuitWhenIdle();
  }

  WebDataServiceBase::Handle handle() { return handle_; }
  T& result() { return result_; }

 private:
  WebDataServiceBase::Handle handle_;
  base::RunLoop* run_loop_;
  T result_;
  DISALLOW_COPY_AND_ASSIGN(TokenWebDataServiceConsumer);
};
}  // namespace

ACTION_P(SignalEvent, event) {
  event->Signal();
}

class TokenWebDataServiceTest : public testing::Test {
 public:
  TokenWebDataServiceTest() {}

 protected:
  void SetUp() override {
#if defined(OS_MACOSX)
    OSCrypt::UseMockKeychain(true);
#endif
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    base::FilePath path = temp_dir_.GetPath().AppendASCII("TestWebDB");
    wdbs_ = new WebDatabaseService(path, base::ThreadTaskRunnerHandle::Get(),
                                   message_loop_.task_runner());
    wdbs_->AddTable(base::WrapUnique(new TokenCacheTable));
    wdbs_->LoadDatabase();
    wds_ = new TokenCacheWebData(
        wdbs_, base::ThreadTaskRunnerHandle::Get(), message_loop_.task_runner(),
        WebDataServiceBase::ProfileErrorCallback());
    wds_->Init();
  }

  void TearDown() override {
    wds_->ShutdownOnUIThread();
    wdbs_->ShutdownDatabase();
    wds_ = NULL;
    wdbs_ = NULL;
    WaitForDatabase();

    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure());
    run_loop.Run();
  }

  void WaitForDatabase() {
    base::WaitableEvent done(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                             base::WaitableEvent::InitialState::NOT_SIGNALED);
    message_loop_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&base::WaitableEvent::Signal, base::Unretained(&done)));
    base::RunLoop().RunUntilIdle();
    done.Wait();
  }

  base::MessageLoopForUI message_loop_;
  scoped_refptr<TokenCacheWebData> wds_;
  scoped_refptr<WebDatabaseService> wdbs_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(TokenWebDataServiceTest, PreserveOneValidToken) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kValidToken1);

  wds_->SaveTokens(tokens);
  WaitForDatabase();

  base::RunLoop run_loop;
  TokenWebDataServiceConsumer<std::vector<scoped_refptr<const AuthToken>>>
      consumer(&run_loop);
  WebDataServiceBase::Handle handle;
  handle = wds_->LoadTokens(&consumer);

  run_loop.Run();

  EXPECT_EQ(handle, consumer.handle());
  ASSERT_EQ(1U, consumer.result().size());

  EXPECT_EQ(*test::kValidToken1, *consumer.result()[0])
      << test::kValidToken1->GetDiagnosticDescription() << " vs "
      << consumer.result()[0]->GetDiagnosticDescription();
}

TEST_F(TokenWebDataServiceTest, PreserveTwoValidTokens) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kValidToken1);
  tokens.push_back(test::kValidToken2);

  wds_->SaveTokens(tokens);
  WaitForDatabase();

  base::RunLoop run_loop;
  TokenWebDataServiceConsumer<std::vector<scoped_refptr<const AuthToken>>>
      consumer(&run_loop);
  WebDataServiceBase::Handle handle;
  handle = wds_->LoadTokens(&consumer);

  run_loop.Run();

  EXPECT_EQ(handle, consumer.handle());
  ASSERT_EQ(2U, consumer.result().size());

  EXPECT_EQ(*test::kValidToken1, *consumer.result()[0])
    << test::kValidToken1->GetDiagnosticDescription() << " vs "
      << consumer.result()[0]->GetDiagnosticDescription();
  EXPECT_EQ(*test::kValidToken2, *consumer.result()[1])
    << test::kValidToken2->GetDiagnosticDescription() << " vs "
    << consumer.result()[0]->GetDiagnosticDescription();
}

TEST_F(TokenWebDataServiceTest, DontPreserveInvalidToken) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kInvalidToken);
  EXPECT_FALSE(tokens.front()->is_valid());
  EXPECT_FALSE(tokens.front()->is_expired());

  wds_->SaveTokens(tokens);
  WaitForDatabase();

  base::RunLoop run_loop;
  TokenWebDataServiceConsumer<std::vector<scoped_refptr<const AuthToken>>>
      consumer(&run_loop);
  WebDataServiceBase::Handle handle;
  handle = wds_->LoadTokens(&consumer);

  run_loop.Run();

  EXPECT_EQ(handle, consumer.handle());
  ASSERT_EQ(0U, consumer.result().size());
}

TEST_F(TokenWebDataServiceTest, DontPreserveExpiredToken) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kExpiredToken);
  EXPECT_TRUE(tokens.front()->is_valid());
  EXPECT_TRUE(tokens.front()->is_expired());

  wds_->SaveTokens(tokens);
  WaitForDatabase();

  base::RunLoop run_loop;
  TokenWebDataServiceConsumer<std::vector<scoped_refptr<const AuthToken>>>
      consumer(&run_loop);
  WebDataServiceBase::Handle handle;
  handle = wds_->LoadTokens(&consumer);

  run_loop.Run();

  EXPECT_EQ(handle, consumer.handle());
  ASSERT_EQ(0U, consumer.result().size());
}

TEST_F(TokenWebDataServiceTest, OnlyPreserveValidTokens) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kValidToken1);
  tokens.push_back(test::kValidToken2);
  tokens.push_back(test::kInvalidToken);
  tokens.push_back(test::kExpiredToken);
  wds_->SaveTokens(tokens);
  WaitForDatabase();

  base::RunLoop run_loop;
  TokenWebDataServiceConsumer<std::vector<scoped_refptr<const AuthToken>>>
      consumer(&run_loop);
  WebDataServiceBase::Handle handle;
  handle = wds_->LoadTokens(&consumer);

  run_loop.Run();

  EXPECT_EQ(handle, consumer.handle());
  ASSERT_EQ(2U, consumer.result().size());

  EXPECT_EQ(*test::kValidToken1, *consumer.result()[0])
    << test::kValidToken1->GetDiagnosticDescription() << " vs "
      << consumer.result()[0]->GetDiagnosticDescription();
  EXPECT_EQ(*test::kValidToken2, *consumer.result()[1])
    << test::kValidToken2->GetDiagnosticDescription() << " vs "
      << consumer.result()[0]->GetDiagnosticDescription();
}

TEST_F(TokenWebDataServiceTest, ClearTokensWorks) {
  std::vector<scoped_refptr<const AuthToken>> tokens;
  tokens.push_back(test::kValidToken1);
  tokens.push_back(test::kValidToken2);
  wds_->SaveTokens(tokens);
  WaitForDatabase();

  base::RunLoop run_loop;
  TokenWebDataServiceConsumer<std::vector<scoped_refptr<const AuthToken>>>
      consumer(&run_loop);
  WebDataServiceBase::Handle handle;
  handle = wds_->LoadTokens(&consumer);
  run_loop.Run();
  EXPECT_EQ(handle, consumer.handle());
  ASSERT_EQ(2U, consumer.result().size());

  wds_->ClearTokens();
  WaitForDatabase();

  base::RunLoop run_loop2;
  TokenWebDataServiceConsumer<std::vector<scoped_refptr<const AuthToken>>>
      consumer2(&run_loop2);
  WebDataServiceBase::Handle handle2;
  handle2 = wds_->LoadTokens(&consumer2);
  run_loop2.Run();
  EXPECT_EQ(handle2, consumer2.handle());
  ASSERT_EQ(0U, consumer2.result().size());
}

}  // namespace oauth2
}  // namespace opera
