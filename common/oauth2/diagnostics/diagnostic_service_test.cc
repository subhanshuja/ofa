// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/diagnostics/diagnostic_service.h"

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "components/sync/api/time.h"

#include "common/oauth2/diagnostics/diagnostic_supplier.h"
#include "common/oauth2/test/testing_constants.h"

namespace opera {
namespace oauth2 {
namespace {

using ::testing::_;

class TestingDiagnosticSupplier : public DiagnosticSupplier {
 public:
  TestingDiagnosticSupplier() {}
  ~TestingDiagnosticSupplier() override {}

  void SetDiagnosticState(std::unique_ptr<base::DictionaryValue> state) {
    state_ = std::move(state);
  }

  std::string GetDiagnosticName() const override { return "supplier1"; }
  std::unique_ptr<base::DictionaryValue> GetDiagnosticSnapshot() override {
    return state_->CreateDeepCopy();
  }

 protected:
  std::unique_ptr<base::DictionaryValue> state_;
};

class TestingDiagnosticObserver : public DiagnosticService::Observer {
 public:
  TestingDiagnosticObserver() : state_update_count_(0) {}
  ~TestingDiagnosticObserver() override {}
  void OnStateUpdate() override { state_update_count_++; }
  int state_update_count() const { return state_update_count_; }

 private:
  int state_update_count_;
};

class DiagnosticServiceTest : public ::testing::Test {
 public:
  DiagnosticServiceTest() {}
  ~DiagnosticServiceTest() override {}

  void SetUp() override { service_.reset(new DiagnosticService(3u)); }
  void TearDown() override {}

