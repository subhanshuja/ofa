// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include <fstream>
#include <string>

#include "base/files/file_util.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/search_history_importer.h"
#include "common/migration/tools/test_path_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using opera::common::migration::ut::MigrationResultListenerMock;
using ::testing::Test;

namespace opera {
namespace common {
namespace migration {

class SearchHistoryImporterTest : public Test {
 public:
  SearchHistoryImporterTest() : searches_(NULL) {}

 protected:
  const SearchQueries* searches_;

  friend class TestSearchHistoryReceiver;
};

class TestSearchHistoryReceiver : public SearchHistoryReceiver {
 public:
  explicit TestSearchHistoryReceiver(
      SearchHistoryImporterTest& search_history_importer_test)
      : search_history_importer_test_(search_history_importer_test) {}

  void OnNewSearchQueries(
      const SearchQueries& searches,
      scoped_refptr<MigrationResultListener> listener) override {
    search_history_importer_test_.searches_ = &searches;
    listener->OnImportFinished(opera::SEARCH_HISTORY, true);
  }

 private:
  ~TestSearchHistoryReceiver() override {}
  SearchHistoryImporterTest& search_history_importer_test_;
};

TEST_F(SearchHistoryImporterTest, ParseGoodSearchHistoryFile) {
  base::FilePath history_path =
      ut::GetTestDataDir().AppendASCII("search_field_history.dat");

  std::unique_ptr<std::ifstream> history_file(
      new std::ifstream(history_path.value().c_str()));

  ASSERT_TRUE(history_file->is_open());
  ASSERT_FALSE(history_file->fail());

  scoped_refptr<SearchHistoryImporter> importer =
    new SearchHistoryImporter(make_scoped_refptr(
      new TestSearchHistoryReceiver(*this)));

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::SEARCH_HISTORY, true));
  importer->Import(std::move(history_file), migration_result_listener);
  ASSERT_FALSE(searches_ == NULL);

  const SearchQueries& searches = *searches_;

  ASSERT_EQ(4u, searches.size());

  EXPECT_EQ("search4", searches[0]);
  EXPECT_EQ("search3", searches[1]);
  EXPECT_EQ("search2", searches[2]);
  EXPECT_EQ("search1", searches[3]);
}

TEST_F(SearchHistoryImporterTest, ParseCorruptedSearchHistoryInput) {
  base::FilePath history_path =
      ut::GetTestDataDir().AppendASCII("search_field_history.dat");

  std::ifstream history_file(history_path.value().c_str());

  ASSERT_TRUE(history_file.is_open());
  ASSERT_FALSE(history_file.fail());

  std::string input_string(
      (std::istreambuf_iterator<char>(history_file)),
      std::istreambuf_iterator<char>());
  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  std::string input_string_corrupted = input_string;

  // Cut away term value
  size_t searched_pos = input_string_corrupted.find("search4");
  ASSERT_NE(input_string_corrupted.npos, searched_pos);
  ASSERT_LT(searched_pos + 7, input_string_corrupted.size());
  input_string_corrupted = input_string_corrupted.substr(0, searched_pos)
      + input_string_corrupted.substr(searched_pos + 7);

  std::unique_ptr<std::istringstream> input(
      new std::istringstream(input_string_corrupted));
  scoped_refptr<SearchHistoryImporter> importer1 =
    new SearchHistoryImporter(make_scoped_refptr(
      new TestSearchHistoryReceiver(*this)));
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::SEARCH_HISTORY, false));
  importer1->Import(std::move(input), migration_result_listener);

  input_string_corrupted = input_string;
  // Insert some extra attribute.
  searched_pos = input_string_corrupted.find("search3");
  ASSERT_NE(input_string_corrupted.npos, searched_pos);
  ASSERT_LT(searched_pos + 8, input_string_corrupted.size());
  input_string_corrupted = input_string_corrupted.substr(0, searched_pos + 8)
      + " nonsense=\"nonsense\""
      + input_string_corrupted.substr(searched_pos + 8);

  std::unique_ptr<std::istringstream> input2(
      new std::istringstream(input_string_corrupted));
  scoped_refptr<SearchHistoryImporter> importer2 =
    new SearchHistoryImporter(make_scoped_refptr(
      new TestSearchHistoryReceiver(*this)));
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::SEARCH_HISTORY, false));
  importer2->Import(std::move(input2), migration_result_listener);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
