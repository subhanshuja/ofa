// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/migration_assistant.h"

#include <fstream>
#include <algorithm>
#include <vector>

#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/profile_folder_finder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using opera::common::migration::ut::MigrationResultListenerMock;
using ::testing::Test;
using ::testing::Eq;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::Return;

namespace opera {
namespace common {
namespace migration {

class MockFinder : public ProfileFolderFinder {
public:
  MOCK_CONST_METHOD2(FindFolderFor, bool(const base::FilePath& input_file,
                                         base::FilePath* suggested_folder));
};

class MigrationAssistantTest : public Test {
 public:
  MigrationAssistantTest()
    : finder_(new MockFinder()) {
  }

  void SetUp() override {
    ASSERT_TRUE(base::CreateTemporaryFile(&temporary_file_));
  }

  void TearDown() override {
    ASSERT_TRUE(base::DeleteFile(temporary_file_,
                                 false /* Not recursive */));
    temporary_file_.clear();  // Just in case
  }


  class ImporterMock : public Importer {
   public:
    void Import(
        std::unique_ptr<std::istream> input,
        scoped_refptr<MigrationResultListener> listener) override {
      ImportMock(input.get(), listener);
    }
    MOCK_METHOD2(ImportMock, void(std::istream*,
                                  scoped_refptr<MigrationResultListener>));
   private:
    virtual ~ImporterMock() {}
  };

 protected:
  base::FilePath temporary_file_;
  std::unique_ptr<MockFinder> finder_;
  base::MessageLoop message_loop_;
};

TEST_F(MigrationAssistantTest, NoImporterRegistered) {
  scoped_refptr<MigrationAssistant> assistant =
      new MigrationAssistant(message_loop_.task_runner(), finder_.get());
  scoped_refptr<MigrationResultListenerMock> listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*listener, OnImportFinished(opera::COOKIES, false));
  assistant->ImportAsync(opera::COOKIES, listener);
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();
}

MATCHER_P2(IstreamContains, data, datalen, "") {
  // arg is a istream*
  std::vector<char> buffer(datalen);
  arg->read(&buffer[0], datalen);
  return std::equal(buffer.begin(), buffer.end(), data);
}

TEST_F(MigrationAssistantTest, ImporterRegistered) {
  std::fstream data_file(temporary_file_.value().c_str(),
                        std::ios_base::out | std::ios_base::binary);
  ASSERT_TRUE(data_file.is_open());
  ASSERT_FALSE(data_file.fail());
  const char data[] = "someDataHere";
  data_file.write(data, strlen(data));
  data_file.close();  // We're closing, assistant will open again in input mode

  scoped_refptr<MigrationResultListenerMock> listener =
      new MigrationResultListenerMock();

  scoped_refptr<ImporterMock> importer_mock = new ImporterMock();
  EXPECT_CALL(*importer_mock, ImportMock(IstreamContains(data, strlen(data)),
                                         Eq(listener)));

  scoped_refptr<MigrationAssistant> assistant =
      new MigrationAssistant(message_loop_.task_runner(), finder_.get());

  base::FilePath temporary_file_base_ = temporary_file_.BaseName();
  base::FilePath temporary_file_dir = temporary_file_.DirName();

  assistant->RegisterImporter(importer_mock,
                              opera::MOCK_FOR_TESTING,
                              temporary_file_base_,
                              false);
  EXPECT_CALL(*finder_, FindFolderFor(temporary_file_base_, ::testing::_)).
      WillOnce(DoAll(SetArgPointee<1>(temporary_file_dir), Return(true)));
  assistant->ImportAsync(opera::MOCK_FOR_TESTING, listener);
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();
}

}  // namespace migration
}  // namespace common
}  // namespace opera
