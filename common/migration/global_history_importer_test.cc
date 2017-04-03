// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include <fstream>
#include <string>

#include "base/files/file_util.h"
#include "common/migration/global_history_importer.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/tools/test_path_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using opera::common::migration::ut::MigrationResultListenerMock;
using ::testing::Test;

namespace opera {
namespace common {
namespace migration {

class GlobalHistoryImporterTest : public Test {
 public:
  GlobalHistoryImporterTest() : history_items_(NULL) {}

 protected:
  const GlobalHistoryItems* history_items_;

  friend class TestGlobalHistoryReceiver;
};

class TestGlobalHistoryReceiver : public GlobalHistoryReceiver {
 public:
  explicit TestGlobalHistoryReceiver(
      GlobalHistoryImporterTest& global_history_importer_test)
      : global_history_importer_test_(global_history_importer_test) {}

  void OnNewHistoryItems(const GlobalHistoryItems& items,
      scoped_refptr<MigrationResultListener> listener) override {
    global_history_importer_test_.history_items_ = &items;
    listener->OnImportFinished(opera::HISTORY, true);
  }

 private:
  ~TestGlobalHistoryReceiver() override {}
  GlobalHistoryImporterTest& global_history_importer_test_;
};

TEST_F(GlobalHistoryImporterTest, ImportSimple) {
  std::string input_string("Title\nhttp://url\n1351610186\n-1\n");
  std::unique_ptr<std::istringstream> input(new std::istringstream(input_string));

  scoped_refptr<GlobalHistoryImporter> importer =
      new GlobalHistoryImporter(make_scoped_refptr(
          new TestGlobalHistoryReceiver(*this)));

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::HISTORY, true));
  importer->Import(std::move(input), migration_result_listener);
  ASSERT_FALSE(history_items_ == NULL);

  const GlobalHistoryItems& items = *history_items_;

  ASSERT_EQ(1u, items.size());

  GlobalHistoryItem item = items[0];

  EXPECT_EQ("Title", item.title_);
  EXPECT_EQ("http://url", item.url_);
  EXPECT_EQ(1351610186, item.visit_time_);
  EXPECT_EQ(-1, item.average_visit_interval_);
}

TEST_F(GlobalHistoryImporterTest, ImportCorrupted) {
  std::string input_string("Title\nhttp://url\nnan\n-1\n");
  std::unique_ptr<std::istringstream> input(new std::istringstream(input_string));

  scoped_refptr<GlobalHistoryImporter> importer =
      new GlobalHistoryImporter(make_scoped_refptr(
          new TestGlobalHistoryReceiver(*this)));

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::HISTORY, false));
  importer->Import(std::move(input), migration_result_listener);
}

TEST_F(GlobalHistoryImporterTest, ParseGoodGlobalHistoryFile) {
  base::FilePath history_path =
      ut::GetTestDataDir().AppendASCII("global_history.dat");

  std::unique_ptr<std::ifstream> history_file(
      new std::ifstream(history_path.value().c_str()));

  ASSERT_TRUE(history_file->is_open());
  ASSERT_FALSE(history_file->fail());

  scoped_refptr<GlobalHistoryImporter> importer =
      new GlobalHistoryImporter(make_scoped_refptr(
          new TestGlobalHistoryReceiver(*this)));

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::HISTORY, true));
  importer->Import(std::move(history_file), migration_result_listener);
  ASSERT_FALSE(history_items_ == NULL);

  const GlobalHistoryItems& items = *history_items_;

  ASSERT_EQ(4u, items.size());

  EXPECT_EQ("Standard Allegro.", items[0].title_);
  EXPECT_EQ("http://uslugi.allegro.pl/sa/", items[0].url_);
  EXPECT_EQ(1352905643, items[0].visit_time_);
  EXPECT_EQ(-1, items[0].average_visit_interval_);

  EXPECT_EQ("Allegro.", items[1].title_);
  EXPECT_EQ("http://allegro.pl/", items[1].url_);
  EXPECT_EQ(1352905644, items[1].visit_time_);
  EXPECT_EQ(227775, items[1].average_visit_interval_);

  EXPECT_EQ("Onet", items[2].title_);
  EXPECT_EQ("http://www.onet.pl/", items[2].url_);
  EXPECT_EQ(1352905644, items[2].visit_time_);
  EXPECT_EQ(1252798, items[2].average_visit_interval_);

  EXPECT_EQ("http://pages.ebay.com/topratedsellers/index.html",
            items[3].title_);
  EXPECT_EQ("http://pages.ebay.com/topratedsellers/index.html",
            items[3].url_);
  EXPECT_EQ(1352905742, items[3].visit_time_);
  EXPECT_EQ(-1, items[3].average_visit_interval_);
}

TEST_F(GlobalHistoryImporterTest, ParseCorruptedHistoryFile) {
  base::FilePath history_path =
      ut::GetTestDataDir().AppendASCII("global_history.dat");

  std::ifstream history_file(history_path.value().c_str());

  ASSERT_TRUE(history_file.is_open());
  ASSERT_FALSE(history_file.fail());

  std::ostringstream output_string_stream;

  // Make the file corrupted by simply skipping two last lines.
  std::string line;
  for (int i = 0; i < 14; i++) {
    ASSERT_TRUE(std::getline(history_file, line));
    output_string_stream << line << std::endl;
  }

  std::string input_string = output_string_stream.str();
  size_t first_occurence = input_string.find("index.html");
  size_t last_occurence = input_string.rfind("index.html");

  EXPECT_NE(input_string.npos, first_occurence);
  EXPECT_NE(input_string.npos, last_occurence);
  EXPECT_NE(first_occurence, last_occurence);

  std::unique_ptr<std::istringstream> input(
      new std::istringstream(input_string));

  scoped_refptr<GlobalHistoryImporter> importer =
      new GlobalHistoryImporter(make_scoped_refptr(
          new TestGlobalHistoryReceiver(*this)));

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::HISTORY, false));
  importer->Import(std::move(input), migration_result_listener);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