 protected:
  TestingDiagnosticSupplier supplier_;
  std::unique_ptr<DiagnosticService> service_;
};

}  // namespace

TEST_F(DiagnosticServiceTest, RegisterSupplier) {
  std::unique_ptr<base::DictionaryValue> state(new base::DictionaryValue);
  state->SetString("key", "value");
  supplier_.SetDiagnosticState(state->CreateDeepCopy());

  service_->TakeSnapshot();
  ASSERT_EQ(0u, service_->GetAllSnapshots().size());

  service_->AddSupplier(&supplier_);
  service_->TakeSnapshot();
  const auto snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(1u, snapshots.size());
  std::unique_ptr<base::DictionaryValue> expected(new base::DictionaryValue);
  expected->Set("supplier1", state->CreateDeepCopy());
  auto actual = snapshots.at(0)->state.get();
  ASSERT_NE(nullptr, actual);
  EXPECT_TRUE(expected->Equals(actual)) << "Actual: " << *actual
                                        << "Expected: " << *expected;
}

TEST_F(DiagnosticServiceTest, UnRegisterSupplier) {
  std::unique_ptr<base::DictionaryValue> state(new base::DictionaryValue);
  state->SetString("key", "value");
  supplier_.SetDiagnosticState(state->CreateDeepCopy());

  service_->AddSupplier(&supplier_);
  service_->TakeSnapshot();
  auto snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(1u, snapshots.size());
  std::unique_ptr<base::DictionaryValue> expected(new base::DictionaryValue);
  expected->Set("supplier1", state->CreateDeepCopy());
  auto actual = snapshots.at(0)->state.get();
  ASSERT_NE(nullptr, actual);
  EXPECT_TRUE(expected->Equals(actual)) << "Actual: " << *actual
                                        << "Expected: " << *expected;

  service_->RemoveSupplier(&supplier_);
  service_->TakeSnapshot();
  snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(1u, snapshots.size());
}

TEST_F(DiagnosticServiceTest, ObserverNotified) {
  TestingDiagnosticObserver observer;
  std::unique_ptr<base::DictionaryValue> state(new base::DictionaryValue);
  state->SetString("key", "value");
  supplier_.SetDiagnosticState(state->CreateDeepCopy());

  service_->AddObserver(&observer);
  service_->AddSupplier(&supplier_);
  ASSERT_EQ(0, observer.state_update_count());
  service_->TakeSnapshot();
  const auto snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(1u, snapshots.size());
  ASSERT_EQ(1, observer.state_update_count());
}

TEST_F(DiagnosticServiceTest, IngoresEmptySnapshot) {
  std::unique_ptr<base::DictionaryValue> state(new base::DictionaryValue);
  supplier_.SetDiagnosticState(state->CreateDeepCopy());

  service_->AddSupplier(&supplier_);
  service_->TakeSnapshot();
  const auto snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(0u, snapshots.size());
}

TEST_F(DiagnosticServiceTest, IngoresSameSnapshot) {
  service_->AddSupplier(&supplier_);

  std::unique_ptr<base::DictionaryValue> state(new base::DictionaryValue);
  state->SetString("key", "value");
  supplier_.SetDiagnosticState(state->CreateDeepCopy());
  service_->TakeSnapshot();
  auto snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(1u, snapshots.size());

  supplier_.SetDiagnosticState(state->CreateDeepCopy());
  service_->TakeSnapshot();
  snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(1u, snapshots.size());
}

TEST_F(DiagnosticServiceTest, RecondsDifferentSnapshot) {
  service_->AddSupplier(&supplier_);

  std::unique_ptr<base::DictionaryValue> state(new base::DictionaryValue);
  state->SetString("key", "value1");
  supplier_.SetDiagnosticState(state->CreateDeepCopy());
  service_->TakeSnapshot();
  auto snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(1u, snapshots.size());

  state.reset(new base::DictionaryValue);
  state->SetString("key", "value2");
  supplier_.SetDiagnosticState(state->CreateDeepCopy());
  service_->TakeSnapshot();
  snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(2u, snapshots.size());
}

TEST_F(DiagnosticServiceTest, NewestSnapshotOnTop) {
  service_->AddSupplier(&supplier_);

  std::unique_ptr<base::DictionaryValue> state1(new base::DictionaryValue);
  state1->SetString("key", "value1");
  supplier_.SetDiagnosticState(state1->CreateDeepCopy());
  service_->TakeSnapshot();

  std::unique_ptr<base::DictionaryValue> state2(new base::DictionaryValue);
  state2->SetString("key", "value2");
  supplier_.SetDiagnosticState(state2->CreateDeepCopy());
  service_->TakeSnapshot();

  auto snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(2u, snapshots.size());

  std::unique_ptr<base::DictionaryValue> expected(new base::DictionaryValue);
  expected->Set("supplier1", state2->CreateDeepCopy());
  auto actual = snapshots.at(0)->state.get();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(expected->Equals(actual)) << "Actual: " << *actual
                                        << "Expected: " << *expected;

  expected.reset(new base::DictionaryValue);
  expected->Set("supplier1", state1->CreateDeepCopy());
  actual = snapshots.at(1)->state.get();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(expected->Equals(actual)) << "Actual: " << *actual
                                        << "Expected: " << *expected;
}

TEST_F(DiagnosticServiceTest, UpToMaxSnapshots) {
  service_->AddSupplier(&supplier_);

  std::unique_ptr<base::DictionaryValue> state1(new base::DictionaryValue);
  state1->SetString("key", "value1");
  supplier_.SetDiagnosticState(state1->CreateDeepCopy());
  service_->TakeSnapshot();

  std::unique_ptr<base::DictionaryValue> state2(new base::DictionaryValue);
  state2->SetString("key", "value2");
  supplier_.SetDiagnosticState(state2->CreateDeepCopy());
  service_->TakeSnapshot();

  std::unique_ptr<base::DictionaryValue> state3(new base::DictionaryValue);
  state3->SetString("key", "value3");
  supplier_.SetDiagnosticState(state3->CreateDeepCopy());
  service_->TakeSnapshot();

  std::unique_ptr<base::DictionaryValue> state4(new base::DictionaryValue);
  state4->SetString("key", "value4");
  supplier_.SetDiagnosticState(state4->CreateDeepCopy());
  service_->TakeSnapshot();

  auto snapshots = service_->GetAllSnapshots();
  ASSERT_EQ(3u, snapshots.size());

  std::unique_ptr<base::DictionaryValue> expected(new base::DictionaryValue);
  expected->Set("supplier1", state4->CreateDeepCopy());
  auto actual = snapshots.at(0)->state.get();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(expected->Equals(actual)) << "Actual: " << *actual
                                        << "Expected: " << *expected;

  expected.reset(new base::DictionaryValue);
  expected->Set("supplier1", state3->CreateDeepCopy());
  actual = snapshots.at(1)->state.get();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(expected->Equals(actual)) << "Actual: " << *actual
                                        << "Expected: " << *expected;

  expected.reset(new base::DictionaryValue);
  expected->Set("supplier1", state2->CreateDeepCopy());
  actual = snapshots.at(2)->state.get();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(expected->Equals(actual)) << "Actual: " << *actual
                                        << "Expected: " << *expected;
}

TEST_F(DiagnosticServiceTest, ExtendsTimesAsNeeded) {
  service_->AddSupplier(&supplier_);

  std::unique_ptr<base::DictionaryValue> state(new base::DictionaryValue);
  state->SetString("key", "value1");
  state->SetDouble("some_time", 12345);
  state->SetDouble("some_time_wrong", 12345);
  supplier_.SetDiagnosticState(state->CreateDeepCopy());
  service_->TakeSnapshot();
  auto snapshots = service_->GetAllSnapshotsWithFormattedTimes();
  ASSERT_EQ(1u, snapshots.size());

  std::unique_ptr<base::DictionaryValue> expected(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> state1(new base::DictionaryValue);

  state1->SetString("key", "value1");
  state1->SetDouble("some_time", 12345);
  state1->SetString("some_time_str",
                    syncer::GetTimeDebugString(base::Time::FromJsTime(12345)));
  state1->SetDouble("some_time_wrong", 12345);

  expected->Set("supplier1", std::move(state1));
  auto actual = snapshots.at(0)->state.get();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(expected->Equals(actual)) << "Actual: " << *actual
                                        << "Expected: " << *expected;
}

TEST_F(DiagnosticServiceTest, ExtendsTimesAsNeededInList) {
  service_->AddSupplier(&supplier_);

  std::unique_ptr<base::DictionaryValue> state(new base::DictionaryValue);
  std::unique_ptr<base::ListValue> list(new base::ListValue);
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString("key", "value1");
  dict->SetDouble("some_time", 12345);
  dict->SetDouble("some_time_wrong", 12345);
  list->Append(dict->CreateDeepCopy());
  list->Append(dict->CreateDeepCopy());
  state->Set("some_list", std::move(list));
  supplier_.SetDiagnosticState(state->CreateDeepCopy());

  service_->TakeSnapshot();
  auto snapshots = service_->GetAllSnapshotsWithFormattedTimes();
  ASSERT_EQ(1u, snapshots.size());

  std::unique_ptr<base::DictionaryValue> expected(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> state1(new base::DictionaryValue);
  std::unique_ptr<base::ListValue> expected_list(new base::ListValue);
  std::unique_ptr<base::DictionaryValue> expected_dict(
      new base::DictionaryValue);

  expected_dict->SetString("key", "value1");
  expected_dict->SetDouble("some_time", 12345);
  expected_dict->SetString("some_time_str", syncer::GetTimeDebugString(
                                                base::Time::FromJsTime(12345)));
  expected_dict->SetDouble("some_time_wrong", 12345);
  expected_list->Append(expected_dict->CreateDeepCopy());
  expected_list->Append(expected_dict->CreateDeepCopy());
  state1->Set("some_list", std::move(expected_list));

  expected->Set("supplier1", std::move(state1));
  auto actual = snapshots.at(0)->state.get();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(expected->Equals(actual)) << "Actual: " << *actual
                                        << "Expected: " << *expected;
}

}  // namespace oauth2
}  // namespace opera
